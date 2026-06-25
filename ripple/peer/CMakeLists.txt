set(PROJECT ripple_peer)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${PROJECT})
set_target_properties(${PROJECT} PROPERTIES EXPORT_NAME peer)
add_library(ripple::peer ALIAS ${PROJECT})

target_sources(${PROJECT}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/discovery.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/peer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/quic.cpp
)

target_include_directories(${PROJECT}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT}
    PUBLIC
        ripple::router
        ripple::discovery
    ripple::transport::multicast
    ripple::transport::quic
)

install(TARGETS ${PROJECT}
    EXPORT ripple-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)