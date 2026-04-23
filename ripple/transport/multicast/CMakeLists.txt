set(PROJECT ripple_transport_multicast)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${PROJECT})
add_library(ripple::transport::multicast ALIAS ${PROJECT})

target_sources(${PROJECT}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/address.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mcast.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/tx_pair.cpp
)

target_include_directories(${PROJECT}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include/
)

target_link_libraries(${PROJECT} PUBLIC ripple::logger ripple::transport::packet Boost::asio Boost::signals2)

install(TARGETS ${PROJECT} DESTINATION lib)
