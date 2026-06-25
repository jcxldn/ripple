set(PROJECT ripple_serialization)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${PROJECT} INTERFACE)
set_target_properties(${PROJECT} PROPERTIES EXPORT_NAME serialization)
add_library(ripple::serialization ALIAS ${PROJECT})


target_include_directories(${PROJECT}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT} INTERFACE
    ripple::logger
    $<BUILD_INTERFACE:cereal::cereal>
    $<$<BOOL:${RIPPLE_USE_EXTERNAL_CEREAL}>:$<INSTALL_INTERFACE:cereal::cereal>>
)

install(TARGETS ${PROJECT}
    EXPORT ripple-targets
)
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)