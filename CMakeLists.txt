cmake_minimum_required(VERSION 3.16.0)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/colorized_messages.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/clang-format.cmake)
# include(${CMAKE_CURRENT_LIST_DIR}/cmake/FindMathJax.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/doxygen.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/coverage.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/username.cmake)
# CMake custom util functions
include(${CMAKE_CURRENT_LIST_DIR}/cmake/utils.cmake)

# ==============================================================================
# Toolchain Environment Variable Checks
#
# - For A53 core (a53ss0): Requires CGT_GCC_AARCH64_ROOT
# - For C7x cores: Requires CGT7X_ROOT
# ==============================================================================

if(DEVICE STREQUAL "a53ss0")
  if(NOT DEFINED ENV{CGT_GCC_AARCH64_ROOT})
    message(
      FATAL_ERROR
        "${BoldRed} CGT_GCC_AARCH64_ROOT env variable is not set, CMake will exit.")
  endif()
else()
  # Check if CGT7X_ROOT env variable is set, if not try to set it
  if(NOT DEFINED ENV{CGT7X_ROOT})
    resolve_linux_username(CURRENT_USER)
    if (CURRENT_USER STREQUAL "")
      message(FATAL_ERROR "User name not found")
    endif()
    set(HOME "/home/${CURRENT_USER}")
    set(ENV{CGT_C7X_VERSION} "5.0.0.LTS")
    set(_TOOLS_PATH "${HOME}/ti")
    set(_CGT7X_ROOT "${_TOOLS_PATH}/ti-cgt-c7000_$ENV{CGT_C7X_VERSION}")
    set(ENV{TOOLS_PATH} "${_TOOLS_PATH}")
    set(ENV{CGT7X_ROOT} "${_CGT7X_ROOT}")
    set(ENV{PATH} "$ENV{CGT7X_ROOT}/bin:$ENV{PATH}")
    message(STATUS "${Green} CGT7X_ROOT env variable is set to: $ENV{CGT7X_ROOT}")
  else()
    message(STATUS "CGT7X_ROOT is already set: $ENV{CGT7X_ROOT}")
  endif()

  # Final check - if still not set, fail
  if(NOT DEFINED ENV{CGT7X_ROOT})
    message(
      FATAL_ERROR
        "${BoldRed} CGT7X_ROOT env variable is not set and could not be auto-configured, CMake will exit.")
  endif()
endif()
# ==============================================================================
# DSP Library (dsplib) Dependency
#
# dsplib provides optimized DSP kernels for C7x DSP cores.
# Expected at workspace/dsplib/ (sibling of audiolib) — pre-built by
# "west build-audiolib" before cmake is invoked.
# Not required for A53 builds.
# ==============================================================================

if(NOT DEVICE STREQUAL "a53ss0")
  get_filename_component(_audiolib_workspace "${CMAKE_SOURCE_DIR}/.." ABSOLUTE)

  # Resolve dsplib root: flat sibling → DSPLIB_ROOT env var
  set(_dsp_sibling "${_audiolib_workspace}/dsplib")
  set(_dsp_env     "$ENV{DSPLIB_ROOT}")

  if(EXISTS "${_dsp_sibling}/CMakeLists.txt")
    set(DSPLIB_ROOT "${_dsp_sibling}")
  elseif(NOT "${_dsp_env}" STREQUAL "" AND EXISTS "${_dsp_env}")
    set(DSPLIB_ROOT "${_dsp_env}")
  else()
    message(FATAL_ERROR
      "dsplib not found. Tried:\n"
      "  Sibling: ${_dsp_sibling}\n"
      "  Env:     DSPLIB_ROOT (not set or not found)\n"
      "Run: git clone https://github.com/TexasInstruments/c7x_dsplib.git dsplib  or  export DSPLIB_ROOT=/path/to/dsplib")
  endif()

  set(_dsplib_lib_dir "${DSPLIB_ROOT}/lib/${CMAKE_BUILD_TYPE}")
  if(TARGET_PLATFORM STREQUAL "PC")
    if(CMAKE_HOST_WIN32)
      set(_dsplib_artifact "${_dsplib_lib_dir}/DSPLIB_${DEVICE}_x86_64.lib")
    else()
      set(_dsplib_artifact "${_dsplib_lib_dir}/DSPLIB_${DEVICE}_x86_64.a")
    endif()
  else()
    set(_dsplib_artifact "${_dsplib_lib_dir}/DSPLIB_${DEVICE}.lib")
  endif()

  if((BUILD_TEST OR AUTO_TEST OR BUILD_EXAMPLE) AND NOT EXISTS "${_dsplib_artifact}")
    message(FATAL_ERROR
      "DSPLIB artifact not found: ${_dsplib_artifact}\n"
      "Run: west build-audiolib --soc <soc> to build dsplib first.")
  endif()

  set(ENV{DSPLIB_C7X_ROOT} "${DSPLIB_ROOT}")

  find_path(DSPLIB_INCLUDE_DIR
    NAMES dsplib.h
    PATHS "${DSPLIB_ROOT}/src/"
    NO_DEFAULT_PATH
  )

  if(NOT DSPLIB_INCLUDE_DIR)
    message(FATAL_ERROR "dsplib.h not found under DSPLIB_ROOT='${DSPLIB_ROOT}'")
  endif()
endif()

# Set release as default build type
if(NOT EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt)
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE
        "release"
        CACHE STRING "" FORCE)
  endif()
endif()

audiolib_print_build_details()

if(TARGET_PLATFORM STREQUAL "PC")
  set(CMAKE_EXPORT_COMPILE_COMMANDS
      ON
      CACHE INTERNAL "")

endif()

set(CMAKE_CXX_STANDARD 14)

# ##############################################################################
# Toolchain and Platform Configuration Selection
#
# This block configures the build environment based on the target core and
# platform:
# - A53 core (DEVICE = a53ss0): Uses GCC AArch64 cross-compiler from TI's
#   CGT_GCC_AARCH64 toolchain and MCU+ SDK libraries.
# - C7x DSP cores: Uses TI's CGT7X (C7000) compiler.
#   Supports two build platforms:
#     • PC emulation (TARGET_PLATFORM = PC): Builds host-executable binaries
#       using C7x host emulation headers and DSPLIB x86_64 static libraries.
#       Supports both Windows (MSVC) and Linux (GCC) host toolchains.
#     • Target hardware: Builds native C7x binaries using the real C7x
#       toolchain and target-specific DSPLIB libraries.
#
# Output directories for libraries and executables are centralized for easy
# location of build artifacts.
# ##############################################################################
if(DEVICE STREQUAL "a53ss0")
  # --------------------------------------------------------------------------
  # A53 Core (ARM AArch64) Configuration
  # --------------------------------------------------------------------------
  # Centralize all static libraries and executables
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
      "${CMAKE_CURRENT_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
      "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
  # Use AArch64 cross-compilation toolchain
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/aarch64.cmake")
  # Add include paths (compiler headers)
  include_directories(BEFORE SYSTEM "$ENV{CGT_GCC_AARCH64_ROOT}/include")

  get_property(all_includes DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
else()
# TI CGT package has different naming convention for C7524
set(DEVICE_C7524 "C7524-MMA2_256")

  if(TARGET_PLATFORM STREQUAL "PC")

    if(DEVICE STREQUAL "C7524")
      set(C7x_INCLUDES "$ENV{CGT7X_ROOT}/host_emulation/include/${DEVICE_C7524}"
                      "${DSPLIB_ROOT}/src")
    else()
      set(C7x_INCLUDES "$ENV{CGT7X_ROOT}/host_emulation/include/${DEVICE}"
                      "${DSPLIB_ROOT}/src")
    endif()

    set(DSPLIB_C7X
        ${DSPLIB_ROOT}/lib/${CMAKE_BUILD_TYPE}/DSPLIB_${DEVICE}_x86_64.a 
        ${DSPLIB_ROOT}/lib/${CMAKE_BUILD_TYPE}/DSPLIB_common_${DEVICE}_x86_64.a)


    if(CMAKE_HOST_WIN32)
      set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/msvc.cmake")
      set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

      # Place all bins static libs in known place
      set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/")
      set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")
    else()
      set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/gcc.cmake")
      # Place all bins static libs in known place
      set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
          "${CMAKE_CURRENT_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}")
      set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
          "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
    endif()

  else()

    # Place all bins static libs in known place
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")

    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/c7x.cmake")
    include_directories(BEFORE SYSTEM $ENV{CGT7X_ROOT}/include ${DSPLIB_ROOT}/src)

    set(DSPLIB_C7X
        ${DSPLIB_ROOT}/lib/${CMAKE_BUILD_TYPE}/DSPLIB_${DEVICE}.lib
        ${DSPLIB_ROOT}/lib/${CMAKE_BUILD_TYPE}/DSPLIB_common_${DEVICE}.lib)
endif()        

endif()

# Set compilation flags for release and debug based on TARGET_PLATFORM

audiolib_compile_options()
audiolib_include_xlib_headers()

project(AUDIOLIB)

# Place all bins static libs in known place
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")

set(CMAKE_EXPORT_COMPILE_COMMANDS
    ON
    CACHE INTERNAL "") # works
# ##############################################################################
# Create dependency based on testing a kernel or running the example for a given
# kernel #
# ##############################################################################

if(BUILD_TEST)

  add_subdirectory(test)

elseif(AUTO_TEST)

  set(BUILD_LIB 1)
  add_subdirectory(test)

elseif(BUILD_EXAMPLE)

  set(BUILD_LIB 1)
  add_subdirectory(examples)

  # if (TARGET_PLATFORM STREQUAL "DSP") # set(CMAKE_EXE_LINKER_FLAGS
  # "--library=${CMAKE_CURRENT_LIST_DIR}/build/bin/libAUDIOLIB.a
  # ${CMAKE_EXE_LINKER_FLAGS}") # message("This is the updated
  # CMAKE_EXE_LINKER_FLAGS:${CMAKE_EXE_LINKER_FLAGS}") endif()

endif()

# Rules for building src directory
add_subdirectory(src bin)
