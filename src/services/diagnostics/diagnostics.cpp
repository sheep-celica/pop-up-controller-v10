#include "services/diagnostics/diagnostics.h"

#include <Arduino.h>
#include <Preferences.h>
#include <cstring>

#include "config.h"
#include "services/inputs/logic/light_switch_up.h"
#include "services/inputs/logic/light_switch_hold.h"
#include "services/io/leds.h"
#include "services/io/power.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "services/utilities/controller_status.h"

namespace {
    enum class Mode : uint8_t { OFF, SELECT, SWITCH, MOTION };
    enum : uint8_t { SNAPSHOT = 1, RAW = 2, DEBOUNCED = 3, COMMAND = 4,
                     POSITION = 5, MOVE_DONE = 6, ABORT = 7 };
    enum : uint8_t { UP_INPUT = 1, HOLD_INPUT = 2, RH = 3, LH = 4, BOTH = 5 };
    struct Event { uint32_t ms; uint32_t duration_ms; uint8_t kind; uint8_t source; uint8_t from; uint8_t to; };
    struct Record {
        uint32_t record_marker;
        uint32_t id;
        uint32_t duration_ms;
        uint16_t count;
        uint8_t test; // 1=switch, 2=motion
        uint8_t result; // 1=complete, 2=aborted
        uint8_t overflow;
        char board[24];
        char firmware[24];
        Event events[config::utilities::diagnostics::MAX_EVENTS];
    };
    constexpr uint32_t diagnostic_record_marker = 0x44494131; // "DIA1"
    Preferences prefs;
    bool prefs_ready = false;
    Mode mode = Mode::OFF;
    Record record = {};
    uint32_t started_ms = 0;
    uint32_t phase_started_ms = 0;
    uint32_t command_started_ms = 0;
    uint8_t phase = 0;
    bool waiting = false;
    uint32_t feedback_until_ms = 0;
    bool feedback_success = true;
    bool last_raw_up = false, last_raw_hold = false;
    bool last_stable_up = false, last_stable_hold = false;
    PopUpState last_rh = PopUpState::IN_BETWEEN, last_lh = PopUpState::IN_BETWEEN;

    const uint8_t step_sides[9] = { RH, RH, RH, LH, LH, LH, BOTH, BOTH, BOTH };
    const PopUpState step_targets[9] = {
        PopUpState::UP, PopUpState::DOWN, PopUpState::UP,
        PopUpState::UP, PopUpState::DOWN, PopUpState::UP,
        PopUpState::UP, PopUpState::DOWN, PopUpState::UP
    };

    bool raw_up() { return digitalRead(config::pins::LIGHT_SWITCH_UP_PIN) == LOW; }
    bool raw_hold() { return digitalRead(config::pins::LIGHT_SWITCH_HOLD_PIN) == LOW; }
    uint32_t elapsed() { return millis() - started_ms; }
    void add(uint8_t kind, uint8_t source, uint8_t from, uint8_t to, uint32_t at_ms)
    {
        if (record.count >= config::utilities::diagnostics::MAX_EVENTS) {
            record.overflow = 1;
            return;
        }
        record.events[record.count++] = {at_ms, 0, kind, source, from, to};
    }
    void add_now(uint8_t kind, uint8_t source, uint8_t from, uint8_t to)
    {
        add(kind, source, from, to, elapsed());
    }
    bool active() { return mode != Mode::OFF; }
    bool ensure_prefs()
    {
        if (prefs_ready) return true;
        prefs_ready = prefs.begin(config::utilities::diagnostics::NAMESPACE, false);
        return prefs_ready;
    }
    void set_pattern()
    {
        // Selection: two short pulses; switch: slow blink; motion: three pulses.
        const uint32_t cycle = millis() % 2000u;
        bool on = false;
        if (mode == Mode::SELECT) {
            if (static_cast<int32_t>(feedback_until_ms - millis()) > 0)
                on = feedback_success || cycle < 100 || (cycle >= 200 && cycle < 300) || (cycle >= 400 && cycle < 500) || (cycle >= 600 && cycle < 700);
            else on = cycle < 200 || (cycle >= 400 && cycle < 600);
        }
        if (mode == Mode::SWITCH) on = cycle < 1000;
        if (mode == Mode::MOTION) on = cycle < 150 || (cycle >= 300 && cycle < 450) || (cycle >= 600 && cycle < 750);
        set_diagnostic_illumination_override(active(), on);
    }
    void finish(uint8_t result, uint8_t reason)
    {
        if (result == 2) {
            const uint16_t before = record.count;
            add_now(ABORT, 0, 0, reason);
            if (record.count > before && waiting)
                record.events[record.count - 1].duration_ms = millis() - command_started_ms;
        }
        record.result = result;
        record.duration_ms = elapsed();
        const char* key = record.test == 1 ? "switch" : "motion";
        const size_t written = prefs.putBytes(key, &record, sizeof(record));
        LOG("Diagnostic %s %s: id=%lu events=%u overflow=%u saved=%s.",
            key, result == 1 ? "complete" : "aborted",
            static_cast<unsigned long>(record.id), record.count, record.overflow,
            written == sizeof(record) ? "yes" : "NO");
        mode = Mode::SELECT;
        feedback_success = result == 1 && written == sizeof(record);
        feedback_until_ms = millis() + 3000;
        waiting = false;
        set_pattern();
    }
    void begin(uint8_t test)
    {
        memset(&record, 0, sizeof(record));
        record.record_marker = diagnostic_record_marker;
        record.id = prefs.getUInt("next_id", 0) + 1;
        prefs.putUInt("next_id", record.id);
        record.test = test;
        strncpy(record.board, config::board::ID, sizeof(record.board) - 1);
        strncpy(record.firmware, get_current_build_version(), sizeof(record.firmware) - 1);
        started_ms = millis();
        reset_idle_time();
    }
    void capture_switch()
    {
        const bool ru = raw_up(), rh = raw_hold();
        const bool su = light_switch_up.is_high(), sh = light_switch_hold.is_high();
        if (ru != last_raw_up) { add_now(RAW, UP_INPUT, last_raw_up, ru); last_raw_up = ru; }
        if (rh != last_raw_hold) { add_now(RAW, HOLD_INPUT, last_raw_hold, rh); last_raw_hold = rh; }
        if (su != last_stable_up) { add_now(DEBOUNCED, UP_INPUT, last_stable_up, su); last_stable_up = su; }
        if (sh != last_stable_hold) { add_now(DEBOUNCED, HOLD_INPUT, last_stable_hold, sh); last_stable_hold = sh; }
    }
    void capture_position(PopUp& pop, uint8_t side, PopUpState& previous)
    {
        const PopUpState state = pop.get_state();
        if (state != previous) {
            add_now(POSITION, side, static_cast<uint8_t>(previous), static_cast<uint8_t>(state));
            previous = state;
        }
    }
    void abort_motion(uint8_t reason)
    {
        RH_POP_UP.stop_for_diagnostic_abort();
        LH_POP_UP.stop_for_diagnostic_abort();
        finish(2, reason);
    }
    void issue_step()
    {
        const uint8_t side = step_sides[phase];
        const PopUpState target = step_targets[phase];
        command_started_ms = millis();
        if (side == RH || side == BOTH) {
            add_now(COMMAND, RH, static_cast<uint8_t>(RH_POP_UP.get_state()), static_cast<uint8_t>(target));
            RH_POP_UP.set_target(target);
        }
        if (side == LH || side == BOTH) {
            add_now(COMMAND, LH, static_cast<uint8_t>(LH_POP_UP.get_state()), static_cast<uint8_t>(target));
            LH_POP_UP.set_target(target);
        }
        waiting = true;
    }
    bool step_done()
    {
        const uint8_t side = step_sides[phase];
        const PopUpState target = step_targets[phase];
        return (side == LH || (RH_POP_UP.get_target() == PopUpState::IDLE && last_rh == target)) &&
               (side == RH || (LH_POP_UP.get_target() == PopUpState::IDLE && last_lh == target));
    }
    const char* kind_name(uint8_t kind)
    {
        switch (kind) {
            case SNAPSHOT: return "snapshot"; case RAW: return "raw";
            case DEBOUNCED: return "debounced"; case COMMAND: return "command";
            case POSITION: return "position"; case MOVE_DONE: return "moveDone";
            case ABORT: return "abort"; default: return "unknown";
        }
    }
    void print_one(const char* key)
    {
        Record stored = {};
        if (prefs.getBytesLength(key) != sizeof(stored) || prefs.getBytes(key, &stored, sizeof(stored)) != sizeof(stored) ||
            stored.record_marker != diagnostic_record_marker || stored.count > config::utilities::diagnostics::MAX_EVENTS) {
            LOG("DIAG_NONE type=%s", key);
            return;
        }
        LOG("DIAG_BEGIN type=%s id=%lu result=%s duration_ms=%lu events=%u overflow=%u board=%s firmware=%s",
            key, static_cast<unsigned long>(stored.id), stored.result == 1 ? "complete" : "aborted",
            static_cast<unsigned long>(stored.duration_ms), stored.count, stored.overflow, stored.board, stored.firmware);
        for (uint16_t i = 0; i < stored.count; ++i) {
            const Event& e = stored.events[i];
            LOG("DIAG_EVENT id=%lu ms=%lu kind=%s source=%u from=%u to=%u duration_ms=%lu",
                static_cast<unsigned long>(stored.id), static_cast<unsigned long>(e.ms),
                kind_name(e.kind), e.source, e.from, e.to, static_cast<unsigned long>(e.duration_ms));
        }
        LOG("DIAG_END id=%lu", static_cast<unsigned long>(stored.id));
    }
}

bool diagnostics_active() { return active(); }
bool diagnostics_motion_active() { return mode == Mode::MOTION; }
bool diagnostics_light_switch_off()
{
    return !raw_up() && !raw_hold() && is_light_switch_safely_off() &&
           light_switch_up.get_stable_state_time() >= config::utilities::diagnostics::ENTRY_OFF_MS &&
           light_switch_hold.get_stable_state_time() >= config::utilities::diagnostics::ENTRY_OFF_MS;
}
void diagnostics_enter()
{
    if (active() || !diagnostics_light_switch_off()) return;
    if (!ensure_prefs()) { LOG("Diagnostic mode unavailable: NVS open failed."); return; }
    mode = Mode::SELECT;
    LOG("Diagnostic mode entered. RH=switch test, LH=motion test, BH=exit.");
    set_pattern();
}
void diagnostics_exit()
{
    if (mode == Mode::SWITCH || mode == Mode::MOTION) return;
    mode = Mode::OFF;
    set_diagnostic_illumination_override(false, false);
    LOG("Diagnostic mode exited.");
}
void diagnostics_select_rh()
{
    if (mode != Mode::SELECT) return;
    begin(1);
    last_raw_up = raw_up(); last_raw_hold = raw_hold();
    last_stable_up = light_switch_up.is_high(); last_stable_hold = light_switch_hold.is_high();
    add_now(SNAPSHOT, UP_INPUT, last_raw_up, last_stable_up);
    add_now(SNAPSHOT, HOLD_INPUT, last_raw_hold, last_stable_hold);
    mode = Mode::SWITCH;
    LOG("Switch test started: 30 seconds. Move OFF -> HOLD -> UP -> HOLD -> OFF, pausing at each position.");
    set_pattern();
}
void diagnostics_select_lh()
{
    if (mode != Mode::SELECT) return;
    if (!diagnostics_light_switch_off() || is_controller_bench_mode_enabled() ||
        RH_POP_UP.is_motion_locked_out() || LH_POP_UP.is_motion_locked_out() ||
        RH_POP_UP.get_sleepy_eye_mode() || LH_POP_UP.get_sleepy_eye_mode() ||
        !are_pop_ups_idle_or_timed_out()) {
        LOG("Motion test rejected: switch must be OFF, vehicle power present, pop-ups idle, sleepy-eye OFF, and no motion lockout.");
        return;
    }
    begin(2);
    phase = 0; waiting = false; phase_started_ms = millis();
    last_rh = RH_POP_UP.get_state(); last_lh = LH_POP_UP.get_state();
    add_now(SNAPSHOT, RH, static_cast<uint8_t>(last_rh), static_cast<uint8_t>(last_rh));
    add_now(SNAPSHOT, LH, static_cast<uint8_t>(last_lh), static_cast<uint8_t>(last_lh));
    mode = Mode::MOTION;
    LOG("Motion test started. RH, LH, then both: UP-DOWN-UP.");
    set_pattern();
}
void diagnostics_select_bh() { diagnostics_exit(); }
void diagnostics_update()
{
    if (!active()) return;
    reset_idle_time();
    set_pattern();
    if (mode == Mode::SWITCH) {
        capture_switch();
        if (elapsed() >= config::utilities::diagnostics::SWITCH_TEST_MS) finish(1, 0);
        return;
    }
    if (mode != Mode::MOTION) return;
    capture_position(RH_POP_UP, RH, last_rh);
    capture_position(LH_POP_UP, LH, last_lh);
    if (raw_up() || raw_hold() || light_switch_up.is_high() || light_switch_hold.is_high()) { abort_motion(1); return; }
    if (RH_POP_UP.is_motion_locked_out() || LH_POP_UP.is_motion_locked_out()) { abort_motion(2); return; }
    if (!waiting) {
        if (millis() - phase_started_ms >= (phase == 0 ? 0u : config::utilities::diagnostics::STEP_PAUSE_MS)) issue_step();
    } else if (step_done()) {
        const uint16_t before = record.count;
        add_now(MOVE_DONE, step_sides[phase], static_cast<uint8_t>(step_targets[phase]),
                static_cast<uint8_t>(step_targets[phase]));
        if (record.count > before) record.events[record.count - 1].duration_ms = millis() - command_started_ms;
        LOG("Diagnostic motion step %u done in %lu ms.", phase + 1,
            static_cast<unsigned long>(millis() - command_started_ms));
        waiting = false;
        phase_started_ms = millis();
        if (++phase == 9) finish(1, 0);
    }
}
void diagnostics_record_switch_command(bool rh, uint8_t from, uint8_t to)
{
    if (mode == Mode::SWITCH) add_now(COMMAND, rh ? RH : LH, from, to);
}
void diagnostics_print()
{
    if (!ensure_prefs()) { LOG("Diagnostic read failed: NVS open failed."); return; }
    print_one("switch"); print_one("motion");
}
bool diagnostics_clear()
{
    if (mode == Mode::SWITCH || mode == Mode::MOTION) return false;
    if (!ensure_prefs()) return false;
    const bool sw_ok = !prefs.isKey("switch") || prefs.remove("switch");
    const bool mot_ok = !prefs.isKey("motion") || prefs.remove("motion");
    const bool ok = sw_ok && mot_ok;
    return ok;
}
