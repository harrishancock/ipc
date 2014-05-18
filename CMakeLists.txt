cmake_minimum_required(VERSION 2.8.12)
project(boost-log-test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11")

##############################################################################
# Boost

# FindBoost.cmake uses BOOST_ROOT as a hint for where to look.
if(NOT DEFINED BOOST_ROOT)
    if(DEFINED ENV{BOOST_ROOT})
        set(BOOST_ROOT "$ENV{BOOST_ROOT}")
    else()
        message(WARNING "BOOST_ROOT is undefined.\nIf Boost libraries fail to "
            "be found, try setting -DBOOST_ROOT=/path/to/boost on the CMake "
            "command line.\n")
    endif()
endif()

# Linking statically to Boost is easier for prototyping. For multi-module
# projects we'll want to use shared libraries and defined -DBOOST_ALL_DYN_LINK
# or the like.
set(Boost_USE_STATIC_LIBS ON)

# Boost.Log was introduced in Boost version 1.54.0.
find_package(Boost 1.54.0 REQUIRED COMPONENTS thread system log filesystem)

include_directories(${Boost_INCLUDE_DIRS})

##############################################################################
# Targets

include_directories(include)

add_executable(consumer consumer-main.cpp)
add_executable(producer producer-main.cpp)
add_executable(sharedproducer sharedproducer-main.cpp)

set(LIBS ${Boost_LOG_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY} ${Boost_SYSTEM_LIBRARY})

if(UNIX)
    list(APPEND LIBS rt)
endif()

target_link_libraries(consumer ${LIBS})
target_link_libraries(producer ${LIBS})
target_link_libraries(sharedproducer ${LIBS})
