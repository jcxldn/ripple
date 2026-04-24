set(PROJECT ripple_discovery)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_executable(${PROJECT})
add_executable(ripple::discovery ALIAS ${PROJECT})


target_sources(${PROJECT}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/discovery.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/node.cpp
)

target_include_directories(${PROJECT}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include/
)


target_link_libraries(${PROJECT} PUBLIC 
    ripple::util::cert
    ripple::logger
    ripple::serialization
    ripple::transport::multicast
    ripple::transport::quic
)

install(TARGETS ${PROJECT} DESTINATION lib)
