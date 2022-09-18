//
// Created by Dmitry Lavritov on 28.08.2022.
//

#pragma once

#include "locket.h"
#include "configuration.h"

class TEngine {
public:
    TConfiguration *configuration;

    TEngine(
            vector<Locket> *lockets,
            TRadioChannel *radioChannel,
            TConfiguration *configuration
    ) : radioChannel(radioChannel) {
        this->lockets = lockets;
        this->configuration = configuration;
    }

    void run_supercycle() {
        int cnt = 0;
        while ((*lockets)[0].state == OFF) {
            tick();
            cnt++;
        }
//        print_all_states();
        do {
            tick();
            cnt++;
        } while ((*lockets)[0].timers.relative_time > 0);
//        for (int i = 0; i < configuration->supercycle_length; ++i) {
//            tick();
//        }
        cout << "Supercycle takes: " << cnt << endl;
        print_all_states();
    }

private:
    vector<Locket> *lockets;
    TRadioChannel *radioChannel;
    int ts = 0;

    void tick() {
        auto tx_lockets = get_lockets_in_tx_state();

//        LOG_EVERY_N(INFO, 100000) << "Process " << google::COUNTER << "th tick";

        for (auto &locket: *lockets) {
            if (tx_lockets.count(locket.locket_id) > 0) {
                locket.tick();
                CHECK(locket.timers.absolute_time == ts + 1) << "locket_ts = " << locket.timers.absolute_time << " engine ts = " << ts;
            }
        }

        for (auto &locket: *lockets) {
            if (tx_lockets.count(locket.locket_id) == 0) {
                locket.tick();
                CHECK(locket.timers.absolute_time == ts + 1) << "locket_ts = " << locket.timers.absolute_time << " engine ts = " << ts;
            }
        }

        ++ts;
        radioChannel->tick();
        assert(radioChannel->ts == ts);
    }

    set<int> get_lockets_in_tx_state() {
        set<int> ans;
        for (auto &locket: *lockets) {
            if (locket.state == TX || locket.state == BEFORE_RX_TX) ans.insert(locket.locket_id);
        }
        return ans;
    }

    void print_all_states() {
        for (auto &locket : *lockets) {
            cout << locket.locket_id << ":" << locket.state << "; ";
        }
        cout << endl;
    }
};