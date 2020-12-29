if (RPI)
    add_subdirectory(bcm2835)
endif ()
add_subdirectory(curlpp)

# don't run tests
set(JSON_BuildTests OFF CACHE INTERNAL "")
# don't install
set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory(json)
