#include "log.hpp"
#include "consumer.hpp"

#include <chrono>

void fuck (int i) {
    LOG(info) << "Read " << i;
};

int main () {
    ipcq::Consumer<int> consumer { "yoyoyo" };

    //consumer.waitForProducer(std::chrono::seconds(5));

    try {
        consumer.startServiceThread(fuck, std::chrono::seconds(5),
                std::chrono::milliseconds(500));
    }
    catch (std::exception& exc) {
        LOG(error) << "startServiceThread failed with " << exc.what();
    }

    try {
        consumer.joinServiceThread();
    }
    catch (std::exception& exc) {
        LOG(error) << "joinServiceThread failed with " << exc.what();
    }
}
