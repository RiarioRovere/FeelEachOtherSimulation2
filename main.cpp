#include <iostream>
#include <glog/logging.h>

#include "engine.h"
#include "radio_channel.h"

constexpr int LOCKET_CNT = 30;

int main(int argc, char* argv[]) {
    // Initialize Google’s logging library.
    google::InitGoogleLogging(argv[0]);
    google::LogToStderr();

    LOG(INFO) << "Start application";

    TRadioChannel radioChannel;

    vector<Locket> lockets(LOCKET_CNT);
    for (int i = 0; i < LOCKET_CNT; ++i) {
        lockets[i] = Locket(&radioChannel, i, 0);
    }

    TEngine engine(&lockets, &radioChannel);
    for (int i = 0; i < 1; ++i) {
        engine.run_supercycle();
    }

    LOG(INFO) << "Simulation has finished";

    return 0;
}
