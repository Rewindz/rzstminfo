cmake_minimum_required(VERSION 3.28)

project(rzstminfo)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

FetchContent_Declare(valvevdf
  GIT_REPOSITORY https://github.com/TinyTinni/ValveFileVDF.git
  GIT_TAG v1.1.1
  GIT_SHALLOW TRUE
)

FetchContent_GetProperties(valvevdf)
if(NOT valvevdf_POPULATED)
  FetchContent_Populate(valvevdf)
endif()

add_library(valvevdflib INTERFACE)
target_include_directories(valvevdflib INTERFACE ${valvevdf_SOURCE_DIR}/include)
  
add_library(rzstminfo STATIC)
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
target_link_libraries(rzstminfo PRIVATE mbedtls valvevdflib)
