set(PROJECT ripple_logging)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${PROJECT})

add_library(ripple::logger ALIAS ${PROJECT})


target_sources(${PROJECT}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/logger.cpp
)

target_include_directories(${PROJECT}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include/
)

target_link_libraries(${PROJECT} PUBLIC spdlog::spdlog)

if(RIPPLE_USE_EXTERNAL_SPDLOG)
    target_compile_definitions(${PROJECT} PUBLIC SPDLOG_FMT_EXTERNAL)
    target_link_libraries(${PROJECT} PUBLIC fmt::fmt)
endif()

install(TARGETS ${PROJECT} DESTINATION lib)