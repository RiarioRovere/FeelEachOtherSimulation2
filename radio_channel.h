//
// Created by Dmitry Lavritov on 28.08.2022.
//

#pragma once

#include <vector>
#include <optional>

using namespace std;

class TRadioChannel {
public:
    TRadioChannel() {
        ts = 0;
    }

    int ts;
    vector<int> data;

    void transmit(int id) {
        data.push_back(id);
    }

    optional<int> receive() {
        if (data.size() == 1) {
            return { data.front() };
        }
        return {};
    }

    [[nodiscard]] bool is_channel_free() const {
        return data.empty();
    }

    void tick() {
        data.clear();
        ++ts;
    }
};
