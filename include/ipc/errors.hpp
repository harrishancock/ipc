#ifndef IPC_QUEUEERROR_HPP
#define IPC_QUEUEERROR_HPP

#include <exception>
#include <string>

namespace ipc {

class QueueError : std::exception {
public:
    explicit QueueError (std::string msg) : mMsg(msg) { }

    const char* what () {
        return mMsg.c_str();
    }

private:
    std::string mMsg;
};

class LockError : std::exception {
public:
    explicit LockError (std::string msg) : mMsg(msg) { }

    const char* what () {
        return mMsg.c_str();
    }

private:
    std::string mMsg;
};

}

#endif
