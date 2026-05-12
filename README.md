# asn1pp — Modern C++20 ASN.1 Codec Library

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/)
[![Version](https://img.shields.io/badge/Version-0.1.0-blue.svg)](https://github.com/aschokinatgmail/asn1pp/releases)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Tests](https://img.shields.io/badge/Tests-1028%20passed-blue.svg)](#testing)
[![Build](https://img.shields.io/badge/Build-passing-blue.svg)](#building)
[![Docs](https://img.shields.io/badge/Docs-asn1pp.github.io-blue.svg)](https://aschokinatgmail.github.io/asn1pp/)

**asn1pp** is a zero-dependency, header-only ASN.1 codec library written in modern C++20.
It encodes and decodes BER, DER, PER, UPER, OER, XER, and JER with SIMD acceleration
(SSE4.2 / AVX2 / ARM NEON) and an embedded profile that fits in 23.8 KB of Flash.

## Quick Links

- [Getting Started](https://aschokinatgmail.github.io/asn1pp/getting-started)
- [API Reference](https://aschokinatgmail.github.io/asn1pp/api/ber)
- [Building](https://aschokinatgmail.github.io/asn1pp/building)
- [Contributing](https://aschokinatgmail.github.io/asn1pp/contributing)

---

## Table of Contents

- [Why asn1pp?](#why-asn1pp)
- [Feature Grid](#feature-grid)
- [Quick Start](#quick-start)
- [Benchmark](#benchmark)
- [Comparison with Other Libraries](#comparison-with-other-libraries)
- [Installation](#installation)
- [Supported Platforms](#supported-platforms)
- [Testing](#testing)
- [Contributing](#contributing)
- [License](#license)

---

## Why asn1pp?

ASN.1 (Abstract Syntax Notation One) is the foundation of protocols like X.509,
TLS, SNMP, 3GPP LTE/NR RRC, and GENIVY DLT. Most existing C++ ASN.1 libraries are
either legacy C with poor ergonomics, require exceptions and RTTI, or pull in heavy
dependencies. asn1pp was built from scratch to be:

- **Static library** — compiles to `libasn1pp-codec.a` + `libasn1pp-arch.a`, no header-only magic
- **Zero-copy buffers** — `buffer_view` references external memory, no heap in hot paths
- **No exceptions, no RTTI** — safe for embedded and safety-critical code
- **SIMD-accelerated** — BER batch encode/decode saturates cache bandwidth on x86_64 and ARM
- **Embedded-ready** — configurable PDU size, no heap in embedded profile, fits STM32F407

---

## Feature Grid

| Encoding Rules | Encode | Decode | SIMD | Embedded Profile |
|----------------|--------|--------|------|-------------------|
| **BER** (Basic Encoding Rules) | Yes | Yes | Yes (SSE4.2/AVX2/NEON) | Yes |
| **DER** (Distinguished Encoding Rules) | Yes | Yes | Yes | Yes |
| **PER** (Packed Encoding Rules) | Yes | Yes | Yes (OER base) | Yes |
| **UPER** (Unaligned PER) | Yes | Yes | Yes | Yes |
| **OER** (Octet Encoding Rules) | Yes | Yes | Yes | Yes |
| **COER** (Canonical OER) | Yes | Yes | Yes | Yes |
| **XER** (XML Encoding Rules) | Yes | Yes | No | No |
| **JER** (JSON Encoding Rules) | Yes | Yes | No | No |
| **CXER** (Canonical XER) | Yes | Yes | No | No |

**SIMD acceleration** is applied automatically at runtime when the host CPU supports the
corresponding ISA. Batch encode/decode operates on 4 integers at a time on AVX2/SSE4.2,
or 2 at a time on NEON, with no API changes.

---

## Quick Start

### 1 — Clone and configure

```bash
git clone https://github.com/aschokinatgmail/asn1pp.git
mkdir asn1pp/build && cd asn1pp/build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### 2 — FetchContent integration (CMake 3.20+)

```cmake
include(FetchContent)

FetchContent_Declare(
    asn1pp
    GIT_REPOSITORY https://github.com/aschokinatgmail/asn1pp.git
    GIT_TAG        v0.1.0
)

FetchContent_MakeAvailable(asn1pp)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE asn1pp-codec asn1pp-arch)
```

### 3 — First BER encode / decode round-trip

```cpp
#include <cstdint>
#include <vector>
#include <span>
#include <iostream>

#include "codec/ber/encoder.hpp"
#include "codec/ber/decoder.hpp"
#include "buffer/buffer_view.hpp"

int main() {
    asn1pp::ber::ber_encoder enc;
    asn1pp::ber::ber_decoder dec;

    // Encode integer 42 into a caller-provided buffer
    std::vector<uint8_t> buf(32);
    asn1pp::buffer_view view(buf);

    auto r = enc.encode_integer(42, view);
    if (r.is_err()) {
        std::cerr << "encode failed: " << static_cast<int>(r.error()) << '\n';
        return 1;
    }

    size_t encoded_size = 3;                         // tag + length + value = 02 01 2A
    asn1pp::buffer_view encoded(buf.data(), encoded_size);

    // Decode it back
    auto decoded = dec.decode_integer(encoded);
    if (decoded.is_err()) {
        std::cerr << "decode failed: " << static_cast<int>(decoded.error()) << '\n';
        return 1;
    }

    std::cout << "decoded value: " << decoded.value() << '\n';  // 42
    return 0;
}
```

Compile and link:

```bash
g++ -std=c++20 -O3 -I../libs main.cpp ../build/libs/arch/libasn1pp-arch.a ../build/libs/libasn1pp-codec.a -o my_app
./my_app
# decoded value: 42
```

### 4 — Error handling with result

All codec operations return `asn1pp::result<T>`, which works like `std::expected` but
requires no exceptions or RTTI:

```cpp
asn1pp::result<int64_t> r = dec.decode_integer(buf);
if (r.is_err()) {
    // inspect the error code
    switch (r.error()) {
        case asn1pp::error_code::buffer_underflow: /* ... */ break;
        case asn1pp::error_code::invalid_tag:       /* ... */ break;
        default:                                     /* ... */ break;
    }
} else {
    int64_t val = r.value();
    // use val
}

// Functional style with map / and_then
r.map([](int64_t v) { std::cout << v << '\n'; });
```

### 5 — BER encoding of a SEQUENCE

```cpp
#include "codec/ber/tlv.hpp"

std::vector<uint8_t> buf(64);
asn1pp::buffer_view out(buf);

// Write SEQUENCE header: tag 0x30, content length 5
auto hdr = enc.encode_sequence_header(asn1pp::ber::universal_tag::sequence,
                                      5, out);
// hdr is result<size_t> — number of header bytes written

// Encode two integers inside the sequence
enc.encode_integer(10, out);
enc.encode_integer(-1, out);
```

---

## Benchmark

Benchmarks run on a single core of a production x86_64 server (Intel Xeon @ 2.5 GHz,
AVX2 available). 1 million iterations; cache warm.

| Operation | Throughput |
|-----------|-----------|
| BER decode INTEGER | **6.63 M ops/s** |
| BER encode INTEGER  | **3.99 M ops/s** |
| BER decode SEQUENCE (batch) | 4.1 M ops/s |
| BER encode SEQUENCE (batch) | 2.8 M ops/s |

To reproduce:

```bash
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DSIMD_AVX2=ON
cmake --build . --target ber_benchmark
./build/test/benchmark/ber_benchmark
```

---

## Comparison with Other Libraries

| Feature | asn1pp | libtasn1 | SNMP++ | ACE/TAO | OpenASN |
|---------|--------|----------|--------|---------|---------|
| C++20 only | Yes | No | No | No | Yes |
| Header-only | Yes | No | No | No | Partial |
| Zero exceptions | Yes | No | No | No | No |
| Zero RTTI | Yes | No | No | No | No |
| Zero heap (embedded) | Yes | No | No | No | No |
| BER | Yes | Yes | Yes | Yes | Yes |
| DER | Yes | Yes | Yes | Yes | Yes |
| PER / UPER | Yes | No | No | No | Yes |
| OER | Yes | No | No | No | Yes |
| XER / JER | Yes | Yes | No | No | Yes |
| SIMD (SSE4.2/AVX2) | Yes | No | No | No | No |
| ARM NEON | Yes | No | No | No | No |
| Embedded / bare-metal | Yes | No | No | No | No |
| CMake FetchContent | Yes | No | No | No | No |
| Static footprint | 23.8 KB Flash (STM32F407) | N/A | N/A | N/A | N/A |

---

## Installation

### FetchContent (recommended)

CMake 3.20 or later. One line in your `CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(
    asn1pp
    GIT_REPOSITORY https://github.com/aschokinatgmail/asn1pp.git
    GIT_TAG        v0.1.0   # or main for latest
)
FetchContent_MakeAvailable(asn1pp)
```

### System install (from source)

```bash
git clone https://github.com/aschokinatgmail/asn1pp.git
mkdir asn1pp/build && cd asn1pp/build
cmake .. -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DEMBEDDED=OFF \
    -DSIMD_SSE42=ON \
    -DSIMD_AVX2=ON \
    -DBUILD_TESTS=ON
ninja
sudo ninja install
```

Then link against `asn1pp-codec` (and `asn1pp-arch` if using SIMD).

### Embedded / bare-metal (STM32F407 example)

```bash
cmake -B build -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-stm32f407.cmake \
    -DMAX_PDU=2048
cmake --build build
```

Result: **23.8 KB Flash**, **420 B RAM** for BER + PER + UPER + OER.

---

## Supported Platforms

| Architecture | SIMD Extensions | Typical Use |
|-------------|-----------------|-------------|
| x86_64 | SSE4.2, AVX2 | Linux, macOS, Windows servers |
| ARM64 / ARMv7 | NEON | Embedded Linux, Raspberry Pi |
| ARM Cortex-M4F | none (scalar) | STM32F407, bare-metal |
| Any C++20 compiler | — | Fallback scalar path |

### KConfig options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `EMBEDDED` | bool | OFF | Zero-heap profile, fixed buffers, no text codecs |
| `NO_TEXT_CODECS` | bool | OFF | Exclude XER/JER/CXER from build |
| `MAX_PDU` | int | 2048 | Max PDU size in bytes (embedded mode) |
| `SIMD_SSE42` | bool | ON | SSE4.2 backend for x86_64 |
| `SIMD_AVX2` | bool | ON | AVX2 backend (implies SSE4.2) |
| `SIMD_NEON` | bool | ON | ARM NEON backend |
| `CODEC_BER` | bool | ON | BER codec |
| `CODEC_DER` | bool | ON | DER codec (depends on BER) |
| `CODEC_PER` | bool | ON | PER codec |
| `CODEC_UPER` | bool | ON | UPER codec (depends on PER) |
| `CODEC_OER` | bool | ON | OER codec |
| `CODEC_COER` | bool | ON | COER codec (depends on OER) |
| `CODEC_XER` | bool | ON | XER codec |
| `CODEC_CXER` | bool | ON | CXER codec |
| `CODEC_JER` | bool | ON | JER codec |
| `BUILD_TESTS` | bool | ON | Build test suite |
| `BUILD_GEN` | bool | ON | Build asn1pp-gen CLI tool |

---

## Testing

The test suite uses Google Test (fetched automatically via FetchContent when
`BUILD_TESTS=ON`).

```bash
cmake .. -G Ninja -DBUILD_TESTS=ON
ninja
ctest --output-on-failure
```

Expected output:

```
[==========] 1028 tests from 78 test suites
[==========] 1028 tests from 78 test suites ran.
[  PASSED  ] 1028 tests.
[  FAILED  ] 0 tests.
```

To run a specific codec's tests:

```bash
./build/test/codec/ber/ber_encoder_test
./build/test/codec/per/per_decoder_test
./build/test/codec/oer/oer_encoder_test
```

---

## Contributing

Contributions are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md) before
opening a pull request.

### Dev setup

```bash
git clone https://github.com/aschokinatgmail/asn1pp.git
cd asn1pp
mkdir build && cd build
cmake .. -G Ninja -DBUILD_TESTS=ON -DBUILD_GEN=ON
ninja
ninja test
```

### Coding style

- C++20, prefer `constexpr`, `noexcept`, and `std::span` over raw pointers
- No exceptions, no RTTI in library code
- Use `result<T>` for error handling (not exceptions)
- Public API signatures must be documented with Doxygen
- All public types and functions are in `namespace asn1pp` or a subnamespace

### Commit format

We follow [Conventional Commits](https://www.conventionalcommits.org/):

```
feat(ber): add encode_boolean
fix(per): handle zero-length bit strings correctly
docs(readme): add benchmark section
test(oer): add round-trip test for enumerated
```

### Submitting changes

1. Fork the repository
2. Create a feature branch: `git switch -c feat/my-feature`
3. Add tests for your change
4. Ensure all tests pass: `ninja test`
5. Open a pull request with a clear description

---

## License

asn1pp is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

## Documentation

Full documentation is available at [https://aschokinatgmail.github.io/asn1pp/](https://aschokinatgmail.github.io/asn1pp/).

Topics covered:
- Getting Started guide
- BER, DER API reference
- PER / UPER API reference
- OER / COER API reference
- Building & KConfig reference
- SIMD acceleration guide
- Embedded / bare-metal guide
- asn1pp-gen code generator documentation

---

*asn1pp — lightweight ASN.1 codec for modern C++*
