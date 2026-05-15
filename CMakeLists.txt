cmake_minimum_required(VERSION 3.28)

project(rzstminfo)

include(FetchContent)

set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.44.0
    GIT_SHALLOW TRUE
)

FetchContent_Declare(mbedtls
    GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls.git
    GIT_TAG v3.6.6
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(
  httplib
  mbedtls
)

add_library(rzstminfo STATIC)
target_compile_features(rzstminfo PRIVATE cxx_std_23)
target_sources(rzstminfo PRIVATE
  src/SteamAccount.cpp
)
target_include_directories(rzstminfo 
PUBLIC
  include/
PRIVATE
  "${httplib_SOURCE_DIR}"
)
target_compile_definitions(rzstminfo PRIVATE CPPHTTPLIB_MBEDTLS_SUPPORT)
target_link_libraries(rzstminfo PRIVATE mbedtls)
