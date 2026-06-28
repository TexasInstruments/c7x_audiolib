## Build the common library as it's needed by all kernels
add_subdirectory(common)



if(BUILD_LIB)

  add_library(AUDIOLIB STATIC)
  AUDIOLIB_set_lib_extension(AUDIOLIB)

  # add_subdirectory(AUDIOLIB_round)
  # add_subdirectory(AUDIOLIB_recip)

  ######################################################################################
  ## CMake people do not recommend this; not sure about the correct way, needs review ##
  ######################################################################################

  file(GLOB SUBDIRS "${CMAKE_CURRENT_LIST_DIR}/*")
  # message("sub dirs are ${SUBDIRS}")
  
  # For A53 builds, only include kernels that have an A53/ subdirectory.
  # Adding A53 support to a kernel is just: create src/<kernel>/A53/ with the
  # optimized _ci.cpp — no CMake edits required.
  if(DEVICE STREQUAL "a53ss0")
    set(FILTERED_SUBDIRS "")
    foreach(subdir ${SUBDIRS})
      if(IS_DIRECTORY ${subdir})
        get_filename_component(folder_name ${subdir} NAME)
        if(folder_name STREQUAL "common" OR IS_DIRECTORY "${subdir}/A53")
          list(APPEND FILTERED_SUBDIRS ${subdir})
        endif()
      endif()
    endforeach()
    set(SUBDIRS ${FILTERED_SUBDIRS})
  endif()

  foreach(subdir ${SUBDIRS})
    if (IS_DIRECTORY  ${subdir})
      # message("Found directory ${subdir}")

      get_filename_component(nameOnly ${subdir} NAME)

      if (NOT ${nameOnly} STREQUAL "common")

	# message ("Adding kernel name ${nameOnly}")

	add_subdirectory(${subdir})
	target_link_libraries(AUDIOLIB "${nameOnly}_obj")

      endif()
    endif ()
  endforeach()

  # get_target_property(OUT AUDIOLIB LINK_LIBRARIES)
  # message("Linked objs are ${OUT}")

elseif (BUILD_TEST)

  add_subdirectory(${KERNEL_NAME})

endif()  

