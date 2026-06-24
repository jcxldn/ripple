set(PROJECT ripple_util_prng)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${PROJECT} INTERFACE)
add_library(ripple::util::prng ALIAS ${PROJECT})

target_include_directories(${PROJECT}
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include/
)

install(TARGETS ${PROJECT} DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)