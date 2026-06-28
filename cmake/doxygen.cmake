# Doxygen

# look for Doxygen package
find_package(Doxygen)
# find_package(MathJax)

if(DOXYGEN_FOUND)
  # set input and output files
  set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/docs/doxygen/doxyfile_user_guide)
  set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.out)

  # request to configure the file
  configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
  message(STATUS "Doxygen build started")

  # bad hack
  file(READ ${DOXYGEN_OUT} FILE_CONTENTS)
  string(REPLACE "./docs" "../docs" FILE_CONTENTS "${FILE_CONTENTS}")
  string(REPLACE "PREDEFINED             =" "PREDEFINED             = __${DEVICE}__" FILE_CONTENTS "${FILE_CONTENTS}")


    # Map DEVICE to Doxygen ENABLED_SECTIONS for @if/@elif/@endif conditionals
  if(DEVICE STREQUAL "a53ss0")
    set(DOXYGEN_ENABLED_SECTION "ARM_A53")
  else()
    set(DOXYGEN_ENABLED_SECTION "C7X")
  endif()

  string(REPLACE "ENABLED_SECTIONS       ="
                 "ENABLED_SECTIONS       = ${DOXYGEN_ENABLED_SECTION}"
                 FILE_CONTENTS "${FILE_CONTENTS}")

  # For A53 builds, auto-detect which kernels lack an A53/ subdirectory and
  # exclude them from Doxygen output. Adding A53 support to a kernel (i.e.
  # creating src/<kernel>/A53/) automatically includes it in docs — no edits
  # to this file needed.
  if(DEVICE STREQUAL "a53ss0")
    file(GLOB ALL_KERNEL_DIRS "${CMAKE_SOURCE_DIR}/src/AUDIOLIB_*")
    set(A53_EXCLUDE_PATTERNS "")
    foreach(kdir ${ALL_KERNEL_DIRS})
      if(IS_DIRECTORY ${kdir} AND NOT IS_DIRECTORY "${kdir}/A53")
        get_filename_component(kname ${kdir} NAME)
        list(APPEND A53_EXCLUDE_PATTERNS "*/src/${kname}/*")
      endif()
    endforeach()
    if(A53_EXCLUDE_PATTERNS)
      string(JOIN " \\\n                         " A53_EXCLUDE_PATTERNS_STR ${A53_EXCLUDE_PATTERNS})
      string(REPLACE "EXCLUDE_PATTERNS       = */old/* \\"
                     "EXCLUDE_PATTERNS       = */old/* \\\n                         ${A53_EXCLUDE_PATTERNS_STR} \\"
                     FILE_CONTENTS "${FILE_CONTENTS}")
    endif()
  endif()

  file(WRITE ${DOXYGEN_OUT} "${FILE_CONTENTS}")

  set(DOXYGEN_LOG ${CMAKE_CURRENT_BINARY_DIR}/doxygen/doxygen-console.log)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
  # Note: do not put "ALL" - this builds docs together with application EVERY
  # TIME!
  add_custom_target(
    user_guide    
    COMMAND ${CMAKE_COMMAND} -E echo "Running Doxygen, logging to ${DOXYGEN_LOG}"
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT} > "${DOXYGEN_LOG}" 2>&1
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
else(DOXYGEN_FOUND)
  message(STATUS "Doxygen need to be installed to generate the doxygen documentation")
endif(DOXYGEN_FOUND)
