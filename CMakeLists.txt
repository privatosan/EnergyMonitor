cmake_minimum_required(VERSION 2.8)

set(WIRINGPI OFF CACHE STRING "Use WiringPi lib")
set(BCM2835 ON CACHE STRING "Use BCM 2835 lib")
set(RPI ON CACHE STRING "Build for Raspberry PI")

# default to release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    list(APPEND CMAKE_CXX_FLAGS "-std=c++11")
else()
    message(FATAL_ERROR "Compiler $(CMAKE_CXX_COMPILER) has no C++11 support.")
endif()

add_subdirectory(ext)
add_subdirectory(src)
