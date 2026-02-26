#pragma once

#include <cstdint>
#include <Preferences.h>

enum class PopUpId : uint8_t;

struct StatisticsCounters
{
    uint32_t boot_count = 0;
    uint32_t total_run_time_s = 0;

    uint32_t rh_pop_up_cycles = 0;
    uint32_t lh_pop_up_cycles = 0;
    uint32_t rh_pop_up_move_time_ms = 0;
    uint32_t lh_pop_up_move_time_ms = 0;

    uint32_t rh_pop_up_error_count = 0;
    uint32_t lh_pop_up_error_count = 0;

    uint32_t rh_button_presses = 0;
    uint32_t lh_button_presses = 0;
    uint32_t bh_button_presses = 0;

    uint32_t remote_input_1_presses = 0;
    uint32_t remote_input_2_presses = 0;
    uint32_t remote_input_3_presses = 0;
    uint32_t remote_input_4_presses = 0;
};

class StatisticsManager
{
public:
    explicit StatisticsManager(Preferences& prefs);

    // Initialize NVS preferences (must be called from setup(), not global init)
    void initialize();

    uint32_t increment_boot_count();
    uint32_t get_boot_count() const;

    void record_pop_up_move_time(PopUpId pop_up_id, uint32_t move_time_ms);
    void record_pop_up_cycle(PopUpId pop_up_id, uint32_t move_time_ms);
    void record_pop_up_error(PopUpId pop_up_id);

    void record_rh_button_press();
    void record_lh_button_press();
    void record_bh_button_press();
    void record_remote_input_press(uint8_t remote_index_1_to_4);
    void update_runtime();

    bool clear_all_statistics();
    void flush_deferred_counters();
    void print_statistics() const;

    const StatisticsCounters& counters() const;

private:
    Preferences& preferences_;
    StatisticsCounters counters_;
    bool initialized_;
    bool deferred_dirty_;
    bool runtime_dirty_;
    uint32_t runtime_unsaved_seconds_;
    uint32_t runtime_last_ms_;
    uint16_t runtime_remainder_ms_;

    void load_counters();
    void save_boot_count();
    void save_pop_up_counters();
    void save_runtime_counter();
    void save_input_counters();
    void save_deferred_counters();
    void mark_deferred_dirty();
    void add_pop_up_move_time(PopUpId pop_up_id, uint32_t move_time_ms);
};
