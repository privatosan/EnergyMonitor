cmake_minimum_required(VERSION 2.8)

# default to release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

find_package(CURL REQUIRED)
find_package(websocketpp REQUIRED CONFIG)

project(energyMeter)

set(BINARY_NAME energyMeter)

set(SOURCES
    BackgroundTask.cpp
    Log.cpp
    Main.cpp
    Options.cpp
    Post.cpp
    Power.cpp
    Server.cpp
    Settings.cpp
    SignalHandler.cpp
    Solar.cpp
    SolarMax.cpp
    Util.cpp
)

# include for headers
include_directories(
    ${CMAKE_SOURCE_DIR}/ext/bcm2835/include
    ${CMAKE_SOURCE_DIR}/ext/curlpp/include
    )

add_executable(${BINARY_NAME} ${SOURCES})

if (RPI)
    message(STATUS "Building for RPI")
    target_compile_definitions(${BINARY_NAME} PRIVATE RPI=1)
    if (BCM2835)
        message(STATUS "Using BCM2835 lib")
        target_compile_definitions(${BINARY_NAME} PRIVATE BCM2835=1)
        target_link_libraries(${BINARY_NAME} bcm2835)
    endif ()
    if (WIRINGPI)
        message(STATUS "Using WiringPi")
        target_compile_definitions(${BINARY_NAME} PRIVATE WIRINGPI=1)
        target_link_libraries(${BINARY_NAME} wiringPi)
    endif ()
endif ()
target_link_libraries(${BINARY_NAME}
    ${CURL_LIBRARIES}
    curlpp
    pthread
    nlohmann_json::nlohmann_json
    websocketpp::websocketpp
    asio
    )
