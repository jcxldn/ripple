set(PROJECT ripple_discovery)
set(DISCOVERY_LIB ${PROJECT}_lib)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${DISCOVERY_LIB})
add_executable(${PROJECT})
add_executable(ripple::discovery ALIAS ${PROJECT})

target_sources(${DISCOVERY_LIB}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/discovery.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/node.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/peer_manager.cpp
)

target_sources(${PROJECT}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
)

target_include_directories(${DISCOVERY_LIB}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include/
)


target_link_libraries(${DISCOVERY_LIB} PUBLIC 
    ripple::peer
    ripple::util::cert
    ripple::logger
    ripple::serialization
    ripple::transport::multicast
    ripple::transport::quic
    Boost::signals2
)

target_link_libraries(${PROJECT} PRIVATE ${DISCOVERY_LIB})

if (BUILD_TESTING)
  add_executable(${PROJECT}_integration
      ${CMAKE_CURRENT_LIST_DIR}/src/integration_main.cpp)
  target_link_libraries(${PROJECT}_integration PRIVATE ${DISCOVERY_LIB})
  add_test(NAME discovery_two_node_integration COMMAND ${PROJECT}_integration)
  set_tests_properties(discovery_two_node_integration PROPERTIES TIMEOUT 15)
endif()

install(TARGETS ${PROJECT} DESTINATION lib)
