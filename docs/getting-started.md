---
title: Getting Started
---

# Getting Started with asn1pp

This guide walks you through installing asn1pp and encoding/decoding your first ASN.1 BER integer.

## Prerequisites

- C++20 compiler (GCC 12+, Clang 15+, MSVC 2022+)
- CMake 3.20 or later
- Git

## Installation

asn1pp is **not** a header-only library. It compiles as a static library from C++ source files. The recommended way to integrate asn1pp into your project is via CMake's FetchContent module.

### Option 1: FetchContent (Recommended)

Add this to your `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_asn1_app)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    asn1pp
    GIT_REPOSITORY https://github.com/aschokinatgmail/asn1pp.git
    GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(asn1pp)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE asn1pp-codec asn1pp-arch)
```

FetchContent downloads and builds asn1pp's static libraries automatically.

### Option 2: Git Submodule

```bash
git submodule add https://github.com/aschokinatgmail/asn1pp.git thirdparty/asn1pp
```

```cmake
add_subdirectory(thirdparty/asn1pp)
target_link_libraries(my_app PRIVATE asn1pp-codec asn1pp-arch)
```

## Your First Application

Create a file named `main.cpp` with the following code:

```cpp
#include <cstdint>
#include <cstddef>
#include <vector>
#include <iostream>

#include "codec/ber/encoder.hpp"
#include "codec/ber/decoder.hpp"
#include "buffer/buffer_view.hpp"

using namespace asn1pp;
using namespace asn1pp::ber;

int main() {
    // Encode integer 42 to BER
    ber_encoder enc;
    std::vector<uint8_t> buf(64);
    buffer_view view(buf.data(), buf.size());

    auto r = enc.encode_integer(42, view);
    if (r.is_err()) {
        std::cerr << "Encoding failed: " << r.error().message << "\n";
        return 1;
    }

    // r.value() is the number of bytes written
    std::cout << "Encoded " << r.value() << " bytes: ";
    for (size_t i = 0; i < r.value(); ++i) {
        std::printf("%02X ", buf[i]);
    }
    std::cout << "\n";

    // Decode it back
    ber_decoder dec;
    buffer_view decoded_view(buf.data(), r.value());
    auto dr = dec.decode_integer(decoded_view);

    if (dr.is_err()) {
        std::cerr << "Decoding failed: " << dr.error().message << "\n";
        return 1;
    }

    std::cout << "Decoded value: " << dr.value() << "\n";

    return 0;
}
```

Build and run:

```bash
mkdir build && cd build
cmake ..
cmake --build .
./my_app
```

Expected output:

```
Encoded 3 bytes: 02 01 2A
Decoded value: 42
```

The output shows the BER encoding of integer 42: `02` (tag for INTEGER), `01` (length of 1 byte), `2A` (42 in hex).

## Understanding buffer_view

`buffer_view` is a lightweight non-owning view into external memory. It does not allocate heap memory. The encoder writes directly into your buffer through the view.

```cpp
// Construct from pointer and size
buffer_view view(buffer, buffer_size);

// Or from a span-like container
std::array<uint8_t, 64> buf;
buffer_view view(buf);
```

Always ensure your buffer is large enough to hold the encoded output. The encoder returns the number of bytes written via `result<size_t>`.

## Error Handling

All encoder and decoder functions return `result<T>` where `T` is the output type. This is a custom result type, not `std::expected`. It provides:

```cpp
auto r = enc.encode_integer(42, view);

if (r.is_err()) {
    // Access error details
    std::cerr << "Error: " << r.error().message << "\n";
    std::cerr << "Code: " << r.error().code << "\n";
    return;
}

// Access the value
size_t bytes_written = r.value();
```

Similarly for decoding:

```cpp
auto r = dec.decode_integer(view);

if (r.is_err()) {
    std::cerr << "Decode error: " << r.error().message << "\n";
    return;
}

std::int64_t value = r.value();
```

## Complete CMakeLists.txt

Here is a complete CMakeLists.txt for a project using asn1pp:

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_asn1_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enforce zero warnings for production builds
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(FetchContent)
FetchContent_Declare(
    asn1pp
    GIT_REPOSITORY https://github.com/aschokinatgmail/asn1pp.git
    GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(asn1pp)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE asn1pp-codec asn1pp-arch)

# Required because asn1pp uses C++20 features
target_compile_features(my_app PRIVATE cxx_std_20)
```

## Next Steps

- [Building asn1pp from source](guides/building.md) - If you need to modify the library or use specific build options
- [BER Encoder API](api/ber.md) - Full API reference for BER encoding
- [BER Decoder API](api/ber.md) - Full API reference for BER decoding
- [Embedded Profile Guide](guides/embedded.md) - Using asn1pp on microcontrollers with limited resources

## Library Targets

When linking against asn1pp, you need two targets:

| Target | Purpose |
|--------|---------|
| `asn1pp-codec` | Encoding and decoding for BER, DER, PER, UPER, OER, XER, JER |
| `asn1pp-arch` | Architecture utilities, buffer types, memory management |

Both are static libraries. The codec library contains the actual implementation compiled from C++ source files.