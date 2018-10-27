cmake_minimum_required(VERSION 2.8)

project(bcm2835)

set(BINARY_NAME bcm2835)

set(SOURCES
    bcm2835.c
)

# include for headers
include_directories(
    )

add_library(${BINARY_NAME} STATIC ${SOURCES})
