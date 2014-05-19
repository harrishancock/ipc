#include "ipc/producer.hpp"

int main () try {
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
catch (ipc::NoConsumer& exc) {
    LOG(warning) << exc.what();
}
catch (ipc::FileLockError& exc) {
    LOG(error) << "Interprocess synchronization error: " << exc.what();
}
catch (ipc::QueueError& exc) {
    LOG(error) << exc.what();
}
