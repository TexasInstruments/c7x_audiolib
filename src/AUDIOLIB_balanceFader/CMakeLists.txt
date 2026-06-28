if(NOT BUILD_LIB)
  set(extKernelName "AUDIOLIB_gainNCh")
  add_subdirectory("../${extKernelName}" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()



## Call util function to create rules for building object libs for this kernel
AUDIOLIB_create_object_lib_for_kernel()
