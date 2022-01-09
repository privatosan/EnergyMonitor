if (RPI)
    add_subdirectory(bcm2835)
endif ()
add_subdirectory(curlpp)

# don't run tests
set(JSON_BuildTests OFF CACHE INTERNAL "")
# don't install
set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory(json)

add_subdirectory(websocketpp)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include"
)
