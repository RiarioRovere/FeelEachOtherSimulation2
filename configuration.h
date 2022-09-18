//
// Created by rover on 10.09.22.
//

#pragma once

class TConfiguration {
public:
    int tx_time{};
    int supercycle_length{};
    int channel_check_time{};
    int timeslot_cnt{};
    int timeslot_size{};
    bool on_start_jitter{};
    bool log_all_events{};

    explicit TConfiguration(int tx_time = 2 * 1e3,
                            int supercycle_length = 4e6,
                            int channel_check_time = 300,
                            int timeslot_cnt = 16,
                            int on_start_jitter = true) {
        this->tx_time = tx_time;
        this->supercycle_length = supercycle_length;
        this->channel_check_time = channel_check_time;
        this->timeslot_cnt = timeslot_cnt;
        this->timeslot_size = supercycle_length / timeslot_cnt;
        this->on_start_jitter = on_start_jitter;
        log_all_events = false;
    }
};
