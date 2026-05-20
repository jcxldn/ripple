if(RIPPLE_USE_EXTERNAL_SPDLOG)
  find_package(spdlog REQUIRED)
else()
  FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG        79524ddd08a4ec981b7fea76afd08ee05f83755d # v1.17.0
  )

  FetchContent_MakeAvailable(spdlog)
endif()