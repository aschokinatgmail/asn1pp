# CMake cross-compilation toolchain for STM32F407 (ARM Cortex-M4F)
# 512KB Flash, 128KB SRAM, FPU single-precision
#
# Usage:
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-stm32f407.cmake \
#         -DEMBEDDED=ON -DNO_TEXT_CODECS=ON -DBUILD_TESTS=OFF

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(FPU_FLAGS "-fsingle-precision-constant -Wno-double-promotion")

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_RANLIB arm-none-eabi-ranlib)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_SIZE arm-none-eabi-size)

set(CMAKE_C_FLAGS_INIT "${CPU_FLAGS} ${FPU_FLAGS} -ffunction-sections -fdata-sections -fno-common" CACHE STRING "")
set(CMAKE_CXX_FLAGS_INIT "${CPU_FLAGS} ${FPU_FLAGS} -ffunction-sections -fdata-sections -fno-common -fno-exceptions -fno-rtti" CACHE STRING "")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--gc-sections -Wl,--no-warn-rwx-segments -nostartfiles -specs=nano.specs -specs=nosys.specs -T${CMAKE_CURRENT_SOURCE_DIR}/cmake/stm32f407.ld" CACHE STRING "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EMBEDDED ON CACHE BOOL "Embedded profile" FORCE)
set(NO_TEXT_CODECS ON CACHE BOOL "Strip text codecs" FORCE)
set(SIMD_SSE42 OFF CACHE BOOL "No SSE4.2 on ARM" FORCE)
set(SIMD_AVX2 OFF CACHE BOOL "No AVX2 on ARM" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "No tests on bare-metal" FORCE)
set(BUILD_GEN OFF CACHE BOOL "No code generator on bare-metal" FORCE)
