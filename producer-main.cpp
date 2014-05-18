#include "ipc/producer.hpp"

int main () {
    ipc::Producer<int> producer { "barobo-daemon-master-queue" };

    if (!producer.waitForConsumer(std::chrono::seconds(1))) {
        LOG(debug) << "Consumer no-showed";
        return 1;
    }

    LOG(debug) << "Consumer's here, here's a bunch of shit";

    for (int i = 0; i < 10; ++i) {
        producer.send(i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (int i = 10; i < 20; ++i) {
        producer.send(i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (int i = 10; i < 20; ++i) {
        producer.send(i);
    }
}
