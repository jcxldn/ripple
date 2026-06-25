set(PROJECT ripple_transport_stats)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${PROJECT} INTERFACE)
set_target_properties(${PROJECT} PROPERTIES EXPORT_NAME transport_stats)
add_library(ripple::transport::stats ALIAS ${PROJECT})

target_include_directories(${PROJECT}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

install(TARGETS ${PROJECT}
    EXPORT ripple-targets
)
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)