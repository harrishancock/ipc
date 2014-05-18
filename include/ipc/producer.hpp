#ifndef IPC_PRODUCER_HPP
#define IPC_PRODUCER_HPP

#include "common.hpp"
#include "tmp_file_lock.hpp"
#include "errors.hpp"

#include "util/log.hpp"

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>

#include <chrono>
#include <string>
#include <thread>

namespace ipc {

/* Lock is the template which will be used to get a production lock. Specify
 * boost::interprocess::scoped_lock to instantiate an exclusive producer, or
 * boost::interprocess::sharable_lock to instantiate a shared producer. */
template <typename Msg, template <typename> class Lock>
class BasicProducer {
public:
    BasicProducer (const char* name)
            : mName(name)
            , mConsumptionMutex(mName + IPC_CONSUMER_SUFFIX)
            , mProductionMutex(mName + IPC_PRODUCER_SUFFIX) {
        using std::swap;

        /* TODO timed wait and throw exception in case daemon is stalled? */
        Lock<tmp_file_lock> productionLock { mProductionMutex };
        swap(mProductionLock, productionLock);

        LOG(debug) << "Producer(" << mName << ") constructed";
    }

    template <typename Duration>
    bool waitForConsumer (Duration timeout) {
        using namespace boost::interprocess;
        LOG(debug) << "Waiting for consumer ...";

        auto stopTime = std::chrono::steady_clock::now() + timeout;
        while (mConsumptionMutex.try_lock()) {
            mConsumptionMutex.unlock();
            if (std::chrono::steady_clock::now() >= stopTime) {
                LOG(debug) << "Timed out waiting for consumer";
                return false;
            }
            /* XXX Might have to tune this value, but probably not critical for
             * it to be user-specifiable yet. */
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        LOG(debug) << "Consumer connected!";

        try {
            mQueue.reset(new message_queue(open_only, mName.c_str()));
        }
        catch (interprocess_exception& exc) {
            throw QueueError("Unable to open queue");
        }

        return true;
    }

    void send (Msg msg) {
        if (!mQueue) {
            throw QueueError("No consumer present");
        }

        try {
            mQueue->send(&msg, sizeof(msg), 0);
        }
        catch (boost::interprocess::interprocess_exception& exc) {
            throw QueueError("Internal queue error");
        }
    }

private:
    std::string mName;
    std::unique_ptr<boost::interprocess::message_queue> mQueue = nullptr;
    Lock<tmp_file_lock> mProductionLock;
    tmp_file_lock mConsumptionMutex;
    tmp_file_lock mProductionMutex;
};

template <typename Msg>
using Producer = BasicProducer<Msg, boost::interprocess::scoped_lock>;

template <typename Msg>
using SharedProducer = BasicProducer<Msg, boost::interprocess::sharable_lock>;

}

#endif
