set(PROJECT ripple_transport_quic)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${PROJECT})
set_target_properties(${PROJECT} PROPERTIES EXPORT_NAME transport_quic)
add_library(ripple::transport::quic ALIAS ${PROJECT})

target_compile_definitions(${PROJECT} PRIVATE QUIC_API_ENABLE_PREVIEW_FEATURES)

target_sources(${PROJECT}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/api.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/client.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/quic.cpp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/quic_handle.cpp>
)

target_include_directories(${PROJECT}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT} PUBLIC
    ripple::logger
    ripple::util::cert
    ripple::lib::msquic
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