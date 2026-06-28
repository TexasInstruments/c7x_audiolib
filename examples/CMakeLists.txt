message(STATUS "------------------------------------------")
message(STATUS "   Building example for ${KERNEL_NAME}    ")
message(STATUS "------------------------------------------")

include(${CMAKE_SOURCE_DIR}/cmake/examples.cmake)
add_subdirectory(${KERNEL_NAME})

if(DEVICE STREQUAL "a53ss0")
include_directories(generated)
add_subdirectory(generated)
endif()
