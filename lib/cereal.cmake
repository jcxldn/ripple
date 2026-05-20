if(RIPPLE_USE_EXTERNAL_CEREAL)
  find_package(cereal REQUIRED)
else()
  FetchContent_Declare(
    cereal
    GIT_REPOSITORY https://github.com/USCiLab/cereal
    GIT_TAG        d81e2f7df7b334fee057e53017388d02e555a836 # Mar 6, 2026 (master branch)
  )

  set(BUILD_SANDBOX OFF) # cereal: build sandbox examples
  set(BUILD_DOC OFF) # cereal: build documentation
  FetchContent_MakeAvailable(cereal)
endif()

