# asn1pp Architecture

This document covers the deep technical architecture of asn1pp. For a higher-level introduction, see [../README.md](../README.md).

## Overview

asn1pp is a two-component system:

1. **`libasn1pp-codec`** — Runtime library providing encoding/decoding for all ITU-T encoding rules
2. **`asn1pp-gen`** — CLI tool that parses ASN.1 schemas and generates C++20 type definitions

The runtime library is header-only, requires no exceptions or RTTI, and avoids heap allocations in hot paths.

## Buffer Design

### buffer_view

`libs/buffer/buffer_view.hpp` provides a zero-copy buffer abstraction:

```cpp
class buffer_view {
    const uint8_t* data_;
    size_t size_;
};
```

Key operations:
- `subview(offset, length)` — create a view into a portion of the buffer
- `slice_at(offset)` — split buffer into two views at offset
- `bit_at(bit_offset)` — read single bit (MSB-first per ASN.1 conventions)
- `bit_span(bit_offset, bit_count)` — read up to 64 bits as a value

The `buffer_view` never owns data. It references external memory, making it safe to pass around without copying.

### bit_ops.hpp

`libs/buffer/bit_ops.hpp` provides low-level bit manipulation:

- `write_bits(buf, offset, value, count)` — write bits MSB-first
- `read_bits(buf, offset, count)` — read bits MSB-first
- `align_to_octet(offset)` — advance offset to next octet boundary

These operations work with raw pointers and are marked `constexpr` for compile-time evaluation where possible.

### Scatter-Gather

The buffer design supports scatter-gather I/O patterns. A `buffer_view` can reference non-contiguous memory regions through multiple views, though the current implementation uses a single contiguous span. This design choice keeps the API simple while remaining compatible with future scatter-gather extensions.

## Type Traits and Concepts

`libs/codec/traits.hpp` defines the type system for ASN.1 codecs:

### Tag System

ASN.1 uses tags (class, constructed flag, number) to identify types. The `tag` struct represents this:

```cpp
struct tag {
    tag_class cls;      // universal, application, context_specific, private
    bool constructed;  // primitive vs constructed encoding
    uint32_t number;    // tag number
};
```

The `universal_tag` enum covers all standard universal tag values:

```cpp
enum class universal_tag : uint32_t {
    boolean = 1,
    integer = 2,
    bit_string = 3,
    // ...
};
```

### Type-to-Tag Mapping

The `asn1_tag<T>` template specialization maps C++ types to ASN.1 tags:

```cpp
template<> struct asn1_tag<int64_t> {
    static constexpr auto value = universal_tag::integer;
    static constexpr bool is_specialized = true;
};
```

### Concepts

C++20 concepts define codec interfaces:

```cpp
template<typename T>
concept asn1_type = requires { typename T::asn1_type_tag; } || has_tag_v<T>;

template<typename T>
concept has_tag = requires {
    { asn1_tag<T>::is_specialized } -> std::convertible_to<bool>;
    { asn1_tag<T>::value } -> std::convertible_to<universal_tag>;
} && asn1_tag<T>::is_specialized;
```

These concepts enable compile-time validation that types meet ASN.1 requirements.

## Codec Dispatch (CRTP + Concepts)

Codecs use CRTP (Curiously Recurring Template Pattern) with C++20 concepts to avoid virtual dispatch overhead:

### Encoder Pattern

```cpp
template<typename Derived, typename T>
class encoder_base {
    result<size_t> encode(const T& value, buffer_view& buf) {
        return static_cast<Derived*>(this)->do_encode(value, buf);
    }
};
```

The `per_aligned_encoder` class demonstrates this pattern. It tracks a bit offset for PER encoding and provides methods for each ASN.1 type:

```cpp
class per_aligned_encoder {
    size_t bit_offset_ = 0;

public:
    result<void> encode_integer(int64_t value, buffer_view& buf);
    result<void> encode_boolean(bool value, buffer_view& buf);
    result<void> encode_octet_string(std::span<const uint8_t> data, buffer_view& buf);
    // ...
};
```

Constraint metadata comes from `PerMeta` template parameters:

```cpp
template<typename PerMeta>
result<void> encode_integer(int64_t value, buffer_view& buf);
```

### No Virtual Tables

By using CRTP and concepts instead of inheritance, codec implementations avoid virtual function overhead. Each encoding rule has its own class with specific methods. Users select the codec by choosing the encoder type, not by runtime polymorphism.

## Result Type

Error handling uses `result<T>` (similar to `std::expected` but custom for compatibility):

```cpp
template<typename T>
struct result {
    bool is_ok() const;
    bool is_err() const;
    T value();
    error_code error();
    static result ok(T v);
    static result err(error_code e);
};
```

No exceptions are thrown in the runtime library. Errors propagate through return values, making error handling explicit and compile-time checked.

## Code Generator Pipeline

`src/gen/` contains the ASN.1 parser and code emitter.

### Pipeline Stages

1. **Lexer** (`lexer.cpp`, `lexer.hpp`) — Tokenizes ASN.1 schema text into tokens (keywords, identifiers, symbols)
2. **Parser** (`parser.cpp`, `parser.hpp`) — Hand-written recursive descent parser builds an AST
3. **Semantic Analysis** — Validates module structure, type references, constraint syntax
4. **Emitters** — Generate C++ code from the AST for each ASN.1 type

### Hand-Written Parser

The parser is hand-written recursive descent, not generated by ANTLR, Bison, or similar tools. This keeps build requirements minimal and gives full control over error reporting and recovery.

The grammar follows ASN.1 X.680 concrete syntax. The parser handles:
- Module definitions
- Type declarations
- Value assignments
- Constraint expressions
- Information object classes (partial, Wave 10)

### AST Structure

`src/gen/ast.hpp` defines the abstract syntax tree:

```cpp
struct TypeDefinition {
    std::string name;
    std::variant<IntegerType, SequenceType, ChoiceType, ...> type;
};

struct Module {
    std::string name;
    std::vector<TypeDefinition> types;
};
```

### Emitters

Each emitter generates C++ for a specific ASN.1 type:

- `emitter_boolean` — BOOLEAN
- `emitter_integer` — INTEGER (with constraint metadata)
- `emitter_string` — OCTET STRING, BIT STRING, character strings
- `emitter_sequence` — SEQUENCE
- `emitter_choice` — CHOICE (std::variant + which enum)
- `emitter_enumerated` — ENUMERATED
- `emitter_oid` — OBJECT IDENTIFIER
- `emitter_sequence_of` — SEQUENCE OF, SET OF
- `emitter_per_meta` — PER constraint metadata (PerMeta structs)
- `emitter_time` — time types (UTCTime, GeneralizedTime)

The `code_emitter` base class provides common utilities:

```cpp
class code_emitter {
    static std::string emit_header_guard(std::string_view name);
    static std::string emit_namespace_open(const std::string& ns);
    static std::string emit_namespace_close(const std::string& ns);
};
```

## Constraint Handling

PER encoding requires constraint information for efficient encoding. The code generator emits `PerMeta` structs alongside types:

```cpp
// Generated for: MyInt ::= INTEGER (0..255)
struct PerMeta_MyInt {
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 255;
    static constexpr bool has_constraint = true;
};
```

The encoder uses this metadata at compile time (via template parameters) to determine bit counts:

```cpp
template<typename PerMeta>
result<void> per_aligned_encoder::encode_integer(int64_t value, buffer_view& buf) {
    // Uses PerMeta::min_value and PerMeta::max_value to calculate bits
    // ...
}
```

For BER/DER, no constraint metadata is needed since those rules use self-describing encoding.

## Error Handling Strategy

The runtime library uses a consistent error strategy:

1. **No exceptions** — Errors propagate through `result<T>`
2. **No RTTI** — Type information comes from templates and concepts
3. **Explicit error codes** — `error_code` enum lists all possible failures

```cpp
enum class error_code {
    success = 0,
    buffer_overflow,
    value_out_of_range,
    invalid_tag,
    truncated_data,
    // ...
};
```

This design compiles to efficient code suitable for embedded systems.

## CMake Integration

`cmake/asn1pp.cmake` provides `asn1pp_generate()`:

```cmake
function(asn1pp_generate)
    set(options)
    set(one_value_args TARGET SCHEMA OUTPUT_DIR)
    cmake_parse_arguments(ASN1PP_GEN "${options}" "${one_value_args}" ... ${ARGN})

    # TODO: Invoke asn1pp-gen with SCHEMA file
    # Currently a placeholder; full implementation in progress
endfunction()
```

Users include this module and call the function to integrate code generation into their build:

```cmake
find_package(asn1pp REQUIRED)
asn1pp_generate(TARGET myapp SCHEMA schema.asn)
```

## Test Infrastructure

Tests use Google Test with GMock. The test directory structure mirrors the source:

```
test/
├── buffer/
│   ├── buffer_view_test.cpp
│   └── bit_ops_test.cpp
├── codec/
│   ├── traits_test.cpp
│   ├── ber/
│   │   ├── encoder_test.cpp
│   │   └── decoder_test.cpp
│   └── per/
│       ├── per_encoder_test.cpp
│       └── uper_decoder_test.cpp
└── gen/
    ├── lexer_test.cpp
    ├── parser_test.cpp
    └── emitter_*.cpp
```

The `asn1pp_add_test()` CMake function links tests to `asn1pp-codec`:

```cmake
function(asn1pp_add_test test_name)
    add_executable(${test_name} ${sources})
    target_link_libraries(${test_name} PRIVATE asn1pp-codec GTest::gtest ...)
endfunction()
```

## Build System

CMake 3.20+ is required for C++20 features. Key build options:

- `CMAKE_CXX_STANDARD=20` — C++20 required
- `CMAKE_EXPORT_COMPILE_COMMANDS=ON` — IDE support
- Warnings as errors: `-Wall -Wextra -Wpedantic -Werror`

Three subdirectories define targets:

- `libs/` — `asn1pp-codec` (codec library)
- `src/` — `asn1pp-gen` (code generator CLI)
- `cmake/` — module for `find_package(asn1pp)`
- `test/` — test executables

## Component Dependencies

```
asn1pp-gen (CLI)
  └── lexer.cpp, parser.cpp, emitter_*.cpp, ast.hpp

asn1pp-codec (library)
  ├── buffer/buffer_view.hpp, bit_ops.hpp
  ├── codec/traits.hpp, result.hpp
  ├── codec/ber/encoder.hpp, decoder.hpp
  └── codec/per/encoder.hpp, decoder.hpp

asn1pp-cmake (CMake module)
  └── cmake/asn1pp.cmake
```

The codec library is header-only. The generator tool is compiled once and reused.

## Standards Compliance

asn1pp implements these ITU-T recommendations:

- **X.680** — ASN.1 abstract syntax notation (module structure, type system)
- **X.681** — ASN.1 information object classes (partial)
- **X.690** — BER, DER, CER encoding rules
- **X.691** — PER (aligned and unaligned) encoding rules
- **X.693** — XER, CXER, E-XER encoding rules (planned)
- **X.696** — OER, COER encoding rules (planned)
- **X.697** — JER encoding rules (planned)

For the ASN.1 standard itself, refer to [ITU-T X.680](https://www.itu.int/rec/T-REC-X.680/).