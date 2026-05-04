set(PROJECT ripple_transport_stats)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${PROJECT} INTERFACE)
add_library(ripple::transport::stats ALIAS ${PROJECT})

target_include_directories(${PROJECT}
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include/
)

install(TARGETS ${PROJECT} DESTINATION lib)
