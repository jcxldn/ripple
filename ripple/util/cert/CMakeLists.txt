set(PROJECT ripple_util_cert)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${PROJECT})
add_library(ripple::util::cert ALIAS ${PROJECT})

target_sources(${PROJECT}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/cert.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/common.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/ed25519.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/identity.cpp
)

target_include_directories(${PROJECT}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include/
)


find_package(OpenSSL REQUIRED)


target_link_libraries(${PROJECT} PUBLIC ripple::util::prng OpenSSL)

install(TARGETS ${PROJECT} DESTINATION lib)
