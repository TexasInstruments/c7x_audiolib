function(AUDIOLIB_create_example_binary_name myKernelName)
  if(TARGET_PLATFORM STREQUAL "PC")
    set(exeName
        "example_${myKernelName}_${DEVICE}_x86_64"
        PARENT_SCOPE)
  else()
    set(exeName
        "example_${myKernelName}_${DEVICE}"
        PARENT_SCOPE)
  endif()
endfunction()

function(AUDIOLIB_create_binary_for_example_kernel)

  file(GLOB_RECURSE SRC_FILES "*.cpp" "*.h")
  # message("source test files are: ${SRC_FILES}")
  get_filename_component(myKernelName "${CMAKE_CURRENT_LIST_DIR}" NAME)
  audiolib_create_example_binary_name(${myKernelName})
  add_executable(${exeName} ${SRC_FILES})
  audiolib_set_bin_extension(${exeName})

  if(DEVICE STREQUAL "a53ss0")
    target_link_libraries(
      ${exeName} PRIVATE "${myKernelName}_obj" "${extObjNames}" AUDIOLIB_common
                          AUDIOLIB_test_generated AUDIOLIB_common ${MCU_SDK_LIBS})
  else()
    # add dsplib dependencies
    if(NOT DEFINED ENV{DSPLIB_C7X_ROOT})
      add_dependencies(${exeName} dsplib)
    endif()
    target_include_directories(${exeName} PRIVATE
      $ENV{DSPLIB_C7X_ROOT}/src
    )
    target_link_libraries(${exeName} PRIVATE AUDIOLIB AUDIOLIB_common ${DSPLIB_C7X} ${C7x_LIBS})
  endif()

  if(TARGET_PLATFORM STREQUAL "PC")
    if(CMAKE_HOST_UNIX)
      target_link_options(${exeName} PRIVATE -coverage -lgcov)
    endif()
  endif()
  
  if(NOT TARGET_PLATFORM STREQUAL "PC")
    if(DEVICE STREQUAL "a53ss0")
      AUDIOLIB_set_linker_options_A53(${exeName})
    else()
    audiolib_set_linker_options_c7x(${exeName})
    endif()
  endif()

  audiolib_finish_compilation(${exeName})

endfunction()
