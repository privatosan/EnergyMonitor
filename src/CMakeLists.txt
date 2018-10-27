cmake_minimum_required(VERSION 2.8)

project(energyMeter)

set(BINARY_NAME energyMeter)

set(SOURCES
    Main.cpp
    Post.cpp
    Solar.cpp
    Util.cpp
)

# include for headers
include_directories(
    ${CMAKE_SOURCE_DIR}/ext/bcm2835/include
    ${CMAKE_SOURCE_DIR}/ext/curlpp/include
    )

add_executable(${BINARY_NAME} ${SOURCES})

if (RPI)
    target_link_libraries(${BINARY_NAME} bcm2835)
endif ()
target_link_libraries(${BINARY_NAME}
    curlpp
    pthread
    )

