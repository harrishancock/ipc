#include "util/log.hpp"
#include "ipc/consumer.hpp"

int main () try {
    ipc::Consumer<int> consumer { "barobo-daemon-master-queue" };

    consumer.startServiceThread(
            [] (int i) { LOG(debug) << "Received a " << i; },
            std::chrono::seconds(4),
            std::chrono::milliseconds(10));

    std::this_thread::sleep_for(std::chrono::seconds(3));
    consumer.stopServiceThread();
}
catch (ipc::FileLockError& exc) {
    LOG(error) << "Interprocess synchronization error: " << exc.what();
}
catch (ipc::QueueError& exc) {
    LOG(error) << exc.what();
}
