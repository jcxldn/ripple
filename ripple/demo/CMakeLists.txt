set(PROJECT ripple_demo)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_executable(${PROJECT})
add_executable(ripple::demo ALIAS ${PROJECT})

target_sources(${PROJECT}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
)

target_include_directories(${PROJECT}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include/
)

target_link_libraries(${PROJECT}
    PUBLIC ripple::peer
)

install(TARGETS ${PROJECT} DESTINATION lib)
