---
title: Embedded & Bare-Metal
---

# Embedded & Bare-Metal Guide

asn1pp runs on bare-metal ARM Cortex-M targets with no operating system, no heap, and no standard library. This guide covers what the embedded profile does, how to build for STM32F407, and what memory to expect.

## Overview

asn1pp targets embedded environments where:
- No dynamic memory allocation is available or desired
- No C++ exceptions or RTTI
- Flash and RAM are severely constrained
- The toolchain is a bare-metal cross-compiler (e.g., `arm-none-eabi-gcc`)

The STM32F407 is a representative target: ARM Cortex-M4F with 512 KB Flash and 128 KB SRAM. The entire codec library, without any text codecs, fits in 23.8 KB of Flash.

## What EMBEDDED=ON Does

Setting `EMBEDDED=ON` at the CMake level triggers a cascade of build changes:

| Effect | Detail |
|---|---|
| No heap | `ASN1PP_NO_HEAP` is defined. All buffers are stack-allocated or provided by the caller. |
| No exceptions | `-fno-exceptions` passed to the compiler. All error handling is via return codes (`asn1pp::result`). |
| No RTTI | `-fno-rtti` passed to the compiler. |
| Text codecs stripped | XER and JER are excluded. This saves roughly 8 KB of text. |
| Fixed PDU size | `ASN1PP_MAX_PDU` defaults to 2048. Decoders reject PDUs larger than this. |
| SIMD disabled | SSE4.2 and AVX2 are explicitly disabled for ARM targets. |

These settings are enforced in `cmake/toolchain-stm32f407.cmake`:

```cmake
set(EMBEDDED ON CACHE BOOL "Embedded profile" FORCE)
set(NO_TEXT_CODECS ON CACHE BOOL "Strip text codecs" FORCE)
set(SIMD_SSE42 OFF CACHE BOOL "No SSE4.2 on ARM" FORCE)
set(SIMD_AVX2 OFF CACHE BOOL "No AVX2 on ARM" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "No tests on bare-metal" FORCE)
set(BUILD_GEN OFF CACHE BOOL "No code generator on bare-metal" FORCE)
```

The `config.hpp` header reflects these choices:

```cpp
#ifdef ASN1PP_EMBEDDED
inline constexpr bool is_embedded = true;
#define ASN1PP_NO_HEAP
static_assert(ASN1PP_MAX_PDU > 0, "ASN1PP_MAX_PDU must be positive in embedded mode");
#else
inline constexpr bool is_embedded = false;
#endif

#ifndef ASN1PP_MAX_PDU
#define ASN1PP_MAX_PDU 2048
#endif
```

## STM32F407 Walkthrough

### Prerequisites

You need the ARM bare-metal toolchain. The project ships with a Docker-based builder:

```bash
docker run --rm -v $(pwd):/src asn1pp-cross-builder \
  cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-stm32f407.cmake \
        -DEMBEDDED=ON -DNO_TEXT_CODECS=ON -DBUILD_TESTS=OFF
```

Inside the container, the toolchain is pre-configured. The `toolchain-stm32f407.cmake` file sets the correct compiler prefix (`arm-none-eabi-`), CPU flags, FPU flags, and linker flags.

### Build Commands

```bash
# Configure
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-stm32f407.cmake \
      -DEMBEDDED=ON -DNO_TEXT_CODECS=ON -DBUILD_TESTS=OFF

# Compile
cmake --build build --target asn1pp-codec asn1pp-arch

# Inspect the archive
arm-none-eabi-ar -t build/libs/libasn1pp-codec.a
arm-none-eabi-size build/libs/libasn1pp-arch.a
```

### What Gets Built

Two static archives are produced:

- `libasn1pp-codec.a` — BER, DER, PER, UPER, OER encoders and decoders
- `libasn1pp-arch.a` — Architecture detection, SIMD stubs (empty on ARM)

Both archives are standalone. Linking them into your project requires no additional libraries.

### Linking Into Your Firmware

```bash
arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
    -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections \
    -Wl,--gc-sections -nostartfiles -specs=nano.specs -specs=nosys.specs \
    -T cmake/stm32f407.ld \
    your_main.cpp \
    build/libs/libasn1pp-codec.a build/libs/libasn1pp-arch.a \
    -o firmware.elf
```

The `nano.specs` and `nosys.specs` linkerscripts replace the standard library with a minimal stub that has no file I/O and no memory allocation.

## Memory Footprint

Cross-compilation measurements on STM32F407 with `EMBEDDED=ON` and `NO_TEXT_CODECS=ON`:

| Section | Size (bytes) |
|---|---|
| .text | 24,248 |
| .data | 80 |
| .bss | 340 |
| **Total Flash** | **24,328 (23.8 KB)** |
| **Total RAM** | **420** |

The `.text` section covers all encoder and decoder code. The `.data` section holds initialized globals. The `.bss` section holds zero-initialized globals and the fixed decode/encode buffer.

### RAM Usage Breakdown

- **Fixed PDU buffer**: 2048 bytes (the max PDU size)
- **Internal decode/encode state**: ~400 bytes (varies by codec)
- **Stack usage per decode call**: ~64 bytes

In embedded mode, all buffers are caller-provided. The 420 bytes of static RAM covers the library's internal globals and does not include the PDU buffer that your code passes in.

## Linker Script

The project provides a linker script for STM32F407 at `cmake/stm32f407.ld`:

```ld
ENTRY(_start)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}
```

The entry point is `_start`. This symbol must be declared `extern "C"` and kept by the linker:

```cpp
extern "C" {
__attribute__((used)) void _start() {
    // your decode/encode calls here
    while (1) {} // bare-metal main loop
}
}
```

The `__attribute__((used))` prevents the linker from discarding the entry point when `--gc-sections` is active.

## Reference Implementation

A complete bare-metal demo is in `examples/bare-metal/`. It exercises every codec:

```cpp
#include "codec/config.hpp"
#include "codec/ber/decoder.hpp"
#include "codec/ber/encoder.hpp"
#include "codec/per/decoder.hpp"
#include "codec/per/encoder.hpp"
#include "codec/per/uper_decoder.hpp"
#include "codec/per/uper_encoder.hpp"
#include "codec/oer/decoder.hpp"
#include "codec/oer/encoder.hpp"

extern "C" {
__attribute__((used)) void _start() {
    // BER decode
    uint8_t ber_int[] = {0x02, 0x01, 0x2A};
    buffer_view bv(ber_int, sizeof(ber_int));
    ber::ber_decoder dec;
    auto r = dec.decode_integer(bv);

    // PER aligned encode
    uint8_t out[16]{};
    buffer_view obv(out, sizeof(out));
    per::per_aligned_encoder enc;
    enc.encode_integer<Int0To255>(42, obv);

    while (1) {}
}
}
```

The struct `Int0To255` is a minimal type descriptor for range-constrained integers:

```cpp
struct Int0To255 {
    static constexpr bool has_range_constraint = true;
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 255;
    static constexpr bool has_size_constraint = true;
    static constexpr bool extensible = false;
};
```

All decode and encode calls are zero-allocation. The `buffer_view` wraps caller-provided memory. Results are returned via `asn1pp::result` which encodes success/failure as a status code, never throwing.

## Build Configuration Options

| CMake Option | Default | Effect |
|---|---|---|
| `EMBEDDED` | OFF | Enables embedded profile, sets `ASN1PP_EMBEDDED` |
| `NO_TEXT_CODECS` | OFF | Excludes XER/JER, reduces text size |
| `ASN1PP_MAX_PDU` | 2048 | Maximum PDU size in bytes |
| `SIMD_SSE42` | ON (x86) | Enable SSE4.2 accelerations |
| `SIMD_AVX2` | ON (x86) | Enable AVX2 accelerations |
| `BUILD_TESTS` | ON | Build test suite |
| `BUILD_GEN` | ON | Build code generator |

For the smallest possible build, use:

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-stm32f407.cmake \
      -DEMBEDDED=ON -DNO_TEXT_CODECS=ON -DBUILD_TESTS=OFF -DBUILD_GEN=OFF
```

## Limitations in Embedded Mode

- **No text codecs**: XER and JER are not available when `NO_TEXT_CODECS=ON`.
- **Fixed PDU size**: PDUs larger than `ASN1PP_MAX_PDU` (default 2048) are rejected at decode time.
- **No dynamic allocation**: All encode/decode buffers must be provided by the caller.
- **No exceptions**: Errors are reported through `asn1pp::result::status`. Check the status before using the output.
- **No copy-on-write**: `buffer_view` does not copy data. The underlying storage must remain valid for the duration of the decode/encode call.