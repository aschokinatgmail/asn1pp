---
title: SIMD Acceleration
---

# SIMD Acceleration Guide

asn1pp provides SIMD-accelerated batch operations for high-throughput encoding
and decoding of ASN.1 integer PDUs. The library automatically selects the best
available SIMD backend at runtime based on CPU feature detection.

## Overview

SIMD (Single Instruction Multiple Data) instructions allow processing multiple
data elements in parallel with a single instruction. For ASN.1 integer batch
operations, this translates to significant throughput gains when decoding or
encoding multiple integer values simultaneously.

The acceleration operates at the byte-array to integer conversion layer, where
asn1pp loads or stores multiple big-endian unsigned integers in a single call
using vector registers. Sign extension and error handling follow each batch
operation in scalar fallback code.

## Supported Backends

asn1pp supports four SIMD levels, ordered from most capable to least:

| Backend   | Target Arch    | Vector Width | Batch Size (ints) |
|-----------|----------------|--------------|------------------|
| SCALAR    | Any            | 64-bit       | 1                 |
| SSE4.2    | x86_64         | 128-bit      | 2                 |
| AVX2      | x86_64         | 256-bit      | 4                 |
| NEON      | ARMv8 AArch64  | 128-bit      | 2                 |

The maximum SIMD level is AVX2, chosen to ensure compatibility with mainstream
x86_64 CPUs and avoid the power consumption characteristics of wider ISA
extensions. AVX2 provides sufficient throughput for all supported use cases.

### SSE4.2 Backend

SSE4.2 (Streaming SIMD Extensions 4.2) is the baseline SIMD backend for x86_64
targets. It provides 128-bit vector registers (`XMM0`–`XMM15`) enabling batch
processing of 2 big-endian integers per call.

The backend uses `pmovzxbd` for zero-extending packed bytes and `pcmpgtb` for
sign detection. Compilation requires `-msse4.2` or detection via
`__builtin_cpu_supports("sse4.2")`.

### AVX2 Backend

AVX2 (Advanced Vector Extensions 2) extends the vector width to 256 bits
(`YMM0`–`YMM15`), allowing batch processing of 4 big-endian integers per call.
AVX2 is detected via `__builtin_cpu_supports("avx2")`.

The AVX2 backend requires SSE4.2 as a baseline ISA dependency (AVX2 implies
SSE4.2 at the hardware level). The Kconfig dependency chain enforces this.

### NEON Backend

ARM NEON provides 128-bit vector registers (`V0`–`V31`) for SIMD operations on
AArch64 targets. The backend processes 2 big-endian integers per call, matching
the SSE4.2 batch size but using different intrinsics (`vld1q_u64`, `vqtbl1q_u8`).

Runtime detection on Linux uses `getauxval(AT_HWCAP)` with `HWCAP_ASIMD`.
On macOS, `sysctlbyname("hw.optional.arm.FEAT_NEON")` provides equivalent
information.

### SCALAR Fallback

When no SIMD backend is available or runtime detection fails, asn1pp falls
back to scalar operations using plain C++ integer arithmetic. This ensures
correctness on any supported platform, albeit with lower throughput.

## Auto-Detection

asn1pp uses runtime CPU feature detection to select the optimal SIMD level.
The detection happens once, on first use, with the result cached for all
subsequent calls.

### Detection Mechanism

**x86_64 (Linux / macOS / Windows):**
```cpp
f.has_sse42 = __builtin_cpu_supports("sse4.2");
f.has_avx2  = __builtin_cpu_supports("avx2");
```
The `__builtin_cpu_supports` intrinsic uses `cpuid` instruction and is
evaluated at runtime, not compile time.

**ARM64 (Linux):**
```cpp
unsigned long hwcap = getauxval(AT_HWCAP);
f.has_neon = (hwcap & HWCAP_ASIN) != 0;
```
The `getauxval` function queries the kernel for hardware capabilities exposed
via the auxiliary vector.

**ARM64 (macOS):**
```cpp
sysctlbyname("hw.optional.arm.FEAT_NEON", &neon_present, ...);
```
Apple platforms do not expose NEON via `getauxval`; `sysctl` is the equivalent
mechanism.

### Caching

`available_simd_level()` uses `std::call_once` with a static local to ensure
thread-safe one-time initialization:

```cpp
inline simd_level available_simd_level() {
    static simd_level cached = simd_level::scalar;
    static std::once_flag flag;
    std::call_once(flag, [&] {
        auto features = detect_cpu();
        if (features.has_avx2)       cached = simd_level::avx2;
        else if (features.has_sse42) cached = simd_level::sse42;
        else if (features.has_neon)  cached = simd_level::neon;
        else                         cached = simd_level::scalar;
    });
    return cached;
}
```

The cached value is returned for all subsequent calls without additional
locking or `cpuid` overhead.

## Batch Operations

### batch_decode_integers

Decodes multiple ASN.1 integer PDUs from big-endian wire format to signed
`int64_t` values in a single call:

```cpp
void batch_decode_integers(const uint8_t* const* bufs,
                           const size_t* sizes,
                           int64_t* out,
                           error_code* errors,
                           size_t count) noexcept;
```

The function:
1. Calls `arch::batch_load_u64_be()` to load raw bytes into vector registers
2. Applies sign extension based on the most significant bit of each integer
3. Writes results to the output array, set `errors[i]` for invalid entries

For AVX2, up to 4 integers are processed per iteration. For SSE4.2 and NEON,
2 integers are processed per iteration.

### batch_encode_integers

Encodes multiple signed `int64_t` values to big-endian wire format:

```cpp
void batch_encode_integers(uint8_t** ptrs,
                            const int64_t* values,
                            const size_t* sizes,
                            size_t count) noexcept;
```

The function mirrors the decode path: sign bits are checked to determine
whether to emit leading `0x00` or `0xFF` bytes, then `arch::batch_store_u64_be()`
writes the remaining bytes.

### batch_load_u64_be / batch_store_u64_be

Low-level primitives that load or store multiple big-endian unsigned integers
using the best available SIMD backend:

```cpp
void batch_load_u64_be(const uint8_t* const* ptrs,
                       const size_t* sizes,
                       uint64_t* out,
                       size_t count) noexcept;

void batch_store_u64_be(uint8_t** ptrs,
                         const uint64_t* values,
                         const size_t* sizes,
                         size_t count) noexcept;
```

These are dispatched through `simd.hpp` based on `available_simd_level()`.

## KConfig Options

SIMD acceleration can be disabled or limited at compile time via KConfig
options in `arch/Kconfig`:

### SIMD_SSE42

```
CONFIG SIMD_SSE42: bool (default ON)
```
Enable SSE4.2 SIMD acceleration for x86_64 targets. When disabled, SSE4.2
intrinsics are not compiled and the backend is unavailable even if the CPU
supports it.

### SIMD_AVX2

```
CONFIG SIMD_AVX2: bool (default ON)
depends on SIMD_SSE42
```
Enable AVX2 SIMD acceleration for x86_64 targets. The `depends on` constraint
ensures AVX2 builds always have SSE4.2 as a fallback. When disabled, the AVX2
backend is not compiled.

### SIMD_NEON

```
CONFIG SIMD_NEON: bool (default ON)
```
Enable ARM NEON SIMD acceleration for AArch64 targets. On x86_64 builds,
this option has no effect. When disabled, NEON intrinsics are not compiled.

## Disabling SIMD

If SIMD causes issues (e.g., binary compatibility with older CPUs, debugging
interference, or regulatory restrictions on certain instruction sets), you can
disable acceleration at the KConfig level:

```bash
# Disable all SIMD (forces SCALAR)
cd build
make menuconfig
# Navigate to "SIMD Acceleration" and disable all backends

# Disable only AVX2 (keep SSE4.2)
# Set SIMD_AVX2=OFF while SIMD_SSE42=ON
```

After disabling, rebuild to ensure no SIMD intrinsics are compiled into the
binary. The `compile_level` constant in `simd.hpp` will reflect the maximum
available level based on compile-time macros and KConfig settings.

Disabling SIMD does not affect runtime detection—it simply removes the
code paths for disabled backends. If runtime detection reports an unavailable
level, the next available level is used (or SCALAR as final fallback).

## Performance Notes

- Batch operations are most effective when processing 8 or more integers
  concurrently. For single-integer PDUs, scalar code may be faster due to
  SIMD dispatch overhead.
- AVX2 requires operating system support for `XSAVE`/`XRSTOR` context
  management. On Linux, kernel FPU state handling must be enabled.
- ARM NEON on macOS AArch64 benefits from Apple Silicon unified memory
  architecture but may show variance depending on memory pressure.