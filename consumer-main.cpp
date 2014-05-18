#include "ipc/consumer.hpp"

int main () {
    ipc::Consumer<int> consumer { "barobo-daemon-master-queue" };

    consumer.startServiceThread(
            [] (int i) { LOG(debug) << "Received a " << i; },
            std::chrono::seconds(4),
            std::chrono::milliseconds(10));

    consumer.joinServiceThread();
}
