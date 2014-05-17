#include "log.hpp"
#include "producer.hpp"

#include <chrono>

int main () {
    ipcq::Producer<int> producer { "yoyoyo" };

    producer.waitForConsumer(std::chrono::seconds(5));
    for (int i = 0; i < 10; ++i) {
        producer.send(i);
    }
}
