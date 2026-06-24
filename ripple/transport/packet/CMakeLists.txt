set(PROJECT ripple_transport_packet)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${PROJECT})
add_library(ripple::transport::packet ALIAS ${PROJECT})

target_sources(${PROJECT}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/controller.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/packet.cpp
)

target_include_directories(${PROJECT}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include/
)

target_link_libraries(${PROJECT} PUBLIC ripple::logger ripple::util::prng Boost::asio Boost::signals2)

install(TARGETS ${PROJECT} DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)
