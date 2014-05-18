#ifndef STD_CHRONO_DURATION_TO_POSIX_TIME_DURATION_HPP
#define STD_CHRONO_DURATION_TO_POSIX_TIME_DURATION_HPP

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <chrono>

template <typename Rep, typename Period>
boost::posix_time::time_duration stdChronoDurationToPosixTimeDuration (const std::chrono::duration<Rep, Period>& from) {
    using duration_t = std::chrono::nanoseconds;
    using rep_t = duration_t::rep;
    rep_t d = std::chrono::duration_cast<duration_t>(from).count();
    rep_t sec = d / 1000000000;
    rep_t nsec = d % 1000000000;
    return boost::posix_time::seconds(static_cast<long long>(sec)) +
#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
        boost::posix_time::nanoseconds(nsec);
#else
        boost::posix_time::microseconds((nsec+(nsec>0?500:-500))/1000);
#endif
}

#endif
