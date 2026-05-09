# Learnings — asn1pp-full

## Architecture Decisions
- Code Generator approach (not header-only library)
- C++20 with all modern features (concepts, std::span, constexpr, std::expected)
- Hand-written recursive descent parser (no ANTLR/PEGTL)
- Custom buffer_view with bit-level operations (not std::span)
- Concepts + runtime for constraint validation
- Separate CMake targets: libasn1pp-codec, asn1pp-gen, asn1pp-cmake
- TDD with Google Test + GMock
- CRTP + concepts for codec dispatch (no vtable, no RTTI, no exceptions)
- DER is wave-1 encoding rule (testable against X.509)
- IOC deferred to Wave 9 after core pipeline proven

## Code Conventions
- No RTTI, no exceptions in runtime codec library
- No macro-based code generation
- No heap allocations in hot paths
- Each encoding rule is independent — no hidden coupling
- TDD: RED → GREEN → REFACTOR for every deliverable
- All files: #pragma once for headers
- Namespace: asn1pp (runtime), asn1pp::gen (code generator)
- Result type for error handling (not exceptions)

## Build System
- CMake 3.20+
- C++20 standard required
- FetchContent for Google Test (v1.14.0)
- Three targets: libasn1pp-codec (static lib), asn1pp-gen (executable), asn1pp-cmake (cmake module)
- Compiler flags: -Wall -Wextra -Wpedantic -Werror (GCC/Clang), /W4 /WX (MSVC)
## Task 1: Project Scaffold (2025-05-10)
- Created 3 CMake targets: asn1pp-codec (static lib), asn1pp-gen (executable), asn1pp-cmake (module)
- All subdirectories with .gitkeep files: libs/buffer, libs/codec/*, src/gen, test/*, cmake
- CMakeLists.txt uses modern target_* conventions (no global includes, no add_definitions)
- Placeholder .cpp files created for each source entry (empty files for now)
- compile_commands.json generation enabled via CMAKE_EXPORT_COMPILE_COMMANDS
- Compiler flags work for both AppleClang (using -Wall -Wextra -Wpedantic -Werror) and MSVC
- Build verified: configure + compile both succeed

## Task 3: buffer_view Implementation (2026-05-10)
- **TDD strictness**: RED phase confirmed via `fatal error: 'buffer/buffer_view.hpp' file not found`; GREEN phase required header + 33 tests all passing
- **GTest sign-compare issue**: AppleClang with `-Werror` triggers `-Wsign-compare` on `EXPECT_EQ(size_t, int_literal)` inside GTest macros; fixed by adding `target_compile_options(${test_name} PRIVATE -Wno-sign-compare)` to `asn1pp_add_test` function in test/CMakeLists.txt
- **buffer_view design**: Pure value type — `const uint8_t* data_` + `size_t size_` = exactly 2 pointers; `[[nodiscard]]` on all query methods; `noexcept` on all methods; `explicit` constructors from span/vector; `assert()` for bounds in debug, UB in release
- **Bit access**: MSB-first convention (bit 0 = most significant bit of byte 0), critical for ASN.1 PER/UPER; `bit_span()` packs up to 64 bits into `uint64_t` via struct return
- **Zero-copy**: `subview()` returns new buffer_view pointing into same memory; `slice_at()` splits without allocation via `std::pair`
- **static_assert verification**: `std::is_trivially_copyable_v<buffer_view>` and `sizeof(buffer_view) == 2 * sizeof(void*)` both compile-verified

### Files created/modified:
  - `libs/buffer/buffer_view.hpp` (NEW) — public header, automatically included via `target_include_directories(asn1pp-codec PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})`
  - `test/buffer/buffer_view_test.cpp` (NEW) — 25 test cases covering construction, access, slicing, bit ops, conversion, iteration, copy, comparison
  - `test/CMakeLists.txt` (MODIFIED) — added `buffer_view_test` registration + `-Wno-sign-compare` for test targets

### Verified:
  - `cmake --build build` — zero warnings on asn1pp-codec sources
  - `ctest --test-dir build --output-on-failure` — 34/34 passing (1 smoke + 33 buffer_view)
  - LSP diagnostics — clean on both .hpp and .cpp files

## Task 4: Core Type Traits (2026-05-10)
- **GTest macro comma issue**: `EXPECT_TRUE(encodable_v<T, E>)` breaks because commas split macro args. Fix: use parentheses `EXPECT_TRUE((encodable_v<T, E>))` or `not` operator in static_asserts: `static_assert(not encodable_v<int, MockEncoder>)`
- **Duplicate template specializations**: When files have duplicate `template<> struct X<Y>`, compiler emits redefinition error. Remove duplicates carefully.
- **`has_tag` concept design**: Uses `is_specialized` boolean member in `asn1_tag<T>` to distinguish default (unspecialized) from user-specialized. Primary template has `is_specialized = false`, specializations set it to `true`.
- **Concept parameter order**: `encoder_for<E, T>` (encoder first, type second) matches concept usage pattern, but variable templates `encodable_v<T, E>` put type first for ergonomic use.
- **Only GTest warning**: `gtest-printers.h` char8_t conversion warning — from GTest headers, not our code. Already suppressed via `-Wno-error=character-conversion`.

## Task N: bit_ops Implementation (2026-05-10)
- **bit_ops.hpp**: Free functions for bit/octet manipulation in MSB-first order
- **write_bits**: Writes bit_count LSBs of value at bit_offset, MSB first, advances bit_offset
- **read_bits**: Reads bit_count bits MSB first from bit_offset, returns value, advances bit_offset  
- **write_octets/read_octets**: Simple byte copies with offset advancement
- **align_to_octet**: Advances bit_offset to next byte boundary (not zeroing!)
- **remaining_bits/remaining_octets**: Returns remaining readable bits/bytes from current offset
- **MSB-first**: bit 0 of byte 0 is MSB (bit 7 in LSB notation) — same as buffer_view::bit_at
- **TDD lessons**: Tests revealed misunderstanding of align_to_octet semantics; tests must match implementation semantics

## Task N: ASN.1 Universal Tag Registry (2026-05-10)
- **X.680 Amendment 2 tags 31-36**: Added date(31), time_of_day(32), datetime(33), duration(34), rel_uri(35), rel_uris(36)
- **is_primitive()**: Added relative_oid (was missing), date, time_of_day, datetime
- **is_constructed()**: Added duration, rel_uris as constructed types; rel_uri is primitive
- **universal_tag_name()**: Added name entries for all 6 new tags
- **tag_test.cpp**: New dedicated test file for tag registry covering:
  - All X.680 Table 1 + Amendment 2 universal tag values
  - tag_to_name round-trip for every tag
  - is_primitive/is_constructed for all types
  - tag struct construction, comparison, equality
  - make_universal/make_context_specific
  - tag_class enum values
  - reserved tags 14, 15 verification (not in enum - correct)
  - name string verification
- **Verification**: 124/124 tests pass, 0 warnings, LSP clean
