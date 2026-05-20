if(RIPPLE_USE_EXTERNAL_MSQUIC)
  find_package(msquic REQUIRED)
  find_package(OpenSSL REQUIRED)

else()

  set(QUIC_TLS_LIB "quictls" CACHE STRING "use quictls (openssl)" FORCE)

  FetchContent_Declare(
    msquic
    GIT_REPOSITORY https://github.com/microsoft/msquic.git
    GIT_TAG        v2.5.7
  )

  FetchContent_MakeAvailable(msquic)
endif()

# msquic doesn't export inc by default
add_library(ripple_lib_msquic INTERFACE)
add_library(ripple::lib::msquic ALIAS ripple_lib_msquic)

target_include_directories(ripple_lib_msquic INTERFACE ${msquic_SOURCE_DIR}/src/inc)
target_link_libraries(ripple_lib_msquic INTERFACE msquic)


add_library(ripple_lib_quictls INTERFACE)
add_library(ripple::lib::quictls ALIAS ripple_lib_quictls)

target_link_libraries(ripple_lib_quictls INTERFACE OpenSSL)