set(PROJECT ripple_router)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${PROJECT})
set_target_properties(${PROJECT} PROPERTIES EXPORT_NAME router)
add_library(ripple::router ALIAS ${PROJECT})

target_sources(${PROJECT}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/router.cpp
)

target_include_directories(${PROJECT}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT}
    PUBLIC
        ripple::transport::packet
        ripple::transport::stats
)

install(TARGETS ${PROJECT}
    EXPORT ripple-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)