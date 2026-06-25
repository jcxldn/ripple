include(CMakeFindDependencyMacro)

find_dependency(spdlog REQUIRED)
find_dependency(fmt REQUIRED)
find_dependency(cereal REQUIRED)
find_dependency(Boost 1.80 REQUIRED)
find_dependency(OpenSSL REQUIRED)
find_dependency(msquic REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/rippleTargets.cmake")
