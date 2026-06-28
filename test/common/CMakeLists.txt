enable_language(ASM)

if(DEVICE STREQUAL "a53ss0")
  # A53: collect base files + A53-specific files (no .asm)
  file(GLOB BASE_FILES "*.cpp" "*.c")
  file(GLOB ARCH_FILES "A53/*.cpp" "A53/*.c")
  set(COMMON_FILES ${BASE_FILES} ${ARCH_FILES})
elseif(TARGET_PLATFORM STREQUAL "PC")
  # C7x PC emulation: collect base files + c71 files (no .asm)
  file(GLOB BASE_FILES "*.cpp" "*.c")
  file(GLOB ARCH_FILES "c71/*.cpp" "c71/*.c")
  set(COMMON_FILES ${BASE_FILES} ${ARCH_FILES})
  set_source_files_properties(${COMMON_FILES}/*.c PROPERTIES LANGUAGE CXX)
else()
  # C7x target: collect base files + c71 files (with .asm)
  file(GLOB BASE_FILES "*.cpp" "*.c")
  file(GLOB ARCH_FILES "c71/*.cpp" "c71/*.c" "c71/*.asm")
  set(COMMON_FILES ${BASE_FILES} ${ARCH_FILES})
endif()
# message("common files are: ${COMMON_FILES}")

add_library(AUDIOLIB_test_common STATIC ${COMMON_FILES})

# test_common is first library built, so place dsplib dependency before
if(NOT DEVICE STREQUAL "a53ss0")
  if(NOT DEFINED ENV{DSPLIB_C7X_ROOT})
    add_dependencies(AUDIOLIB_test_common dsplib)
  endif()
endif()

audiolib_set_lib_extension(AUDIOLIB_test_common)

if(DEVICE STREQUAL "a53ss0")
  target_include_directories(AUDIOLIB_test_common PUBLIC ../common .
                                                       ${AARCH64_INCLUDES})
else()
  target_include_directories(AUDIOLIB_test_common PUBLIC ../common .
                                                       ${C7x_INCLUDES})
endif()
