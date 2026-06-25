set(PROJECT ripple_util_cert)

project(
    ${PROJECT}
    VERSION ${CMAKE_PROJECT_VERSION}
    LANGUAGES CXX
)

add_library(${PROJECT})
set_target_properties(${PROJECT} PROPERTIES EXPORT_NAME util_cert)
add_library(ripple::util::cert ALIAS ${PROJECT})

target_sources(${PROJECT}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/cert.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/common.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/ed25519.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/identity.cpp
)

target_include_directories(${PROJECT}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT} PUBLIC ripple::util::prng ripple::lib::quictls)

install(TARGETS ${PROJECT}
    EXPORT ripple-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)