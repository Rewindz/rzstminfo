cmake_minimum_required(VERSION 3.28)

project(rzstminfo)

include(FetchContent)

set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.44.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(
  httplib
)

add_library(rzstminfo STATIC)
target_compile_features(rzstminfo PRIVATE cxx_std_23)
target_sources(rzstminfo PUBLIC
  src/SteamAccount.cpp
)
target_include_directories(rzstminfo 
PUBLIC
  include/
PRIVATE
  "${httplib_SOURCE_DIR}"
)
