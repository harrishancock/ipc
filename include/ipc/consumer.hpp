#ifndef IPC_CONSUMER_HPP
#define IPC_CONSUMER_HPP

#include "common.hpp"
#include "tmp_file_lock.hpp"
#include "errors.hpp"

#include "util/log.hpp"
#include "util/std_chrono_duration_to_posix_time_duration.hpp"

#include <boost/scope_exit.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include <atomic>
#include <string>
#include <thread>

namespace ipc {

template <typename Msg, size_t N = 100>
class Consumer {
public:
    enum FailState {
        NO_ERROR,
        NO_PRODUCER
    };

    Consumer (const char* name)
            : mName(name)
            , mConsumptionMutex(mName + IPC_CONSUMER_SUFFIX)
            , mProductionMutex(mName + IPC_PRODUCER_SUFFIX) {
        using namespace boost::interprocess;
        using std::swap;

        if (!message_queue::remove(name)) {
            LOG(debug) << "Unable to remove message queue " << mName;
        }

        try {
            mQueue.reset(new message_queue(create_only, name, N, sizeof(Msg)));
        }
        catch (interprocess_exception& exc) {
            throw QueueError(std::string("Unable to create queue named ") + mName);
        }

        scoped_lock<tmp_file_lock> consumptionLock { mConsumptionMutex };
        swap(mConsumptionLock, consumptionLock);

        LOG(debug) << "Consumer(" << mName << ") constructed";
    }

    ~Consumer () {
        stopServiceThread();
    }

    /* Start a thread to service the queue. As messages come in, the service
     * thread will call the user-provided processMessage function with the
     * message as the sole argument (use a lambda or std::bind if you need to
     * use a non-unary function). If a producer never connects to the queue
     * after a specified interval (spawnProducerTimeout) the service thread is
     * stopped and the fail state is set to NO_PRODUCER. There is currently no
     * way to specify infinity as a wait time. If a producer appears within the
     * spawnProducerTimeout interval, then subsequently disappears, the service
     * thread is also stopped.
     *
     * Since we can only detect a producer process by checking to see if it has
     * acquired the queue's production lock, and there is no way to block
     * waiting for another process to acquire a lock, we must poll the mutex at
     * intervals to see if the producer has spawned. This interval is
     * pollingTimeout.
     *
     * pollingTimeout is also the interval at which the stop-thread flag is
     * checked. Therefore, pollingTimeout plus some small epsilon is the
     * maximum amount of time client code should have to wait for
     * stopServiceThread to execute.
     *
     * Since a producer process's lifetime might fit into the pollingTimeout
     * window, there is a very likely chance that we cannot detect short-lived
     * processes which open the queue, send some messages, then exit. For this
     * reason, the queue is checked continuously during the initial
     * spawnProducerTimeout interval, and if a message is received, the queue
     * is exhausted before checking the production mutex. This guarantees that
     * we will receive all messages from a short-lived process. */
    template <typename Duration1, typename Duration2>
    void startServiceThread (std::function<void(Msg)> processMessage,
            Duration1 spawnProducerTimeout, Duration2 pollingTimeout) {
        assert(!mServiceThread.joinable());
        LOG(debug) << "Consumer starting service thread";
        mServiceThread = std::thread([=] () {
            serviceThread(processMessage, spawnProducerTimeout, pollingTimeout);
        });
    }

    void joinServiceThread () {
        LOG(debug) << "Consumer joining service thread";
        if (mServiceThread.joinable()) {
            mServiceThread.join();
        }
    }

    /* Signal the service thread to exit gracefully. This function should block
     * for at most ~pollingTimeout duration, where pollingTimeout is the
     * argument to startServiceThread. */
    void stopServiceThread () {
        LOG(debug) << "Consumer stopping service thread";
        bool expected = false;
        if (mServiceThread.joinable() &&
                mStopServiceThreadFlag.compare_exchange_strong(expected, true)) {
            joinServiceThread();
        }
    }

    template <typename Rep, typename Period>
    bool timedReceiveAndProcess (std::chrono::duration<Rep, Period> timeout,
            std::function<void(Msg)> processMessage) {
        Msg message;

        /* Things we have to receive because we're using Boost.Interprocess
         * message_queues, but don't care about. */
        boost::interprocess::message_queue::size_type nReceivedBytes;
        unsigned int priority;

        auto stopTime = boost::posix_time::microsec_clock::universal_time() +
            stdChronoDurationToPosixTimeDuration(timeout);
        if (mQueue->timed_receive(&message, sizeof(message), nReceivedBytes, priority, stopTime)) {
            LOG(debug) << "Consumer got msg with size " << nReceivedBytes
                       << ", priority " << priority;
            processMessage(message);
            return true;
        }
        else {
            return false;
        }
    }

    FailState failState () const {
        return mFailState;
    }

private:
    /* XXX This is important: if you go into a loop in this function which
     * might block for a while (more than a few milliseconds), consider
     * checking mStopServiceThreadFlag on every test of the conditional.
     *
     * XXX This is also important: if you change the implementation here,
     * update the comments above startServiceThread. */
    template <typename R1, typename P1, typename R2, typename P2>
    void serviceThread (std::function<void(Msg)> processMessage,
            std::chrono::duration<R1, P1> spawnProducerTimeout,
            std::chrono::duration<R2, P2> pollingTimeout) {
        BOOST_SCOPE_EXIT(void) {
            LOG(debug) << "Exiting consumer service thread";
        } BOOST_SCOPE_EXIT_END

        LOG(debug) << "Consumer service thread started";

        {
            /* Wait for and process the first burst of messages, or until
             * spawnProducerTimeout elapses. */
            auto stopTime = std::chrono::steady_clock::now() + spawnProducerTimeout;
            bool gotMessage = false;
            while (!mStopServiceThreadFlag &&
                    std::chrono::steady_clock::now() < stopTime &&
                    !gotMessage) {
                gotMessage = timedReceiveAndProcess(pollingTimeout, processMessage);
            }

            if (gotMessage) {
                while (!mStopServiceThreadFlag &&
                        timedReceiveAndProcess(pollingTimeout, processMessage))
                    ;
            }
        }

        while (!mStopServiceThreadFlag) {
            if (mProductionMutex.try_lock()) {
                mProductionMutex.unlock();
                mFailState = NO_PRODUCER;
                LOG(debug) << "No producer present";
                break;
            }

            while (!mStopServiceThreadFlag &&
                    timedReceiveAndProcess(pollingTimeout, processMessage))
                ;
        }

        /* Who knows, we might need to be restarted. */
        mStopServiceThreadFlag = false;
    }

    std::atomic<FailState> mFailState = { NO_ERROR };
    std::atomic<bool> mStopServiceThreadFlag = { false } ;
    std::thread mServiceThread;

    std::string mName;
    std::unique_ptr<boost::interprocess::message_queue> mQueue = nullptr;
    boost::interprocess::scoped_lock<tmp_file_lock> mConsumptionLock;
    tmp_file_lock mConsumptionMutex;
    tmp_file_lock mProductionMutex;
};

}

#endif
