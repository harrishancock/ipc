#ifndef IPCQ_SHAREDPRODUCER_HPP
#define IPCQ_SHAREDPRODUCER_HPP

#include "log.hpp"
#include "tmp_file_lock.hpp"
#include "queueconstructionerror.hpp"
#include "std_chrono_duration_to_posix_time_duration.hpp"

#include <boost/scope_exit.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include <atomic>
#include <string>
#include <thread>

namespace ipcq {

template <typename Msg, size_t N = 100>
class Consumer {
public:
    enum FailState {
        NO_ERROR,
        NO_PRODUCER
    };

    Consumer (const char* name)
            : mFailState(NO_ERROR)
            , mStopServiceThreadFlag(false)
            , mName(name)
            , mConsumptionMutex(mName + "-")
            , mProductionMutex(mName + "+") {
        using namespace boost::interprocess;
        using std::swap;

        if (!message_queue::remove(name)) {
            LOG(debug) << "Unable to remove message queue " << mName;
        }

        try {
            mQueue.reset(new message_queue(create_only, name, N, sizeof(Msg)));
        }
        catch (interprocess_exception& exc) {
            throw QueueConstructionError(std::string("Unable to create queue named ") + mName);
        }

        scoped_lock<tmp_file_lock> consumptionLock { mConsumptionMutex };
        swap(mConsumptionLock, consumptionLock);

        LOG(debug) << "Consumer(" << mName << ") constructed";
    }

    ~Consumer () {
        stopServiceThread();
    }

    template <typename Duration>
    bool waitForProducer (Duration timeout) {
        LOG(debug) << "Waiting for producer ...";

        auto stopTime = std::chrono::steady_clock::now() + timeout;
        while (mProductionMutex.try_lock()) {
            LOG(debug) << "Got production lock :(";
            mProductionMutex.unlock();
            if (std::chrono::steady_clock::now() >= stopTime) {
                LOG(debug) << "Timed out waiting for producer";
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        LOG(debug) << "Producer connected!";
        return true;
    }

    template <typename Duration1, typename Duration2>
    void startServiceThread (std::function<void(Msg)> processMessage,
            Duration1 spawnProducerTimeout, Duration2 pollingTimeout) {
        mServiceThread = std::thread([=] () {
            serviceThread(processMessage, spawnProducerTimeout, pollingTimeout);
        });
    }

    void joinServiceThread () {
        mServiceThread.join();
    }

    void stopServiceThread () {
        if (mServiceThread.joinable()) {
            mStopServiceThreadFlag = true;
            joinServiceThread();
        }
    }

    FailState failState () const {
        return mFailState;
    }

private:
    template <typename Duration1, typename Duration2>
    void serviceThread (std::function<void(Msg)> processMessage,
            Duration1 spawnProducerTimeout, Duration2 pollingTimeout) {
        BOOST_SCOPE_EXIT(void) {
            LOG(debug) << "Exiting consumer service thread";
        } BOOST_SCOPE_EXIT_END

        LOG(debug) << "Consumer service thread started";

        if (!waitForProducer(spawnProducerTimeout)) {
            mFailState = NO_PRODUCER;
            return;
        }

        Msg msg;
        boost::interprocess::message_queue::size_type rx_size;
        unsigned int priority;
        while (!mProductionMutex.try_lock() && !mStopServiceThreadFlag) {
            auto stopTime = boost::posix_time::microsec_clock::universal_time() +
                stdChronoDurationToPosixTimeDuration(pollingTimeout);
            while (mQueue->timed_receive(&msg, sizeof(msg), rx_size, priority,
                        stopTime)) {
                LOG(debug) << "Consumer got msg with size " << rx_size
                           << "priority " << priority;
                processMessage(msg);
            }
        }

        if (!mStopServiceThreadFlag) {
            mFailState = NO_PRODUCER;
        }

        mProductionMutex.unlock();
    }

    std::atomic<FailState> mFailState;
    std::atomic<bool> mStopServiceThreadFlag;
    std::thread mServiceThread;

    std::string mName;
    std::unique_ptr<boost::interprocess::message_queue> mQueue;
    boost::interprocess::scoped_lock<tmp_file_lock> mConsumptionLock;
    tmp_file_lock mConsumptionMutex;
    tmp_file_lock mProductionMutex;
};

}

#endif
