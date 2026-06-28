set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Specify the cross-compiler
set(CMAKE_C_COMPILER 		    $ENV{CGT_GCC_AARCH64_ROOT}/bin/aarch64-none-elf-gcc)
set(CMAKE_CXX_COMPILER 		    $ENV{CGT_GCC_AARCH64_ROOT}/bin/aarch64-none-elf-g++)
set(CMAKE_ASM_COMPILER          $ENV{CGT_GCC_AARCH64_ROOT}/bin/aarch64-none-elf-gcc)
set(CMAKE_LINKER                $ENV{CGT_GCC_AARCH64_ROOT}/bin/aarch64-none-elf-gcc)
#set(CMAKE_LINKER                $ENV{CGT_GCC_AARCH64_ROOT}/bin/aarch64-none-elf-ld)
set(CMAKE_OBJCOPY               $ENV{CGT_GCC_AARCH64_ROOT}/bin/aarch64-none-elf-objcopy)
set(CMAKE_STRIP                 $ENV{CGT_GCC_AARCH64_ROOT}/bin/aarch64-none-elf-strip)


# Compiler flags for Cortex-A53
set(CMAKE_C_FLAGS "-mcpu=cortex-a53 -march=armv8-a" CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS "-mcpu=cortex-a53 -march=armv8-a" CACHE STRING "C++ flags")
set(CMAKE_EXE_LINKER_FLAGS "--specs=nosys.specs" CACHE STRING "Linker flags")



# Search paths
set(CMAKE_FIND_ROOT_PATH $ENV{CGT_GCC_AARCH64_ROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)


set(AARCH64_INCLUDES $ENV{CGT_GCC_AARCH64_ROOT}/lib/gcc/aarch64-none-elf/9.2.1/include)


# Avoid running try_compile on the target
set(CMAKE_CROSSCOMPILING TRUE)