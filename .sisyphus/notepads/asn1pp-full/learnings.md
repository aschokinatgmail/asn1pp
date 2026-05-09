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

## Task: Codec Interface Traits (2026-05-10)
- **Concept conflict resolution**: `traits.hpp` had placeholder `encoder_for`/`decoder_for`/`codec_for` concepts returning `void`. Replaced with result-based concepts in `codec_interface.hpp`. Removed old concepts and `encodable_v`/`decodable_v`/`codec_for_v` variable templates from `traits.hpp`.
- **Concept semantics change**: `codec_for` now requires BOTH `encoder_for AND decoder_for` (was `||` before). A type that only encodes or only decodes is NOT a full codec.
- **`decoder_for` signature**: Uses `result<T> decode(buffer_view&)` (non-template, returns `result<T>`), not `void decode(buffer_view&, T&)`. Template decoders that require explicit type parameter do NOT match.
- **`buffer_view` as mutable reference**: encode/decode signatures take `buffer_view&` (not const) since the view's position may need to advance during encoding/decoding. This differs from the read-only `buffer_view` usage in traits.hpp mock types.
- **CRTP pattern**: `encoder_base` and `decoder_base` provide stub implementations for `encode_fields`, `encode_tagged`, `decode_tagged` that will be filled in with BER codec. Derived classes inherit without virtual dispatch.
- **`stateless_codec`**: Uses `std::is_empty_v<C>` — empty codec types have no instance state, enabling zero-cost abstractions.
- **Test coverage**: 10 runtime tests + 13 static_assert concept tests covering positive/negative encoder_for, decoder_for, codec_for, stateless_codec, and all CRTP base classes.

## Task 9: Codec Interface Traits (2026-05-10)
- **Files already implemented**: `libs/codec/codec_interface.hpp` and `test/codec/codec_interface_test.cpp` were pre-existing and fully implemented
- **Concepts defined**: `encoder_for<E,T>`, `decoder_for<D,T>`, `codec_for<C,T>`, `stateless_codec<C>`
- **CRTP bases**: `encoder_base<Derived>` and `decoder_base<Derived>` with stub `encode_fields`/`encode_tagged`/`decode_tagged`
- **decoder_for concept**: Uses `d.decode(buf) -> result<T>` (non-template decode), so template `decode<T>(buf)` does NOT match — this is by design
- **10 tests all pass**: 3 CRTP runtime tests + 7 concept validation tests
- **CMakeLists already wired**: `asn1pp_add_test(codec_interface_test ...)` was already in test/CMakeLists.txt
- **Fixed**: Removed unused `#include <concepts>` from test file (was flagged by clangd)
- **clangd false positive**: `<type_traits>` flagged as unused despite `std::is_empty_v` usage in static_assert — clangd's unused-include heuristic doesn't trace static_assert usages

## Task 10: Source Location + Diagnostic Framework (2026-05-10)
- **source_location reuse**: Existing struct in `src/gen/ast.hpp` (lines 16-20) with `string_view file`, `size_t line`, `size_t column`. No duplicate struct needed — `diagnostics.hpp` includes `ast.hpp`.
- **Header-only design**: All `diagnostic_engine` methods inline in the header (no .cpp). Simpler build integration, works with `asn1pp_add_test` macro without extra source files.
- **GCC-style format**: `file:line:col: severity: message` — standard format recognizable by IDEs for clickable error navigation.
- **No exceptions**: Engine accumulates diagnostics in a `std::vector<diagnostic>`. Caller checks `has_errors()` after parsing/validation.
- **No iostream**: Uses `std::string` concatenation with `reserve()` for formatting. No `<format>` header needed.
- **Iteration**: Raw pointer iteration (`begin()`/`end()`) returning `const diagnostic*` from `vector::data()` — simple, no iterator wrapper needed.
- **Count methods**: `error_count()`, `warning_count()`, `note_count()` each iterate the vector (O(n)). For small diagnostic counts this is fine; if scaling, consider maintaining separate count fields.
- **Caret positioning**: `format_line_with_caret()` adds spaces (column-1) then `^`. Column 0 means no caret. Simple tab handling — each tab counts as 1 space position.

## Task 11: ASN.1 Lexer (2026-05-10)
- **Hand-written lexer** using string_view for zero-copy scanning, no regex, no generator tools
- **Case-insensitive keywords**: `scan_identifier_or_keyword()` builds uppercase version for `unordered_map` lookup, preserving original case in token value
- **ASN.1 identifier rules**: Allows hyphens in identifiers (e.g., `obj-id`, `my-module`), matching X.680 spec
- **Binary/hex string parsing**: `'0101'B` / `'A0FF'H` — scans between single quotes, then checks suffix character (case-insensitive B/H)
- **Comment handling**: `--` line comments (until newline), `/* */` block comments (multiline, with nesting stars support)
- **Peek mechanism**: `std::optional<token> peeked_` stores one-token lookahead; `peek_token()` caches, `next_token()` consumes
- **Diagnostic integration**: `has_error_` flag + optional `diagnostic_engine*` for error reporting (null-safe)
- **149 test cases** covering: 28 keywords, 14 case-insensitivity, 8 identifiers, 6 numbers, 5 strings, 5 binary, 4 hex, 14 operators, 7 comments, 3 comment interaction, 7 errors, 5 peek, 6 locations, 5 EOF, 8 integration
- **`double_dot` token_type** exists in enum but unused (lexer uses `range` for `..`) — kept for spec compliance
- Test registered as: `asn1pp_add_test(lexer_test gen/lexer_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/lexer.cpp)`

## Task 12: ASN.1 Parser (2026-05-10)
- Parser implementation lives in `src/gen/parser.hpp` + `src/gen/parser.cpp` and is already wired into `asn1pp-gen` plus `parser_test` via existing CMake entries.
- Parser uses lexer lookahead directly, `result<T>` for parse failures, and reports diagnostics through the optional `diagnostic_engine*`; hard module-structure failures return parse_error while module-body assignment/import/export issues synchronize and continue.
- `TAGS`, `STRING`, `IMPLIED`, `OBJECT IDENTIFIER`, `RELATIVE OID`, and `ANY` are handled as case-insensitive identifiers because the lexer keyword table does not classify those multi-word/special ASN.1 terms.
- Constraints supported for this wave: value ranges, `SIZE (...)`, extension marker, and a basic `ALL EXCEPT ...` skip-to-close representation as `extension_constraint`.
- Parser test suite currently has 99 Parser tests covering module structure, tag defaults, assignments, core builtin types, SEQUENCE/SET/CHOICE, OF types, imports/exports, constraints, tagged types, values, recovery, and real-world patterns.
- AppleClang/GTest emits `-Wcharacter-conversion` from system-installed gtest headers during parser test compilation unless test targets add `-Wno-character-conversion`; `asn1pp_add_test` now applies it with `-Wno-sign-compare`.
- Verification run: `cmake --build build` completed, but Apple `ranlib` warns that placeholder codec objects have no symbols; no compiler warnings remained. `ctest --test-dir build -R Parser` and direct `./build/test/parser_test` passed all 99 tests.
