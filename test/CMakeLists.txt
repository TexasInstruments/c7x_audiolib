message(STATUS "                                                     ")
message(STATUS "-----------------------------------------------------")
message(STATUS "   Generating configs for testing ${KERNEL_NAME}     ")
message(STATUS "-----------------------------------------------------")
message(STATUS "                                                     ")

include(${CMAKE_SOURCE_DIR}/cmake/test.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/gen_test_cases.cmake)

if(AUTO_TEST)
  # TODO: complete dependency that can help with regression testing the library
  # file(GLOB SUBDIRS "${CMAKE_CURRENT_LIST_DIR}/*") message("sub dirs are
  # ${SUBDIRS}")

  # message("kernels_list: ${KERNEL_LIST}") foreach (kernel ${KERNEL_LIST})
  # message("kernel name: ${kernel}") endforeach ()

  file(GLOB SUBDIRS "${CMAKE_CURRENT_LIST_DIR}/*")

  foreach(subdir ${SUBDIRS})
    if(IS_DIRECTORY ${subdir})
      get_filename_component(nameOnly ${subdir} NAME)

      if(${nameOnly} STREQUAL "AUDIOLIB_idat_gen")
        continue()
      endif()

      # For A53 builds, auto-detect supported kernels: only include tests for
      # kernels whose source has an A53/ subdirectory, plus common and generated.
      if(DEVICE STREQUAL "a53ss0")
        if(NOT (${nameOnly} STREQUAL "common" OR ${nameOnly} STREQUAL "generated"
                OR IS_DIRECTORY "${CMAKE_SOURCE_DIR}/src/${nameOnly}/A53"))
          continue()
        endif()
      else()
        # generated/ contains A53-specific MCU SDK files — skip for non-A53
        if(${nameOnly} STREQUAL "generated")
          continue()
        endif()
      endif()

      message("Adding kernel name ${nameOnly}")
      add_subdirectory(${subdir})

    endif()
  endforeach()
else()
  add_subdirectory(common)
  if(DEVICE STREQUAL "a53ss0")
    include_directories(generated)
    add_subdirectory(generated)
  endif()
  add_subdirectory(${KERNEL_NAME})
endif()
