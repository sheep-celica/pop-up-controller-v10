#include "services/logging/statistics_manager.h"
#include <Arduino.h>
#include <limits>
#include "helpers/pop_up.h"
#include "config.h"
#include "services/logging/logging.h"

namespace {
    // Keep keys short for ESP32 Preferences/NVS key length limits.
    constexpr const char* KEY_BOOT   = "boot";
    constexpr const char* KEY_RUN_S  = "run_s";
    constexpr const char* KEY_RH_CYC = "rh_cyc";
    constexpr const char* KEY_LH_CYC = "lh_cyc";
    constexpr const char* KEY_RH_MVM = "rh_mvm";
    constexpr const char* KEY_LH_MVM = "lh_mvm";
    constexpr const char* KEY_RH_ERR = "rh_err";
    constexpr const char* KEY_LH_ERR = "lh_err";

    constexpr const char* KEY_RH_BTN = "rh_btn";
    constexpr const char* KEY_LH_BTN = "lh_btn";
    constexpr const char* KEY_BH_BTN = "bh_btn";
    constexpr const char* KEY_R1_BTN = "r1_btn";
    constexpr const char* KEY_R2_BTN = "r2_btn";
    constexpr const char* KEY_R3_BTN = "r3_btn";
    constexpr const char* KEY_R4_BTN = "r4_btn";
}

StatisticsManager::StatisticsManager(Preferences& prefs)
    : preferences_(prefs),
      counters_(),
      initialized_(false),
      deferred_dirty_(false),
      runtime_dirty_(false),
      runtime_unsaved_seconds_(0),
      runtime_last_ms_(0),
      runtime_remainder_ms_(0)
{
}

void StatisticsManager::initialize()
{
    preferences_.begin(config::utilities::STATISTICAL_LOG_NAMESPACE, false);
    load_counters();
    runtime_last_ms_ = millis();
    runtime_remainder_ms_ = 0;
    runtime_unsaved_seconds_ = 0;
    runtime_dirty_ = false;
    initialized_ = true;
}

uint32_t StatisticsManager::increment_boot_count()
{
    ++counters_.boot_count;
    save_boot_count();
    return counters_.boot_count;
}

uint32_t StatisticsManager::get_boot_count() const
{
    return counters_.boot_count;
}

void StatisticsManager::record_pop_up_cycle(PopUpId pop_up_id, uint32_t move_time_ms)
{
    switch (pop_up_id)
    {
        case PopUpId::RH:
            ++counters_.rh_pop_up_cycles;
            break;

        case PopUpId::LH:
            ++counters_.lh_pop_up_cycles;
            break;
    }

    add_pop_up_move_time(pop_up_id, move_time_ms);
    save_pop_up_counters();
}

void StatisticsManager::record_pop_up_move_time(PopUpId pop_up_id, uint32_t move_time_ms)
{
    add_pop_up_move_time(pop_up_id, move_time_ms);
    save_pop_up_counters();
}

void StatisticsManager::record_pop_up_error(PopUpId pop_up_id)
{
    switch (pop_up_id)
    {
        case PopUpId::RH:
            ++counters_.rh_pop_up_error_count;
            break;

        case PopUpId::LH:
            ++counters_.lh_pop_up_error_count;
            break;
    }

    save_pop_up_counters();
}

void StatisticsManager::record_rh_button_press()
{
    ++counters_.rh_button_presses;
    mark_deferred_dirty();
}

void StatisticsManager::record_lh_button_press()
{
    ++counters_.lh_button_presses;
    mark_deferred_dirty();
}

void StatisticsManager::record_bh_button_press()
{
    ++counters_.bh_button_presses;
    mark_deferred_dirty();
}

void StatisticsManager::record_remote_input_press(uint8_t remote_index_1_to_4)
{
    switch (remote_index_1_to_4)
    {
        case 1: ++counters_.remote_input_1_presses; break;
        case 2: ++counters_.remote_input_2_presses; break;
        case 3: ++counters_.remote_input_3_presses; break;
        case 4: ++counters_.remote_input_4_presses; break;
        default: return;
    }

    mark_deferred_dirty();
}

void StatisticsManager::update_runtime()
{
    if (!initialized_) {
        return;
    }

    const uint32_t now_ms = millis();
    const uint32_t delta_ms = now_ms - runtime_last_ms_; // uint32_t subtraction is wrap-safe for millis()
    runtime_last_ms_ = now_ms;

    const uint32_t accumulated_ms = static_cast<uint32_t>(runtime_remainder_ms_) + delta_ms;
    const uint32_t delta_seconds = accumulated_ms / 1000u;
    runtime_remainder_ms_ = static_cast<uint16_t>(accumulated_ms % 1000u);

    if (delta_seconds == 0) {
        return;
    }

    const uint32_t max_u32 = std::numeric_limits<uint32_t>::max();
    const uint32_t remaining = max_u32 - counters_.total_run_time_s;
    counters_.total_run_time_s += (delta_seconds > remaining) ? remaining : delta_seconds;

    const uint32_t unsaved_remaining = max_u32 - runtime_unsaved_seconds_;
    runtime_unsaved_seconds_ += (delta_seconds > unsaved_remaining) ? unsaved_remaining : delta_seconds;

    runtime_dirty_ = true;

    if (runtime_unsaved_seconds_ >= config::utilities::statistics::RUNTIME_FLUSH_SECONDS)
    {
        save_runtime_counter();
    }
}

bool StatisticsManager::clear_all_statistics()
{
    if (!initialized_) {
        return false;
    }

    if (!preferences_.clear()) {
        return false;
    }

    counters_ = StatisticsCounters{};
    deferred_dirty_ = false;
    runtime_dirty_ = false;
    runtime_unsaved_seconds_ = 0;
    runtime_remainder_ms_ = 0;
    runtime_last_ms_ = millis();

    return true;
}

void StatisticsManager::flush_deferred_counters()
{
    save_deferred_counters();
}

void StatisticsManager::print_statistics() const
{
    LOG("---- Statistics ----");
    LOG("Boot count: %lu", counters_.boot_count);
    LOG("Total runtime: %lu s (%.2f days)",
        counters_.total_run_time_s,
        counters_.total_run_time_s / 86400.0f);
    LOG("RH cycles / errors / move: %lu / %lu / %lu ms",
        counters_.rh_pop_up_cycles,
        counters_.rh_pop_up_error_count,
        counters_.rh_pop_up_move_time_ms);
    LOG("LH cycles / errors / move: %lu / %lu / %lu ms",
        counters_.lh_pop_up_cycles,
        counters_.lh_pop_up_error_count,
        counters_.lh_pop_up_move_time_ms);
    LOG("Buttons RH/LH/BH: %lu / %lu / %lu",
        counters_.rh_button_presses,
        counters_.lh_button_presses,
        counters_.bh_button_presses);
    LOG("Remote 1/2/3/4: %lu / %lu / %lu / %lu",
        counters_.remote_input_1_presses,
        counters_.remote_input_2_presses,
        counters_.remote_input_3_presses,
        counters_.remote_input_4_presses);
    LOG("--------------------");
}

const StatisticsCounters& StatisticsManager::counters() const
{
    return counters_;
}

void StatisticsManager::load_counters()
{
    counters_.boot_count = preferences_.getUInt(KEY_BOOT, 0);
    counters_.total_run_time_s = preferences_.getUInt(KEY_RUN_S, 0);
    counters_.rh_pop_up_cycles = preferences_.getUInt(KEY_RH_CYC, 0);
    counters_.lh_pop_up_cycles = preferences_.getUInt(KEY_LH_CYC, 0);
    counters_.rh_pop_up_move_time_ms = preferences_.getUInt(KEY_RH_MVM, 0);
    counters_.lh_pop_up_move_time_ms = preferences_.getUInt(KEY_LH_MVM, 0);
    counters_.rh_pop_up_error_count = preferences_.getUInt(KEY_RH_ERR, 0);
    counters_.lh_pop_up_error_count = preferences_.getUInt(KEY_LH_ERR, 0);

    counters_.rh_button_presses = preferences_.getUInt(KEY_RH_BTN, 0);
    counters_.lh_button_presses = preferences_.getUInt(KEY_LH_BTN, 0);
    counters_.bh_button_presses = preferences_.getUInt(KEY_BH_BTN, 0);
    counters_.remote_input_1_presses = preferences_.getUInt(KEY_R1_BTN, 0);
    counters_.remote_input_2_presses = preferences_.getUInt(KEY_R2_BTN, 0);
    counters_.remote_input_3_presses = preferences_.getUInt(KEY_R3_BTN, 0);
    counters_.remote_input_4_presses = preferences_.getUInt(KEY_R4_BTN, 0);

    deferred_dirty_ = false;
    runtime_dirty_ = false;
    runtime_unsaved_seconds_ = 0;
}

void StatisticsManager::save_boot_count()
{
    if (!initialized_) {
        return;
    }

    preferences_.putUInt(KEY_BOOT, counters_.boot_count);
}

void StatisticsManager::save_pop_up_counters()
{
    if (!initialized_) {
        return;
    }

    preferences_.putUInt(KEY_RH_CYC, counters_.rh_pop_up_cycles);
    preferences_.putUInt(KEY_LH_CYC, counters_.lh_pop_up_cycles);
    preferences_.putUInt(KEY_RH_MVM, counters_.rh_pop_up_move_time_ms);
    preferences_.putUInt(KEY_LH_MVM, counters_.lh_pop_up_move_time_ms);
    preferences_.putUInt(KEY_RH_ERR, counters_.rh_pop_up_error_count);
    preferences_.putUInt(KEY_LH_ERR, counters_.lh_pop_up_error_count);
}

void StatisticsManager::save_runtime_counter()
{
    if (!initialized_ || !runtime_dirty_) {
        return;
    }

    preferences_.putUInt(KEY_RUN_S, counters_.total_run_time_s);
    runtime_dirty_ = false;
    runtime_unsaved_seconds_ = 0;
}

void StatisticsManager::save_input_counters()
{
    if (!initialized_ || !deferred_dirty_) {
        return;
    }

    preferences_.putUInt(KEY_RH_BTN, counters_.rh_button_presses);
    preferences_.putUInt(KEY_LH_BTN, counters_.lh_button_presses);
    preferences_.putUInt(KEY_BH_BTN, counters_.bh_button_presses);
    preferences_.putUInt(KEY_R1_BTN, counters_.remote_input_1_presses);
    preferences_.putUInt(KEY_R2_BTN, counters_.remote_input_2_presses);
    preferences_.putUInt(KEY_R3_BTN, counters_.remote_input_3_presses);
    preferences_.putUInt(KEY_R4_BTN, counters_.remote_input_4_presses);

    deferred_dirty_ = false;
}

void StatisticsManager::save_deferred_counters()
{
    if (!initialized_) {
        return;
    }

    save_runtime_counter();
    save_input_counters();
}

void StatisticsManager::mark_deferred_dirty()
{
    deferred_dirty_ = true;
}

void StatisticsManager::add_pop_up_move_time(PopUpId pop_up_id, uint32_t move_time_ms)
{
    const uint32_t max_u32 = std::numeric_limits<uint32_t>::max();

    switch (pop_up_id)
    {
        case PopUpId::RH:
            counters_.rh_pop_up_move_time_ms +=
                (move_time_ms > (max_u32 - counters_.rh_pop_up_move_time_ms))
                    ? (max_u32 - counters_.rh_pop_up_move_time_ms)
                    : move_time_ms;
            break;

        case PopUpId::LH:
            counters_.lh_pop_up_move_time_ms +=
                (move_time_ms > (max_u32 - counters_.lh_pop_up_move_time_ms))
                    ? (max_u32 - counters_.lh_pop_up_move_time_ms)
                    : move_time_ms;
            break;
    }
}
