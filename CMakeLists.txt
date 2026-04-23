cmake_minimum_required(VERSION 3.14...3.31 FATAL_ERROR) # FetchContent

set(CMAKE_POLICY_VERSION_MINIMUM 3.5) # yaml-cpp on modern CMake

project(
    ripple
    VERSION 0.0.1
    DESCRIPTION "meta-project"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17) # boost

set(CMAKE_BUILD_TYPE Release CACHE STRING "build type" FORCE) 

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS ON)

include(FetchContent)
include(lib/boost.cmake)
include(lib/cereal.cmake)
include(lib/spdlog.cmake)

add_subdirectory(ripple/logger)
add_subdirectory(ripple/serialization)
add_subdirectory(ripple/util/prng)
add_subdirectory(ripple/util/cert)
add_subdirectory(ripple/transport/packet)
add_subdirectory(ripple/transport/multicast)

add_subdirectory(ripple/discovery)