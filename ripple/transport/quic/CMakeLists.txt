set(PROJECT ripple_transport_quic)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${PROJECT})
add_library(ripple::transport::quic ALIAS ${PROJECT})

target_sources(${PROJECT}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/api.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/client.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/quic.cpp
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/quic_handle.cpp
)

target_include_directories(${PROJECT}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include/
)

target_link_libraries(${PROJECT} PUBLIC 
    ripple::logger 
    ripple::util::cert 
    ripple::lib::msquic
    ripple::transport::packet
)

install(TARGETS ${PROJECT} DESTINATION lib)
