//
// Created by Dmitry Lavritov on 28.08.2022.
//

#pragma once

#include "locket.h"

class TEngine {
public:
    TEngine(
            vector<Locket> *lockets,
            TRadioChannel *radioChannel
    ) : radioChannel(radioChannel) {
        this->lockets = lockets;
    }

    void run_supercycle() {
        for (int i = 0; i < SUPERCYCLE_LENGTH; ++i) {
            tick();
        }
    }

private:
    vector<Locket> *lockets;
    TRadioChannel *radioChannel;
    int ts = 0;

    void tick() {
        auto tx_lockets = get_lockets_in_tx_state();

//        LOG_EVERY_N(INFO, 100000) << "Process " << google::COUNTER << "th tick";

        for (auto &locket: *lockets) {
            if (tx_lockets.contains(locket.locket_id)) {
                locket.tick();
                CHECK(locket.ts == ts + 1) << "locket_ts = " << locket.ts << " engine ts = " << ts;
            }
        }

        for (auto &locket: *lockets) {
            if (!tx_lockets.contains(locket.locket_id)) {
                locket.tick();
                CHECK(locket.ts == ts + 1) << "locket_ts = " << locket.ts << " engine ts = " << ts;
            }
        }

        ++ts;
        radioChannel->tick();
        assert(radioChannel->ts == ts);
    }

    set<int> get_lockets_in_tx_state() {
        set<int> ans;
        for (auto &locket: *lockets) {
            if (locket.state == TX) ans.insert(locket.locket_id);
        }
        return ans;
    }
};