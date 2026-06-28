// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#if defined(__C7100__)
/**

 \page build_instructions_linux Ubuntu 22.04 Build Instructions for AUDIOLIB

 \par Python3 Setup

 - We auto generate test cases via python scripts pacakged in the library

 - Please ensure that you have the  following python3 packages installed

   - numpy, scipy, and scikit-learn

 \par Environment Setup

  - Run the executables on EVM using CCS (Code composer studio)

  - Setup needed for the compiler for CGT7x
    - Download [CGT7x Compiler Installer](https://www.ti.com/tool/C7000-CGT) and install
    - export CGT7X_ROOT=${CGT7X_COMPILER_DIR}

  - Path setup
    - export PATH=${CGT7X_ROOT}/bin:${PATH}

  - AUDIOLIB depends on DSPLIB
    - Please set DSPLIB_C7X_ROOT env variable to the location of DSPLIB
    - Ex:- export DSPLIB_C7X_ROOT=${HOME}/freertos_sdk_am275x_11_01_00_10/source/dsplib/

 \par Build

  - AUDIOLIB utilizes CMake to build, ensure that system has CMake v3.16.0 or higher
  - Build options:
    - SOC=AM62A/j721e/j721s2/j784s4/j722s
    - DEVICE=C7504/C7100/C7120/C7524
    - TARGET_PLATFORM=PC/""
    - KERNEL_NAME
    - BUILD_TEST=1 OR BUILD_EXAMPLE=1 OR AUTO_TEST=1
    - AUDIOLIB_DEBUGPRINT=1/0
    - AUDIOLIB_TESTPRINT
    - ALL_TEST_CASES=1 OR TEST_CASE=TestCaseID
    - CMAKE_EXPORT_COMPILE_COMMANDS=TRUE
    - CMAKE_BUILD_TYPE=Release/Debug
    - AUTO_TEST=1/0 (builds all test cases and libraries)
  - Example CMake commands
    - To build AUDIOLIB_add for target on J721E and run performance test cases:
      - \code cmake -B build -DTARGET_PLATFORM="" -DBUILD_TEST="1" -DKERNEL_NAME="AUDIOLIB_add" -DSOC="j721e" \
-DDEVICE="C7100" -DAUDIOLIB_DEBUGPRINT="0" -DALL_TEST_CASES="1" -DCMAKE_EXPORT_COMPILE_COMMANDS="TRUE" \
-DCMAKE_BUILD_TYPE="Release" \endcode
    - To build AUDIOLIB_dotprod for host emulation on J721E and run example code
      - \code cmake -B build -DTARGET_PLATFORM="PC" -DBUILD_EXAMPLE="1" -DKERNEL_NAME="AUDIOLIB_dotprod" \
-DSOC="j721e" -DDEVICE="C7100" -DAUDIOLIB_DEBUGPRINT="0" -DCMAKE_EXPORT_COMPILE_COMMANDS="TRUE" \
-DCMAKE_BUILD_TYPE="Release" \endcode
    - To build AUDIOLIB_max for host emulation on J721E and run test case 2 with debug print statements
        - \code cmake -B build -DTARGET_PLATFORM="PC" -DBUILD_TEST="1" -DKERNEL_NAME="AUDIOLIB_max" -DSOC="j721e" \
-DDEVICE="C7100" -DAUDIOLIB_DEBUGPRINT="1" -DTEST_CASE="2" -DCMAKE_EXPORT_COMPILE_COMMANDS="TRUE" \
-DCMAKE_BUILD_TYPE="Release" \endcode
    -  To build all tests cases and libraries for release for J721E for target
        - \code cmake -B build -DTARGET_PLATFORM="" -DAUTO_TEST="1" -DSOC="j721e" -DDEVICE="C7100" \
-DALL_TEST_CASES="1" -DCMAKE_BUILD_TYPE="Release" -DAUDIOLIB_DEBUGPRINT="0" \endcode
    - Then run:
        - \code cmake --build build -j<num_cores> \endcode
  - AUDIOLIB can also be built using the Visual Studio Code's build configuration feature
    - Rename vscode/ to .vscode/
    - In the settings.json, change build parameters as needed
    - "Build" and "Run" buttons can be used to build for target/PC and run for PC
  - Note: If changing build configuration options significantly (e.g. changing between target and host emulation
builds), delete build folder before rebuilding

 \par Run

  - To run AUDIOLIB kernels on a PC:
    - \code bin/Release/test_AUDIOLIB_<kernel_name>_<device>_x86_64 \endcode OR \code
bin/Debug/test_AUDIOLIB_<kernel_name>_<device>_x86_64 \endcode
      - Ex: \code bin/Release/test_AUDIOLIB_sqr_C7100_x86_64 \endcode
  - To run AUDIOLIB kernels for target
    - To reproduce numbers given in the datasheet, run the .out program on the EVM in No Boot Mode

*/

#elif defined(__C7120__)
/**

 \page build_instructions_linux Ubuntu 22.04 Build Instructions for AUDIOLIB

 \par Python3 Setup

 - We auto generate test cases via python scripts pacakged in the library

 - Please ensure that you have the  following python3 packages installed

   - numpy, scipy, and scikit-learn

 \par Environment Setup

  - Run the executables on EVM using CCS (Code composer studio)

  - Setup needed for the compiler for CGT7x
    - Download [CGT7x Compiler Installer](https://www.ti.com/tool/C7000-CGT) and install
    - export CGT7X_ROOT=${CGT7X_COMPILER_DIR}

  - Path setup
    - export PATH=${CGT7X_ROOT}/bin:${PATH}

  - AUDIOLIB depends on DSPLIB
    - Please set DSPLIB_C7X_ROOT env variable to the location of DSPLIB
    - Ex:- export DSPLIB_C7X_ROOT=${HOME}/freertos_sdk_am275x_11_01_00_10/source/dsplib/

 \par Build

  - AUDIOLIB utilizes CMake to build, ensure that system has CMake v3.16.0 or higher
  - Build options:
    - SOC=AM62A/j721e/j721s2/j784s4/j722s
    - DEVICE=C7504/C7100/C7120/C7524
    - TARGET_PLATFORM=PC/""
    - KERNEL_NAME
    - BUILD_TEST=1 OR BUILD_EXAMPLE=1 OR AUTO_TEST=1
    - AUDIOLIB_DEBUGPRINT=1/0
    - AUDIOLIB_TESTPRINT
    - ALL_TEST_CASES=1 OR TEST_CASE=TestCaseID
    - CMAKE_EXPORT_COMPILE_COMMANDS=TRUE
    - CMAKE_BUILD_TYPE=Release/Debug
    - AUTO_TEST=1/0 (builds all test cases and libraries)
  - Example CMake commands
    - To build AUDIOLIB_add for target on J721S2 and run performance test cases:
      - \code cmake -B build -DTARGET_PLATFORM="" -DBUILD_TEST="1" -DKERNEL_NAME="AUDIOLIB_add" -DSOC="j721s2" \
-DDEVICE="C7120" -DAUDIOLIB_DEBUGPRINT="0" -DALL_TEST_CASES="1" -DCMAKE_EXPORT_COMPILE_COMMANDS="TRUE" \
-DCMAKE_BUILD_TYPE="Release" \endcode
    - To build AUDIOLIB_dotprod for host emulation on J721S2 and run example code
      - \code cmake -B build -DTARGET_PLATFORM="PC" -DBUILD_EXAMPLE="1" -DKERNEL_NAME="AUDIOLIB_dotprod" \
-DSOC="j721s2" -DDEVICE="C7120" -DAUDIOLIB_DEBUGPRINT="0" -DCMAKE_EXPORT_COMPILE_COMMANDS="TRUE" \
-DCMAKE_BUILD_TYPE="Release" \endcode
    - To build AUDIOLIB_max for host emulation on J721S2 and run test case 2 with debug print statements
        - \code cmake -B build -DTARGET_PLATFORM="PC" -DBUILD_TEST="1" -DKERNEL_NAME="AUDIOLIB_max" -DSOC="j721s2" \
-DDEVICE="C7120" -AUDIOLIB_DEBUGPRINT="1" -DTEST_CASE="2" -DCMAKE_EXPORT_COMPILE_COMMANDS="TRUE" \
-DCMAKE_BUILD_TYPE="Release" \endcode
    -  To build all tests cases and libraries for release for J721S2 for target
        - \code cmake -B build -DTARGET_PLATFORM="" -DAUTO_TEST="1" -DSOC="j721s2" -DDEVICE="C7120" \
-DALL_TEST_CASES="1" -DCMAKE_BUILD_TYPE="Release" -DAUDIOLIB_DEBUGPRINT="0" \endcode
    - Then run:
        - \code cmake --build build -j<num_cores> \endcode
    - To build for J784S4, replace the SOC option in the above examples to "j784s4"
  - AUDIOLIB can also be built using the Visual Studio Code's build configuration feature
    - Rename vscode/ to .vscode/
    - In the settings.json, change build parameters as needed
    - "Build" and "Run" buttons can be used to build for target/PC and run for PC
  - Note: If changing build configuration options significantly (e.g. changing between target and host emulation
builds), delete build folder before rebuilding

 \par Run

  - To run AUDIOLIB kernels on a PC:
    - \code bin/Release/test_AUDIOLIB_<kernel_name>_<device>_x86_64 \endcode OR \code
bin/Debug/test_AUDIOLIB_<kernel_name>_<device>_x86_64 \endcode
      - Ex: \code bin/Release/test_AUDIOLIB_sqr_C7120_x86_64 \endcode
  - To run AUDIOLIB kernels for target
    - To reproduce numbers given in the datasheet, run the .out program on the EVM in No Boot Mode

*/

#elif defined(__C7504__)
/**

 \page build_instructions_linux Ubuntu 22.04 Build Instructions for AUDIOLIB

 \par Python3 Setup

 - We auto generate test cases via python scripts pacakged in the library

 - Please ensure that you have the  following python3 packages installed

   - numpy, scipy, and scikit-learn

 \par Environment Setup

  - Run the executables on EVM using CCS (Code composer studio)

  - Setup needed for the compiler for CGT7x
    - Download [CGT7x Compiler Installer](https://www.ti.com/tool/C7000-CGT) and install
    - export CGT7X_ROOT=${CGT7X_COMPILER_DIR}

  - Path setup
    - export PATH=${CGT7X_ROOT}/bin:${PATH}

  - AUDIOLIB depends on DSPLIB
    - Please set DSPLIB_C7X_ROOT env variable to the location of DSPLIB
    - Ex:- export DSPLIB_C7X_ROOT=${HOME}/freertos_sdk_am62dx_11_01_00_10/source/dsplib/

 \par Build

  - AUDIOLIB utilizes CMake to build, ensure that system has CMake v3.16.0 or higher
  - Build options:
    - SOC=AM62A/j721e/j721s2/j784s4/j722s/AM62D
    - DEVICE=C7504/C7100/C7120/C7120/C7524/C7504
    - TARGET_PLATFORM=PC/""
    - KERNEL_NAME
    - BUILD_TEST=1 OR BUILD_EXAMPLE=1 OR AUTO_TEST=1
    - AUDIOLIB_DEBUGPRINT=1/0
    - AUDIOLIB_TESTPRINT
    - ALL_TEST_CASES=1 OR TEST_CASE=TestCaseID
    - CMAKE_EXPORT_COMPILE_COMMANDS=TRUE
    - CMAKE_BUILD_TYPE=Release/Debug
    - AUTO_TEST=1/0 (builds all test cases and libraries)
  - Example CMake commands
    - To build AUDIOLIB_asrc for target on AM62D and run performance test cases:
      - \code cmake -B build -DTARGET_PLATFORM="" -DBUILD_TEST="1" -DKERNEL_NAME="AUDIOLIB_asrc" -DSOC="AM62D" \
-DDEVICE="C7504" -DMMA=2_256 -DAUDIOLIB_DEBUGPRINT="0" -DALL_TEST_CASES="1" -DCMAKE_EXPORT_COMPILE_COMMANDS="TRUE" \
-DCMAKE_BUILD_TYPE="Release" \endcode
    - Then run:
      - \code cmake --build build -j<num_cores> \endcode
  - Note: If changing build configuration options significantly (e.g. changing between target and host emulation
builds), delete build folder before rebuilding

 \par Run

  - To run AUDIOLIB kernels on a PC:
    - \code bin/Release/test_AUDIOLIB_<kernel_name>_<device>_x86_64 \endcode OR \code
bin/Debug/test_AUDIOLIB_<kernel_name>_<device>_x86_64 \endcode
      - Ex: \code bin/Release/test_AUDIOLIB_asrc_C7504_x86_64 \endcode
  - To run AUDIOLIB kernels for target
    - To reproduce numbers given in the datasheet, run the .out program on the EVM in No Boot Mode

*/

#elif defined(__C7524__)
/**

 \page build_instructions_linux Ubuntu 22.04 Build Instructions for AUDIOLIB

 \par Python3 Setup

 - We auto generate test cases via python scripts pacakged in the library

 - Please ensure that you have the  following python3 packages installed

   - numpy, scipy, and scikit-learn

 \par Environment Setup

  - Run the executables on EVM using CCS (Code composer studio)

  - Setup needed for the compiler for CGT7x
    - Download [CGT7x Compiler Installer](https://www.ti.com/tool/C7000-CGT) and install
    - export CGT7X_ROOT=${CGT7X_COMPILER_DIR}

  - Path setup
    - export PATH=${CGT7X_ROOT}/bin:${PATH}

  - AUDIOLIB depends on DSPLIB
    - Please set DSPLIB_C7X_ROOT env variable to the location of DSPLIB
    - Ex:- export DSPLIB_C7X_ROOT=${HOME}/freertos_sdk_am275x_11_01_00_10/source/dsplib/

 \par Build

  - AUDIOLIB utilizes CMake to build, ensure that system has CMake v3.16.0 or higher
  - Build options:
    - SOC=AM62A/j721e/j721s2/j784s4/j722s/AM275
    - DEVICE=C7504/C7100/C7120/C7524/C7524
    - TARGET_PLATFORM=PC/""
    - KERNEL_NAME
    - BUILD_TEST=1 OR BUILD_EXAMPLE=1 OR AUTO_TEST=1
    - AUDIOLIB_DEBUGPRINT=1/0
    - AUDIOLIB_TESTPRINT
    - ALL_TEST_CASES=1 OR TEST_CASE=TestCaseID
    - CMAKE_EXPORT_COMPILE_COMMANDS=TRUE
    - CMAKE_BUILD_TYPE=Release/Debug
    - AUTO_TEST=1/0 (builds all test cases and libraries)
  - Example CMake commands
    - To build AUDIOLIB_asrc for target on AM275 and run performance test cases:
      - \code cmake -B build -DTARGET_PLATFORM="" -DBUILD_TEST="1" -DKERNEL_NAME="AUDIOLIB_asrc" -DSOC="AM275" \
-DDEVICE="C7524" -DMMA=2_256F -DAUDIOLIB_DEBUGPRINT="0" -DTEST_CASE="1" -DCMAKE_EXPORT_COMPILE_COMMANDS="TRUE" \
-DCMAKE_BUILD_TYPE="Release" \endcode
    - Then run:
        - \code cmake --build build -j<num_cores> \endcode
  - Note: If changing build configuration options significantly (e.g. changing between target and host emulation
builds), delete build folder before rebuilding

 \par Run

  - To run AUDIOLIB kernels on a PC:
    - \code bin/Release/test_AUDIOLIB_<kernel_name>_<device>_x86_64 \endcode OR \code
bin/Debug/test_AUDIOLIB_<kernel_name>_<device>_x86_64 \endcode
      - Ex: \code bin/Release/test_AUDIOLIB_asrc_C7524_x86_64 \endcode
  - To run AUDIOLIB kernels for target
    - To reproduce numbers given in the datasheet, run the .out program on the EVM in No Boot Mode

*/

#elif defined(__a53ss0__)
/**

 \page build_instructions_linux Ubuntu 22.04 Build Instructions for AUDIOLIB

 \par Python3 Setup

 - We auto generate test cases via python scripts pacakged in the library

 - Please ensure that you have the  following python3 packages installed

   - numpy, scipy, and scikit-learn

 \par Environment Setup

  - Run the executables on EVM using CCS (Code composer studio)

  - Setup needed for the ARM compiler (A53 core)
    - Download [AArch64 Compiler
Installer](https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-aarch64-none-elf.tar.xz)
and install
    - export CGT_GCC_AARCH64_ROOT=${CGT_GCC_AARCH64_COMPILER_DIR}

  - Setup needed for MCU+ SDK
    - Please set MCU_PLUS_SDK_ROOT env variable
    - Ex:- export MCU_PLUS_SDK_ROOT=${HOME}/freertos_sdk_am62dx_11_01_00_00

 \par Build

  - AUDIOLIB utilizes CMake to build, ensure that system has CMake v3.16.0 or higher
  - Build options:
    - SOC=AM62D
    - DEVICE=a53ss0
    - TARGET_PLATFORM=""
    - KERNEL_NAME
    - BUILD_TEST=1 OR BUILD_EXAMPLE=1 OR AUTO_TEST=1
    - AUDIOLIB_DEBUGPRINT=1/0
    - AUDIOLIB_TESTPRINT
    - ALL_TEST_CASES=1 OR TEST_CASE=TestCaseID
    - CMAKE_EXPORT_COMPILE_COMMANDS=TRUE
    - CMAKE_BUILD_TYPE=Release/Debug
    - AUTO_TEST=1/0 (builds all test cases and libraries)
  - Example CMake commands
    - To build AUDIOLIB_asrc for target on AM62D-A53 and run performance test cases:
      - \code cmake -B build -DTARGET_PLATFORM="" -DBUILD_TEST="1" -DKERNEL_NAME="AUDIOLIB_asrc" -DSOC="AM62D" \
-DDEVICE="a53ss0" -DMMA=2_256 -DAUDIOLIB_DEBUGPRINT="0" -DALL_TEST_CASES="1" -DCMAKE_EXPORT_COMPILE_COMMANDS="TRUE" \
-DCMAKE_BUILD_TYPE="Release" \endcode
    - Then run:
      - \code cmake --build build -j<num_cores> \endcode
  - Note: Delete the build folder before rebuilding. This is supported only on the target SOC, not on PC.

 \par Run

  - To run AUDIOLIB kernels for target
    - To reproduce numbers given in the datasheet, run the .out program on the EVM in No Boot Mode

*/

#endif
