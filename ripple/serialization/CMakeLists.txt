set(PROJECT ripple_serialization)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${PROJECT} INTERFACE)
add_library(ripple::serialization ALIAS ${PROJECT})


target_include_directories(${PROJECT}
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include/
)

target_link_libraries(${PROJECT} INTERFACE ripple::logger cereal::cereal)

install(TARGETS ${PROJECT} DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)