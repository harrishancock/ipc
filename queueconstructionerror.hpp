#ifndef IPCQ_QUEUECONSTRUCTIONERROR_HPP
#define IPCQ_QUEUECONSTRUCTIONERROR_HPP

#include <exception>
#include <string>

namespace ipcq {

class QueueConstructionError : std::exception {
public:
    explicit QueueConstructionError (std::string msg) : mMsg(msg) { }

    const char* what () {
        return mMsg.c_str();
    }

private:
    std::string mMsg;
};

}

#endif
