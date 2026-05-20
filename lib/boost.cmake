# Boost 1.80+ required for boost::asio::cancellation_signal
find_package(Boost 1.80.0 COMPONENTS filesystem) # asio, signals2 are header-only


if(Boost_FOUND)
  message("Using system boost ${Boost_VERSION_STRING}")

  # fake boost
  add_library(boost_iface INTERFACE)
  target_link_libraries(boost_iface INTERFACE Boost::headers)

  add_library(Boost::asio ALIAS boost_iface)
  add_library(Boost::signals2 ALIAS boost_iface)
elseif(NOT RIPPLE_USE_EXTERNAL_BOOST)
  message("Boost not found, downloading from git...")
  FetchContent_Declare(
    Boost
    GIT_REPOSITORY https://github.com/boostorg/boost
    GIT_TAG        1bed2b0712b2119f20d66c5053def9173c8462a5 # boost-1.90.0
  )

  FetchContent_MakeAvailable(Boost)
endif()