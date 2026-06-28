enable_language(ASM)

if(TARGET_PLATFORM STREQUAL "PC")
  file(GLOB_RECURSE GENERATED_FILES "*.cpp" "*.c")
  set_source_files_properties(${GENERATED_FILES}/*.c PROPERTIES LANGUAGE CXX)
else()
  #file(GLOB_RECURSE COMMON_FILES "*.cpp" "*.c" "*.asm")
  file(GLOB_RECURSE GENERATED_FILES "*.cpp" "*.c")
endif()

# # Print all the source files included in AUDIOLIB_test_generated
# message(STATUS " %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%All source files for AUDIOLIB_test_generated:")
# foreach(file IN LISTS GENERATED_FILES)
#     message(STATUS "  ${file}")
# endforeach()


add_library(AUDIOLIB_test_generated STATIC ${GENERATED_FILES})

# test_common is first library built, so place dsplib dependency before
#add_dependencies(AUDIOLIB_test_common dsplib)

audiolib_set_lib_extension(AUDIOLIB_test_generated)

# target_include_directories(AUDIOLIB_test_generated PUBLIC ../generated .
#                                                      ${AARCH64_INCLUDES})
# target_include_directories(AUDIOLIB_test_generated PUBLIC
#     ../generated
#     .
#     $ENV{CGT_GCC_AARCH64_ROOT}/include
#     $ENV{MCU_PLUS_SDK_ROOT}/source
#     $ENV{MCU_PLUS_SDK_ROOT}/source/drivers
#     $ENV{MCU_PLUS_SDK_ROOT}/source/drivers/uart
#     $ENV{MCU_PLUS_SDK_ROOT}/source/drivers/uart/v0
#     $ENV{MCU_PLUS_SDK_ROOT}/source/drivers/hw_include
#     $ENV{MCU_PLUS_SDK_ROOT}/source/board
# )

target_compile_definitions(AUDIOLIB_test_generated PUBLIC SOC_AM62DX)
# message(STATUS " %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%All GEN INCLUDEs for AUDIOLIB_test_generated:")
# foreach(file IN LISTS AUDIOLIB_test_generated)
#     message(STATUS "  ${file}")
# endforeach()