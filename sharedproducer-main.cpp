#include "ipc/producer.hpp"

#include <cstdlib>

int main (int argc, char** argv) try {
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
}
catch (ipc::NoConsumer& exc) {
    LOG(warning) << exc.what();
}
catch (ipc::FileLockError& exc) {
    LOG(error) << "Interprocess synchronization error: " << exc.what();
}
catch (ipc::QueueError& exc) {
    LOG(error) << exc.what();
}
