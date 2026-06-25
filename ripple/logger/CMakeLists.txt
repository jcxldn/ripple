set(PROJECT ripple_logging)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${PROJECT})
set_target_properties(${PROJECT} PROPERTIES EXPORT_NAME logger)
add_library(ripple::logger ALIAS ${PROJECT})


target_sources(${PROJECT}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/logger.cpp
)

target_include_directories(${PROJECT}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT} PUBLIC
    $<BUILD_INTERFACE:spdlog::spdlog>
    $<$<BOOL:${RIPPLE_USE_EXTERNAL_SPDLOG}>:$<INSTALL_INTERFACE:spdlog::spdlog>>
)

if(RIPPLE_USE_EXTERNAL_SPDLOG)
    target_compile_definitions(${PROJECT} PUBLIC SPDLOG_FMT_EXTERNAL)
    target_link_libraries(${PROJECT} PUBLIC
        $<BUILD_INTERFACE:fmt::fmt>
        $<INSTALL_INTERFACE:fmt::fmt>
    )
endif()

install(TARGETS ${PROJECT}
    EXPORT ripple-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)