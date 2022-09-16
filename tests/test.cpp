//
// Created by rover on 03.09.22.
//

#include <gtest/gtest.h>
#include <vector>

#include "../radio_channel.h"
#include "../locket.h"
#include "../engine.h"

using namespace std;

class LocketTest : public ::testing::Test {
protected:
    void SetUp() override {
        configuration.tx_time = 5;
        radioChannel = TRadioChannel();
        engine = TEngine(&lockets, &radioChannel, &configuration);
    }

    TRadioChannel radioChannel;
    vector<Locket> lockets = vector<Locket>(2);
    TConfiguration configuration = TConfiguration();
    TEngine engine = TEngine(&lockets, &radioChannel, &configuration);
};

TEST_F(LocketTest, EmptyReceive) {
    lockets = vector<Locket>(2);
    lockets[0] = Locket(&radioChannel, 0, 0, optional<bool>(1), &configuration);
    lockets[1] = Locket(&radioChannel, 1, 0, optional<bool>(1), &configuration);

    for (int i = 0; i < 1; ++i) {
        engine.run_supercycle();
    }

    set<int> ans = {};
    ASSERT_EQ(lockets[0].received_ids_per_supercycle, ans);
}

TEST_F(LocketTest, ReceiveOtherLocket) {
    lockets = vector<Locket>(2);
    lockets[0] = Locket(&radioChannel, 0, 0, optional<bool>(0), &configuration);
    lockets[1] = Locket(&radioChannel, 1, 0, optional<bool>(1), &configuration);

    for (int i = 0; i < 1; ++i) {
        engine.run_supercycle();
    }

    set<int> ans = {1};
    ASSERT_EQ(lockets[0].received_ids_per_supercycle, ans);
}

TEST_F(LocketTest, TestInterference) {
    lockets = vector<Locket>(3);
    lockets[0] = Locket(&radioChannel, 0, 0, optional<bool>(0), &configuration);
    lockets[1] = Locket(&radioChannel, 1, 0, optional<bool>(1), &configuration);
    lockets[2] = Locket(&radioChannel, 2, 0, optional<bool>(1), &configuration);

    for (int i = 0; i < 1; ++i) {
        engine.run_supercycle();
    }

    ASSERT_EQ(lockets[0].received_ids_per_supercycle.size(), 0);
}

TEST_F(LocketTest, TestReceiveTwoLocketsWithChannelCheckDelay) {
    lockets = vector<Locket>(3);
    lockets[0] = Locket(&radioChannel, 0, 0, optional<bool>(0), &configuration);
    lockets[1] = Locket(&radioChannel, 1, 0, optional<bool>(1), &configuration);
    lockets[2] = Locket(&radioChannel, 2, 1, optional<bool>(1), &configuration);

    for (int i = 0; i < 1; ++i) {
        engine.run_supercycle();
    }

    set<int> ans = {1, 2};
    ASSERT_EQ(lockets[0].received_ids_per_supercycle, ans);
}

TEST_F(LocketTest, OnSecondSupercycle) {
    lockets = vector<Locket>(3);
    lockets[0] = Locket(&radioChannel, 0, 0, optional<bool>(0), &configuration);
    lockets[1] = Locket(&radioChannel, 1, 0, optional<bool>(1), &configuration);
    lockets[2] = Locket(&radioChannel, 2, 1, optional<bool>(1), &configuration);

    for (int i = 0; i < 1; ++i) {
        engine.run_supercycle();
    }
    lockets[0].tick();

    set<int> ans = {};
    ASSERT_EQ(lockets[0].received_ids_per_supercycle, ans);
}
