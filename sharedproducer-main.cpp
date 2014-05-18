#include "ipc/producer.hpp"

#include <cstdlib>

int main (int argc, char** argv) {
    ipc::SharedProducer<int> producer { "barobo-daemon-master-queue" };

    if (!producer.waitForConsumer(std::chrono::seconds(1))) {
        LOG(debug) << "Consumer no-showed";
        return 1;
    }

    LOG(debug) << "Consumer's here, here's a bunch of shit";

    int i = argc > 1 ? atoi(argv[1]) : 0;
    for (int k = i; k < i + 10; ++k) {
        producer.send(k);
    }
    i += 10;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (int k = i; k < i + 10; ++k) {
        producer.send(k);
    }
    i += 10;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (int k = i; k < i + 10; ++k) {
        producer.send(k);
    }
    i += 10;

    std::this_thread::sleep_for(std::chrono::seconds(1));
}
