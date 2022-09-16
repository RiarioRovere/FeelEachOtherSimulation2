#include <iostream>
#include <glog/logging.h>
#include <optional>

#include "engine.h"
#include "locket.h"
#include "radio_channel.h"
#include "configuration.h"

constexpr int LOCKET_CNT = 3;

ostream &operator<<(ostream &o, const set<int> &state) {
    for (auto s : state) {
        o << s << " ";
    }
    return o;
}


int main(int argc, char* argv[]) {
    // Initialize Google’s logging library.
    google::InitGoogleLogging(argv[0]);
    google::LogToStderr();

    TConfiguration configuration;

    LOG(INFO) << "Start application";

    TRadioChannel radioChannel;

    vector<Locket> lockets(LOCKET_CNT);
    for (int i = 0; i < LOCKET_CNT; ++i) {
        lockets[i] = Locket(&radioChannel, i, 0, optional<bool>(), &configuration, true);
    }

    TEngine engine(&lockets, &radioChannel, &configuration);
    for (int i = 0; i < 10 ; ++i) {
        engine.run_supercycle();
        cout << "Locket has received: " << lockets[0].received_ids_per_supercycle.size() << " = " << lockets[0].received_ids_per_supercycle << endl;
    }

    LOG(INFO) << "Simulation has finished";

    return 0;
}
