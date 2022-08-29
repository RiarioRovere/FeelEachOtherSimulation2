//
// Created by Dmitry Lavritov on 28.08.2022.
//

#pragma once

#include <string>
#include <utility>
#include <map>
#include <set>
#include <random>
#include <iostream>

#include <glog/logging.h>

#include "radio_channel.h"

using namespace std;

enum EState {
    OFF,
    SLEEP,
    WAKING_UP,
    RX,
    CHANNEL_CHECK,
    TX,
};

ostream &operator<<(ostream& o, EState state) {
    static const string NAMES[] = {
            "OFF", "SLEEP", "WAKING_UP", "RX", "CHANNEL_CHECK", "TX"
    };
    return o << NAMES[state];
}

constexpr int SUPERCYCLE_LENGTH = 1e6; // mcs
constexpr int CHANNEL_CHECK_TIME = 300; // mcs
constexpr int TX_TIME = 2 * 1e3;
constexpr int TIMESLOT_CNT = 5; // per supercycle


class Locket {
public:
    int locket_id;
    int turn_on_at;
    EState state;
    int ts;
    int relative_time = 0;
    int state_stopwatch = 0;
    int sleep_timer = 0;
    int timeslot_size;
    int timeslot_cnt;
    int rx_timeslot;
    pair<int, int> received_packets;
    set<int> received_ids_per_supercycle;
    TRadioChannel* radio_channel;

    Locket() = default;
    Locket& operator=(const Locket& other) = default;

    Locket(TRadioChannel* radio_channel, int locket_id, int turn_on_at) {
        this->ts = 0;
        this->turn_on_at = turn_on_at;
        this->locket_id = locket_id;
        this->radio_channel = radio_channel;
        state = EState::OFF;
        this->timeslot_size = SUPERCYCLE_LENGTH / TIMESLOT_CNT;
        timeslot_cnt = TIMESLOT_CNT;
        rx_timeslot = get_random_timeslot();
    }

    void tick() {
        // if new supercycle has began
        if (relative_time == 0) {
            on_new_supercycle();
        }

        process_state();

        ++ts;
        ++state_stopwatch;
        relative_time = (++relative_time) % SUPERCYCLE_LENGTH;
    }

private:
    void set_state(const EState& new_state) {
        received_packets = {-1, 0};
        state_stopwatch = 0;

        LOG(INFO) << "[" << locket_id << "]: Transfer status " << state << " -> " << new_state << " at " << ts;
        state = new_state;
    }

    void on_off() {
        if (ts == turn_on_at) {
            set_state(WAKING_UP);
        }
    }

    void on_sleep() {
        if (state_stopwatch == sleep_timer) {
            set_state(WAKING_UP);
        }
    }

    void on_waking_up() {
        if (state_stopwatch < 240) {
            return;
        }
        if (get_current_timeslot() == rx_timeslot) {
            set_state(RX);
        } else {
            set_state(CHANNEL_CHECK);
        }
    }

    void on_rx() {
        auto packet = radio_channel->receive();
        if (packet.has_value()) {
            auto& [id, packets_cnt] = received_packets;
            int received_id = packet.value();
            if (received_id == id) {
                ++packets_cnt;
            } else {
                id = received_id;
                packets_cnt = 1;
            }

            if (packets_cnt == TX_TIME) {
                // we received all packet with correct checksum
                received_ids_per_supercycle.insert(received_id);
            }
        }

        if (state_stopwatch + 1 == timeslot_size) {
            set_state(WAKING_UP);
        }
    }

    void on_channel_check() {
        if (!radio_channel->is_channel_free()) {
            this->sleep(TX_TIME);
        }
        if (state_stopwatch == CHANNEL_CHECK_TIME) {
            set_state(TX);
        }
    }

    void on_tx() {
        radio_channel->transmit(locket_id);

        if (state_stopwatch == TX_TIME) {
            sleep_before_next_timeslot();
            return;
        }
    }

    void on_new_supercycle() {
        rx_timeslot = get_random_timeslot();
        received_ids_per_supercycle.clear();
    }

    [[nodiscard]] int get_current_timeslot() const {
        int current_timeslot = relative_time / timeslot_size;
        assert(0 <= current_timeslot);
        assert(current_timeslot < timeslot_cnt);
        return current_timeslot;
    }

    [[nodiscard]] int get_random_timeslot() const {
        static random_device r;
        static default_random_engine e1(r());
        uniform_int_distribution<int> uniform_dist(0, timeslot_cnt);

        return uniform_dist(e1);
    }

    void sleep_before_next_timeslot() {
        int time_before_next_timeslot = (relative_time / timeslot_size + 1) * timeslot_size - relative_time;
        assert(time_before_next_timeslot > 0);
        this->sleep(time_before_next_timeslot);
    }

    void sleep(int mcs) {
        sleep_timer = mcs;
        set_state(SLEEP);
        LOG(INFO) << "Sleep before " << ts + mcs;
    }

    void process_state() {
        switch (state) {
            case EState::OFF:
                on_off();
                break;
            case EState::SLEEP:
                on_sleep();
                break;
            case EState::WAKING_UP:
                on_waking_up();
                break;
            case EState::RX:
                on_rx();
                break;
            case EState::CHANNEL_CHECK:
                on_channel_check();
                break;
            case EState::TX:
                on_tx();
                break;
        }
    }
};
