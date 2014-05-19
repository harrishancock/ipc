#include "ipc/monospawn.hpp"
#include "util/log.hpp"

#include <thread>
#include <chrono>

int main (int argc, char** argv) try {
    /* Make sure we are the only instance of this program running on the
     * computer. Allow up to one second for a contemporaneous instance of this
     * program to die before giving up ourselves. */
    ipc::Monospawn sentinel { "barobo-daemon", std::chrono::seconds(1) };

    LOG(debug) << "Looks like I'm the first!";
    std::this_thread::sleep_for(std::chrono::seconds(3));
}
catch (ipc::FileLockError& exc) {
    LOG(debug) << "Interprocess synchronization error: " << exc.what();
}
catch (ipc::Monospawn::DuplicateProcess& exc) {
    LOG(debug) << exc.what();
}
