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
        ${CMAKE_CURRENT_LIST_DIR}/src/discovery.cpp
)

target_include_directories(${PROJECT}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include/
)


target_link_libraries(${PROJECT} PUBLIC ripple::serialization ripple::transport::multicast)

install(TARGETS ${PROJECT} DESTINATION lib)
