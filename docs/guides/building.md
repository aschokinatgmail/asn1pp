---
title: Building & Configuration
---

# Building asn1pp

This guide covers building asn1pp from source using CMake with the KConfig configuration system.

## Overview

asn1pp uses KConfig to manage build-time configuration. The `Kconfig` file at the repository root defines all configurable options. During CMake configuration, the `cmake/kconfig.cmake` module parses this file and generates:

- CMake cache variables for each option
- A `generated/kconfig.hpp` header with `CONFIG_*` preprocessor macros
- Compile definitions for use in source code (`ASN1PP_*` prefixes)

## Standard Build

Standard build for development on x86_64 Linux/macOS/Windows:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This produces a static library `asn1pp-codec` with all codecs enabled (BER, DER, PER, UPER, OER, COER, XER, JER) and SSE4.2/AVX2 SIMD acceleration.

## KConfig Options

The following table lists all configuration options available in asn1pp:

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `CONFIG_EMBEDDED` | bool | OFF | Embedded profile (zero heap, fixed buffers). Auto-enables NO_TEXT_CODECS. |
| `CONFIG_NO_TEXT_CODECS` | bool | OFF | Strip text codecs (XER/JER) from build. Auto-selected by EMBEDDED. |
| `CONFIG_MAX_PDU` | int | 2048 | Maximum PDU buffer size in bytes (64-65536). Used in embedded mode. |
| `CONFIG_SIMD_SSE42` | bool | ON | SSE4.2 SIMD backend for x86_64 targets. |
| `CONFIG_SIMD_AVX2` | bool | ON | AVX2 SIMD backend for x86_64 targets. Requires SSE4.2. |
| `CONFIG_SIMD_NEON` | bool | ON | ARM NEON SIMD backend for ARM targets. |
| `CONFIG_CODEC_BER` | bool | ON | Basic Encoding Rules (BER). Foundation for most other codecs. |
| `CONFIG_CODEC_DER` | bool | ON | Distinguished Encoding Rules (DER). Requires BER. |
| `CONFIG_CODEC_PER` | bool | ON | Packed Encoding Rules (PER). |
| `CONFIG_CODEC_UPER` | bool | ON | Unaligned PER (UPER). Requires PER. |
| `CONFIG_CODEC_OER` | bool | ON | Octet Encoding Rules (OER). Canonical fixed-width binary. |
| `CONFIG_CODEC_COER` | bool | ON | Canonical OER. Requires OER. |
| `CONFIG_CODEC_XER` | bool | ON | XML Encoding Rules (XER). Disabled when NO_TEXT_CODECS=ON. |
| `CONFIG_CODEC_CXER` | bool | ON | Canonical XER. Requires XER. |
| `CONFIG_CODEC_JER` | bool | ON | JSON Encoding Rules (JER). Disabled when NO_TEXT_CODECS=ON. |
| `CONFIG_BUILD_TESTS` | bool | ON | Build test suite (requires Google Test via FetchContent). |
| `CONFIG_BUILD_GEN` | bool | ON | Build asn1pp-gen CLI code generator. |

### Dependencies Between Options

Some options have implicit dependencies:

- `CODEC_DER` depends on `CODEC_BER` — disabling BER auto-disables DER
- `CODEC_UPER` depends on `CODEC_PER` — disabling PER auto-disables UPER
- `CODEC_COER` depends on `CODEC_OER` — disabling OER auto-disables COER
- `CODEC_CXER` depends on `CODEC_XER` — disabling XER auto-disables CXER
- `SIMD_AVX2` depends on `SIMD_SSE42` — SSE4.2 must be enabled for AVX2
- `EMBEDDED` auto-selects `NO_TEXT_CODECS` — text codecs are stripped automatically

When dependencies are not met, the KConfig system auto-disables dependent options and prints a message during CMake configuration.

## Build Profiles

### Standard Profile (Default)

All codecs, all SIMD backends, heap allocations allowed:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

Target: x86_64 Linux/macOS/Windows workstations.

### Embedded Profile

Minimal footprint for bare-metal targets. Disables heap, strips text codecs, fixes PDU size:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel -DEMBEDDED=ON
```

This sets:
- `NO_TEXT_CODECS=ON` (XER/JER excluded from build)
- `MAX_PDU` defaults to 2048
- `ASN1PP_EMBEDDED` compile definition enabled
- `ASN1PP_NO_TEXT_CODECS` compile definition enabled

The embedded profile produces a static library with BER, DER, PER, UPER, OER, COER only.

### Minimal Profile

For extreme resource constraints, disable unused codecs:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DEMBEDDED=ON \
    -DCODEC_BER=ON \
    -DCODEC_DER=ON \
    -DCODEC_PER=OFF \
    -DCODEC_UPER=OFF \
    -DCODEC_OER=ON \
    -DCODEC_COER=OFF
```

This leaves only BER, DER, and OER codecs.

## Cross-Compilation for STM32F407

The STM32F407 is an ARM Cortex-M4F microcontroller with 512KB Flash and 128KB SRAM. The project includes a toolchain file at `cmake/toolchain-stm32f407.cmake`.

### Prerequisites

Install ARM toolchain:

```bash
# macOS
brew install arm-none-eabi-gcc

# Linux
sudo apt install gcc-arm-none-eabi
```

### Building

```bash
cmake -B build_cross \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-stm32f407.cmake \
    -DBAREMETAL_DEMO=ON

cmake --build build_cross
```

The toolchain file automatically configures:
- `EMBEDDED=ON` (enables fixed-buffer mode)
- `NO_TEXT_CODECS=ON` (strips XER/JER)
- `SIMD_SSE42=OFF` and `SIMD_AVX2=OFF` (no SIMD on ARM)
- `BUILD_TESTS=OFF` and `BUILD_GEN=OFF` (host tools not needed on target)

Output: `bare-metal-demo.elf` in the build directory.

### STM32F407 Toolchain Flags

The toolchain file applies the following compiler flags:

- `-mcpu=cortex-m4 -mthumb` — Cortex-M4 in thumb mode
- `-mfpu=fpv4-sp-d16 -mfloat-abi=hard` — single-precision FPU with hardware calling convention
- `-ffunction-sections -fdata-sections` — section garbage collection
- `-fno-exceptions -fno-rtti` — no C++ exceptions or RTTI
- Linker: `-Wl,--gc-sections -nostartfiles -specs=nano.specs -specs=nosys.specs`

Flash footprint: 23.8 KB for BER+PER+UPER+OER codecs.

## Output Targets

Building asn1pp produces different targets depending on configuration:

### asn1pp-codec (Always)

Static library containing the ASN.1 codec implementation. Built from:
- `libs/buffer/` — buffer management
- `libs/codec/codec.cpp` — common codec infrastructure
- `libs/codec/ber/` — BER and DER encoder/decoder
- `libs/codec/per/` — PER encoder/decoder
- `libs/codec/oer/` — OER encoder/decoder
- `libs/codec/xer/` — XER and CXER encoder/decoder (unless NO_TEXT_CODECS or EMBEDDED)
- `libs/codec/jer/` — JER encoder/decoder (unless NO_TEXT_CODECS or EMBEDDED)

### asn1pp-arch (Always)

Static library containing SIMD architecture code. Selects best implementation at runtime based on CPU features.

### asn1pp-gen (When BUILD_GEN=ON)

Host tool for generating C++ code from ASN.1 module files. Only built on the host, not for cross-compilation targets.

### bare-metal-demo (When BAREMETAL_DEMO=ON)

ELF binary demonstrating asn1pp on bare-metal hardware. Encodes/decodes sample PDUs.

## Generated Headers

CMake generates two headers in `build/generated/`:

### version.hpp

Contains version macros:
```cpp
#define ASN1PP_VERSION_MAJOR 0
#define ASN1PP_VERSION_MINOR 1
#define ASN1PP_VERSION_PATCH 0
```

### kconfig.hpp

Contains configuration state:
```cpp
// Build Profile
#define CONFIG_EMBEDDED 1
#define CONFIG_NO_TEXT_CODECS 1

// Buffer Sizing
#define CONFIG_MAX_PDU 2048

// SIMD Acceleration
#define CONFIG_SIMD_SSE42 1
#define CONFIG_SIMD_AVX2 1
#define CONFIG_SIMD_NEON 1

// Encoding Rules
#define CONFIG_CODEC_BER 1
#define CONFIG_CODEC_DER 1
// ... (all enabled codecs)
```

Source code can include this header with:
```cpp
#include <generated/kconfig.hpp>
```

## Advanced Options

### Changing MAX_PDU

For embedded targets with different buffer requirements:

```bash
cmake -B build -G Ninja -DEMBEDDED=ON -DMAX_PDU=4096
```

Valid range: 64 to 65536 bytes. Values outside this range are clamped automatically.

### Disabling SIMD

For compatibility with older CPUs or to reduce code size:

```bash
cmake -B build -G Ninja -DSIMD_SSE42=OFF -DSIMD_AVX2=OFF
```

When SSE4.2 is disabled, AVX2 is automatically disabled as well (AVX2 requires SSE4.2 as a baseline).

### Disabling Individual Codecs

Reduce binary size by excluding unused codecs:

```bash
cmake -B build -G Ninja \
    -DCODEC_XER=OFF \
    -DCODEC_CXER=OFF \
    -DCODEC_JER=OFF
```

### CMake Cache

Options are cached in the build directory. To reconfigure with different options, either delete the build directory or use CMake's interactive tools:

```bash
cmake -B build -G Ninja  # initial configure
ccmake build             # interactive cache editing
```

## Verification

After building, verify the configuration:

```bash
# Check enabled codecs
grep -E "ASN1PP_CODEC" build/generated/kconfig.hpp

# Check version
head -10 build/generated/version.hpp

# Verify static library contents
nm build/libs/asn1pp-codec.a | grep -E "encode|decode" | head -20

# Check bare-metal demo size (for ARM)
arm-none-eabi-size build/bare-metal-demo.elf
```

## Troubleshooting

### CMake cannot find toolchain file

Use an absolute path or ensure the path is relative to the source directory (not the build directory):

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$(pwd)/cmake/toolchain-stm32f407.cmake
```

### KConfig options not visible in cmake-gui

The KConfig file is parsed automatically during the first CMake run. If options do not appear, check that the `Kconfig` file exists at the repository root and that `cmake/kconfig.cmake` is present.

### Codec auto-disabled due to dependency

If a codec is disabled that you expected to be enabled, check the dependency chain. For example, if `CODEC_DER` is OFF even though you did not explicitly disable it, verify that `CODEC_BER` is ON.