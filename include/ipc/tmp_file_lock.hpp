#ifndef TMP_FILE_LOCK_HPP
#define TMP_FILE_LOCK_HPP

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/filesystem.hpp>

#include <fstream>
#include <string>
#include <utility>

/* TODO remove lock files when they are no longer needed. */
class tmp_file_lock {
public:
    tmp_file_lock (std::string name) {
        auto filename = boost::filesystem::temp_directory_path();
        filename /= name;

        /* Touch the file. */
        std::ofstream(filename.string().c_str()).flush();

        /* FIXME Race condition here. See
         * http://stackoverflow.com/questions/17708885/flock-removing-locked-file-without-race-condition
         * for the solution. */

        boost::interprocess::file_lock flock { filename.string().c_str() };

        using std::swap;
        swap(mFlock, flock);
    }

    void lock () {
        mFlock.lock();
    }

    bool try_lock () {
        return mFlock.try_lock();
    }

    bool timed_lock (const boost::posix_time::ptime& abs_time) {
        return mFlock.timed_lock(abs_time);
    }

    void unlock () {
        mFlock.unlock();
    }

    void lock_sharable () {
        mFlock.lock_sharable();
    }

    bool try_lock_sharable () {
        return mFlock.try_lock_sharable();
    }

    bool timed_lock_sharable (const boost::posix_time::ptime& abs_time) {
        return mFlock.timed_lock_sharable(abs_time);
    }

    void unlock_sharable () {
        mFlock.unlock_sharable();
    }

private:
    boost::interprocess::file_lock mFlock;
};

#endif
