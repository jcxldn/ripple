cmake_minimum_required(VERSION 3.14...3.31 FATAL_ERROR) # FetchContent

set(CMAKE_POLICY_VERSION_MINIMUM 3.5) # yaml-cpp on modern CMake

project(
    ripple
    VERSION 1.0.1
    DESCRIPTION "networking library"
    LANGUAGES CXX
)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17) # msquic, boost
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)


option(RIPPLE_USE_EXTERNAL_BOOST "Use external boost library instead of bundled" OFF)
option(RIPPLE_USE_EXTERNAL_CEREAL "Use external cereal library instead of bundled" OFF)
option(RIPPLE_USE_EXTERNAL_SPDLOG "Use external spdlog library instead of bundled" OFF)
option(RIPPLE_USE_EXTERNAL_MSQUIC "Use external msquic library instead of bundled" OFF)

option(RIPPLE_BUILD_SHARED "Compile ripple as a shared library" ${BUILD_SHARED_LIBS})
option(RIPPLE_BUILD_TEST "Compile the ripple test suite" OFF)
option(RIPPLE_BUILD_DEMO "Compile the ripple demo application" OFF)


if((DEFINED PROJECT_IS_TOP_LEVEL AND PROJECT_IS_TOP_LEVEL) OR (NOT DEFINED PROJECT_IS_TOP_LEVEL
                                                                   AND CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR))

    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        # https://github.com/gabime/spdlog/blob/2e71fdf3c1e8d6299b8129e402ab705e6fb494ba/CMakeLists.txt#L21-L27
        # Set CMAKE_BUILD_TYPE only if this project is top-level and it is not already set
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
    endif()
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Option logic
if (RIPPLE_BUILD_SHARED)
    set(BUILD_SHARED_LIBS ON)
    set(QUIC_BUILD_SHARED ON CACHE BOOL "Build msquic as a dynamic library" FORCE)
else()
    set(BUILD_SHARED_LIBS OFF)
    set(QUIC_BUILD_SHARED OFF CACHE BOOL "Build msquic as a dynamic library" FORCE)

    # msquic only searches for libnuma by so, make it search (by .a) when building statically
    set(NUMA "${NUMA_LIB}" CACHE FILEPATH "Forced static numa for MsQuic" FORCE)
endif()


include(FetchContent)

if(RIPPLE_BUILD_TEST)
    include(CTest)
endif()

# lib/*.cmake include checks for RIPPLE_USE_EXTERNAL_*
include(lib/boost.cmake)
include(lib/cereal.cmake)
include(lib/spdlog.cmake)
include(lib/msquic.cmake)


add_subdirectory(ripple/logger)
add_subdirectory(ripple/serialization)
add_subdirectory(ripple/util/prng)
add_subdirectory(ripple/util/cert)
add_subdirectory(ripple/transport/stats)
add_subdirectory(ripple/transport/packet)
add_subdirectory(ripple/transport/multicast)
add_subdirectory(ripple/transport/quic)
add_subdirectory(ripple/router)
add_subdirectory(ripple/peer)

add_subdirectory(ripple/discovery)

if(RIPPLE_BUILD_DEMO)
    add_subdirectory(ripple/demo)
endif()