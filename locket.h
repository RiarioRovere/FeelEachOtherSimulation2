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
#include <cassert>
#include <optional>

#include <glog/logging.h>

#include "radio_channel.h"
#include "configuration.h"

using namespace std;

enum EState {
    OFF,
    SLEEP,
    WAKING_UP,
    RX,
    CHANNEL_CHECK,
    TX,
    BEFORE_RX_CHANNEL_CHECK,
    BEFORE_RX_TX, // TX that right before RX
};

ostream &operator<<(ostream &o, EState state) {
    static const string NAMES[] = {
            "OFF", "SLEEP", "WAKING_UP", "RX", "CHANNEL_CHECK", "TX", "BEFORE_RX_CHANNEL_CHECK", "BEFORE_RX_TX"
    };
    return o << NAMES[state];
}

class TTimers {
public:
    int turn_on_at = 0;
    int absolute_time = 0;
    int relative_time = 0;
    int state_timer = 0;
    int sleep_timer = 0;
    int supercycle_cnt = 0;

    TTimers() = default;

    explicit TTimers(int turn_on_at) {
        this->turn_on_at = turn_on_at;
    }

    TTimers(int turnOnAt, int ts, int relativeTime, int stateTimer, int sleepTimer) : turn_on_at(turnOnAt), absolute_time(ts),
                                                                                      relative_time(relativeTime),
                                                                                      state_timer(stateTimer),
                                                                                      sleep_timer(sleepTimer) {}
};

class Locket {
public:
    int locket_id;
    EState state;
    int rx_timeslot;
    int rx_length; // В таймслотах
    pair<int, int> received_packets;
    set<int> received_ids_per_supercycle;
    TRadioChannel *radio_channel;
    // Предопределенный timeslot приема
    optional<int> constant_rx_timeslot;
    bool on_start_supercycle_jitter;
    bool is_channel_free;
    static bool enable_logging;
    int power_consumption = 0;

    // settings
    TConfiguration* configuration;
    TTimers timers;

    Locket() = default;

    Locket &operator=(const Locket &other) = default;

    Locket(
            TRadioChannel *radio_channel,
            int locket_id,
            int turn_on_at,
            optional<int> constant_rx_timeslot,
            TConfiguration *configuration,
            bool on_start_supercycle_jitter = false
    ) : timers(TTimers(turn_on_at)) {
        this->locket_id = locket_id;
        this->radio_channel = radio_channel;
        state = EState::OFF;
        this->configuration = configuration;
        this->rx_length = 2;
        rx_timeslot = constant_rx_timeslot.value_or(get_random_timeslot(rx_length));
        this->constant_rx_timeslot = constant_rx_timeslot;
        this->on_start_supercycle_jitter = on_start_supercycle_jitter;
        this->is_channel_free = true;

        this->timers.turn_on_at = turn_on_at;
        if (configuration->on_start_jitter) {
            this->timers.turn_on_at =
                    timers.absolute_time + get_random_int(0, configuration->supercycle_length / 4 * 3);
        }
    }

    void tick(int n) {
        while (n-- > 0) {
            tick();
        }
    }

    void tick() {
        // if new supercycle has began
        if (timers.relative_time == 0) {
            on_new_supercycle();
        }

        process_state();

        ++timers.state_timer;
        ++timers.absolute_time;
        if (state != OFF) {
            timers.relative_time = (++timers.relative_time) % configuration->supercycle_length;
        }
    }

private:
    void set_state(const EState new_state) {
        if (locket_id == 0) {
//            Locket::enable_logging = new_state == RX || configuration->log_all_events;
            Locket::enable_logging = false;

            if (state == RX && new_state != RX) {
                cout << "RX duration = " << timers.state_timer + 1 << "/" << configuration->timeslot_size << endl;
            }
        }
        is_channel_free = true;
        received_packets = {-1, 0};
        timers.state_timer = 0;

        if (Locket::enable_logging)
        LOG(INFO) << "[" << locket_id << "]: Transfer status " << state << " -> " << new_state << " at "
                  << timers.absolute_time;
        state = new_state;
    }

    void on_off() {
        if (timers.absolute_time == timers.turn_on_at) {
            set_state(WAKING_UP);
        }
    }

    void on_sleep() {
        if (timers.state_timer == timers.sleep_timer) {
            set_state(WAKING_UP);
        }
    }

    void on_waking_up() {
        if (timers.state_timer < 240) {
            return;
        }
        if (get_current_timeslot() == rx_timeslot) {
            set_state(BEFORE_RX_CHANNEL_CHECK);
//            set_state(RX);
        } else {
            set_state(CHANNEL_CHECK);
        }
    }

    void on_rx() {
        auto packet = radio_channel->receive();
        if (packet.has_value()) {
            auto &[id, packets_cnt] = received_packets;
            int received_id = packet.value();
            if (received_id == id) {
                ++packets_cnt;
            } else {
                id = received_id;
                packets_cnt = 1;
            }

            if (packets_cnt == configuration->tx_time) {
                // we received all packet with correct checksum
                received_ids_per_supercycle.insert(received_id);
            }
        }

        if (timers.state_timer + 1 == configuration->timeslot_size * rx_length) {
            set_state(WAKING_UP);
        }
    }

    void on_channel_check() {
        if (!radio_channel->is_channel_free()) {
            is_channel_free = false;
        }
        if (timers.state_timer == configuration->channel_check_time) {
            if (is_channel_free) {
                set_state(TX);
            } else {
                this->sleep(configuration->tx_time);
            }
        }
    }

    void on_tx() {
        radio_channel->transmit(locket_id);

        if (timers.state_timer == configuration->tx_time) {
            sleep_before_next_timeslot();
            return;
        }
    }

    void on_before_rx_channel_check() {
        if (!radio_channel->is_channel_free()) {
            is_channel_free = false;
        }
        if (timers.state_timer == configuration->channel_check_time) {
            if (is_channel_free) {
                set_state(BEFORE_RX_TX);
            } else {
                set_state(RX);
            }
        }
    }

    void on_before_rx_tx() {
        radio_channel->transmit(locket_id);

        if (timers.state_timer == configuration->tx_time) {
            set_state(RX);
            return;
        }
    }

    void on_new_supercycle() {
        rx_timeslot = constant_rx_timeslot.value_or(get_random_timeslot(rx_length));
        received_ids_per_supercycle.clear();
//        if (on_start_supercycle_jitter) {
//            if (timers.supercycle_cnt % 100 != 0) {
//                timers.turn_on_at = timers.absolute_time + get_random_int(0, 300);
//            } else {
//                timers.turn_on_at = timers.absolute_time + get_random_int(0, configuration->supercycle_length / 4 * 3);
//            }
//
//            set_state(OFF);
//        }
        timers.supercycle_cnt++;
    }

    [[nodiscard]] int get_current_timeslot() const {
        int current_timeslot = timers.relative_time / configuration->timeslot_size;
        assert(0 <= current_timeslot);
        assert(current_timeslot < configuration->timeslot_cnt);
        return current_timeslot;
    }

    // Резервируем начало непрерывного отрезка из N последовательных таймслотов
    [[nodiscard]] int get_random_timeslot(int timeslot_cnt_to_reserve) const {
        int rnd_ts = get_random_int(0, configuration->timeslot_cnt - timeslot_cnt_to_reserve);
        CHECK(rnd_ts >= 0);
        CHECK(rnd_ts < configuration->timeslot_cnt) << rnd_ts << " " << configuration->timeslot_cnt;
        return rnd_ts;
    }

    [[nodiscard]] static int get_random_int(int a, int b) {
        static random_device r;
        static default_random_engine e1(r());
        uniform_int_distribution<int> uniform_dist(a, b);
        return uniform_dist(e1);
    }

    void sleep_before_next_timeslot() {
        int time_before_next_timeslot = (timers.relative_time / configuration->timeslot_size + 1) *
                                        configuration->timeslot_size - timers.relative_time;
        assert(time_before_next_timeslot > 0);
        this->sleep(time_before_next_timeslot);
    }

    void sleep(int mcs) {
        timers.sleep_timer = mcs;
        set_state(SLEEP);
//        LOG(INFO) << "Sleep before " << timers.absolute_time + mcs;
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
            case EState::BEFORE_RX_CHANNEL_CHECK:
                on_before_rx_channel_check();
                break;
            case EState::BEFORE_RX_TX:
                on_before_rx_tx();
                break;
        }
    }
};

bool Locket::enable_logging = false;
