#ifndef IPCQ_SHAREDPRODUCER_HPP
#define IPCQ_SHAREDPRODUCER_HPP

#include "log.hpp"
#include "tmp_file_lock.hpp"
#include "queueconstructionerror.hpp"

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>

#include <chrono>
#include <string>
#include <thread>

namespace ipcq {

template <typename Msg>
class Producer {
public:
    Producer (const char* name)
            : mName(name)
            , mConsumptionMutex(mName + "-")
            , mProductionMutex(mName + "+") {
        using std::swap;

        /* TODO timed wait and throw exception in case daemon is stalled? */
        //boost::interprocess::sharable_lock<tmp_file_lock> productionLock { mProductionMutex };
        boost::interprocess::scoped_lock<tmp_file_lock> productionLock { mProductionMutex };
        swap(mProductionLock, productionLock);

        LOG(debug) << "Producer(" << mName << ") constructed";
    }

    template <typename Duration>
    bool waitForConsumer (Duration timeout) {
        using namespace boost::interprocess;
        LOG(debug) << "Waiting for consumer ...";

        auto stopTime = std::chrono::steady_clock::now() + timeout;
        while (mConsumptionMutex.try_lock()) {
            LOG(debug) << "Got consumption lock :(";
            mConsumptionMutex.unlock();
            if (std::chrono::steady_clock::now() >= stopTime) {
                LOG(debug) << "Timed out waiting for consumer";
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        LOG(debug) << "Consumer connected!";

        try {
            mQueue.reset(new message_queue(open_only, mName.c_str()));
        }
        catch (interprocess_exception& exc) {
            throw QueueConstructionError("Unable to open queue");
        }

        return true;
    }

    void send (Msg msg) {
    }

private:
    std::string mName;
    std::unique_ptr<boost::interprocess::message_queue> mQueue;
    boost::interprocess::scoped_lock<tmp_file_lock> mProductionLock;
    tmp_file_lock mConsumptionMutex;
    tmp_file_lock mProductionMutex;
};

}

#endif
