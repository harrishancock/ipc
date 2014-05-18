#include "log.hpp"
#include "producer.hpp"

#include <chrono>

int main () {
    ipcq::Producer<int> producer { "yoyoyo" };

    if (!producer.waitForConsumer(std::chrono::seconds(5))) {
        LOG(debug) << "The consumer no-showed";
        return 0;
    }

    for (int i = 0; i < 10; ++i) {
        producer.send(i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    for (int i = 0; i < 10; ++i) {
        producer.send(i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    for (int i = 0; i < 10; ++i) {
        producer.send(i);
    }
}
