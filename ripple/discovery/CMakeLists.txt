set(PROJECT ripple_discovery)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${PROJECT})
set_target_properties(${PROJECT} PROPERTIES EXPORT_NAME discovery)
add_library(ripple::discovery ALIAS ${PROJECT})

target_sources(${PROJECT}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/discovery.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/node.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/peer_manager.cpp
)

target_include_directories(${PROJECT}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT} PUBLIC
    ripple::router
    ripple::util::cert
    ripple::logger
    ripple::serialization
    ripple::transport::multicast
    ripple::transport::quic
    Boost::signals2
)

if (RIPPLE_BUILD_TEST)
  add_executable(${PROJECT}_integration
      ${CMAKE_CURRENT_LIST_DIR}/src/integration_main.cpp)
  target_link_libraries(${PROJECT}_integration PRIVATE ${PROJECT})
  add_test(NAME discovery_two_node_integration COMMAND ${PROJECT}_integration)
  set_tests_properties(discovery_two_node_integration PROPERTIES TIMEOUT 15)
endif()

install(TARGETS ${PROJECT}
    EXPORT ripple-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)