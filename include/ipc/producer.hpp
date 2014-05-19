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
#include <type_traits>

namespace ipc {

/* Provide a write-only interface to a named interprocess queue. Use lock files
 * to synchronize access to this queue. Note that this implies that a
 * BasicProducer cannot communicate with another user's Consumer object, due to
 * file permissions. In order to support this use case, a more nuanced
 * tmp_file_lock, and potentially a special installation process of the
 * compiled binaries, would be required. Think about using DBus or COM before
 * going down that path.
 *
 * The queue transports standard layout objects (i.e., they would have the
 * same memory layout if compiled in C) of type Msg. This is enforced with a
 * static_assert. Do not attempt to pass pointers, or objects containing
 * pointers, across process boundaries--the memory addresses they point to will
 * no longer be valid.
 *
 * Lock is the template which will be used to get a production lock. Specify
 * boost::interprocess::scoped_lock to instantiate an exclusive producer, or
 * boost::interprocess::sharable_lock to instantiate a shared producer.
 *
 * An exclusive producer ensures that it is the only one with (advisory) write
 * access to the message queue. No other producer object may write to the
 * message queue during the lifetime of an object of type Producer.
 *
 * A shared producer may be one of many writers to the message queue.
 *
 * Behavior is undefined if two producer objects (exclusive or shared)
 * referring to the same message queue are instantiated in the same process.
 */
template <typename Msg, template <typename> class Lock>
class BasicProducer {
    static_assert(std::is_standard_layout<Msg>::value,
            "message type must be a standard layout class");
public:
    /* Obtain a production lock on the queue named name. This lock will be
     * shared or unique depending on the semantics of Lock.
     *
     * Before the queue may be used, waitForConsumer must be called. The
     * constructor does not call waitForConsumer in order to allow the client
     * code more flexibility in synchronizing with remote processes. As long as
     * a producer object is alive, a consumer object will know the queue is
     * active, and will not report a failure on its end. We can therefore
     * instantiate a producer object, then instantiate a consumer process,
     * without fear that the consumer might give up waiting for us if it has a
     * short timeout.
     *
     * Throws FileLockError if there is a problem with the lock files which are
     * used to synchronize the queue. There is no realistic recovery in this
     * situation, though using another name for the queue may work. */
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

    /* By convention, the consumer process is the one to prepare the underlying
     * message queue at the operating system level. It signals its completion
     * of this task by obtaining a consumption lock, at which point it is safe
     * for the producer process (us) to open the message queue. waitForConsumer
     * waits for the consumer queue's lock, then opens the message queue.
     *
     * This function may be called multiple times. For instance, if a consumer
     * process crashes, the send function will throw a NoConsumer exception.
     * Client code could then attempt to restart the consumer process, call
     * waitForConsumer, and reattempt the send, without having to destroy and
     * reinstantiate a producer object.
     *
     * Returns false if the specified timeout elapses while waiting for the
     * consumer process.
     *
     * Throws QueueError if there is an error opening the queue, indicating an
     * inconsistent state on the consumer end of things. */
    template <typename Rep, typename Period>
    bool waitForConsumer (std::chrono::duration<Rep, Period> timeout) {
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

    /* Send a message to the consumer endpoint. This function may ONLY be
     * called after waitForConsumer, otherwise a null pointer will be
     * dereferenced. The reason send does not call waitForConsumer implicitly
     * is because we do not know how long the client code is willing to wait
     * for the consumer to appear, nor is it intuitive for us to provide a
     * timeout argument to that effect in the send interface.
     *
     * Throws NoConsumer if the consumer process no longer has the consumption
     * lock on the message queue, which signifies that the other end has hung
     * up.
     *
     * Throws QueueError if there is an internal error sending, which might
     * reflect two consumer process stomping on each other, or an inconsistent
     * state on the other end. */
    void send (Msg msg) {
        assert(mQueue);

        if (mConsumptionMutex.try_lock()) {
            mConsumptionMutex.unlock();
            throw NoConsumer();
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

/* User-friendly aliases for the two types of producers. */

template <typename Msg>
using Producer = BasicProducer<Msg, boost::interprocess::scoped_lock>;

template <typename Msg>
using SharedProducer = BasicProducer<Msg, boost::interprocess::sharable_lock>;

}

#endif
