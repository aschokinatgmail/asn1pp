# asn1pp — Full Architecture Plan

## TL;DR

> **Quick Summary**: Build a two-component ASN.1 system: (1) a hand-written recursive-descent code generator (`asn1pp-gen`) that reads .asn files and emits C++20 types, and (2) a runtime codec library (`libasn1pp-codec`) supporting all ITU-T encoding rules — BER/DER, PER (aligned/unaligned), OER/COER, XER/CXER/E-XER, and JER. Wave-1 encoding rule is DER (testable against X.509). Full X.68x (information object classes, parameterized types) is deferred to a late wave after the core pipeline is proven.
>
> **Deliverables**:
> - `libasn1pp-codec` — Header-heavy C++20 runtime library: `buffer_view`, type traits, encoding/decoding dispatch, all codec rule implementations
> - `asn1pp-gen` — Standalone CLI tool: ASN.1 lexer → recursive-descent parser → semantic analyzer → C++20 code emitter
> - `asn1pp-cmake` — CMake module: `asn1pp_generate()` function for build-integrated code generation
> - `libasn1pp-codec-test` — GTest-based TDD test suite
> - `asn1pp-gen-test` — Parser/conformance test suite with curated .asn modules
>
> **Estimated Effort**: XL (40+ implementation tasks, 4 review tasks)
> **Parallel Execution**: YES — 9 waves, averaging 5+ tasks per wave
> **Critical Path**: Task 3 → 4 → 9 → 19 → 28 → 40 → F1-F4

---

## Context

### Original Request
Build a translator of an abstract model written in ASN.1 to a C++ framework using modern C++ standards, with codecs covering all coding standards for the selected model.

### Interview Summary

**Key Decisions**:
- **Code Generator**: Reads .asn files, emits C++20 `.hpp`/`.cpp`. NOT a header-only type-definition library.
- **C++ Standard**: C++20 — concepts, `std::span`, constexpr, coroutines, `std::expected`.
- **Encoding Rules**: Full suite (BER, DER, CER, PER-aligned, PER-unaligned/UPER, CPER, OER, COER, XER, CXER, E-XER, JER).
- **Use Case**: General purpose (not telecom-specific or PKI-only).
- **ASN.1 Coverage**: Full X.68x suite including information object classes, table constraints, parameterized types — **deferred to late wave**.
- **Build**: Three separate CMake targets — `libasn1pp-codec`, `asn1pp-gen` (CLI), `asn1pp-cmake` (CMake module).
- **Parser**: Hand-written recursive descent (no external parser generator dependency).
- **Buffer**: Custom `buffer_view` class supporting scatter-gather and zero-copy patterns.
- **Constraints**: Both compile-time (concepts) and runtime validation.
- **Tests**: TDD with Google Test + GMock.

**Research Findings**:
- No open-source C++ library handles more than 2-3 encoding rules. Full suite is architecturally novel.
- PER requires schema knowledge for encoding — the code generator MUST emit constraint metadata tables, not just C++ types. PER cannot self-describe like BER.
- asn1c (Google's C implementation) has incomplete/buggy X.681 information object class support — the most mature tool in the space. This confirms IOC must be deferred.
- DER is the obvious wave-1 encoding rule: simpler than PER, testable against X.509 certificates and PKCS standards.

### Metis Review

**Identified Gaps** (addressed):
- **"Lightweight" vs "full X.68x + 12 encodings" tension**: Resolved — "lightweight" is aspirational. The plan delivers core architecture first, then adds weight incrementally. Early waves produce a working DER codec with core types, which IS lightweight.
- **No wave-1 encoding rule specified**: Resolved — DER is wave-1. Testable against X.509.
- **No conformance test schema**: Resolved — plan includes curating X.509 (DER), 3GPP RRC fragments (PER), and hand-crafted edge-case modules.
- **Information object class completeness risk**: Resolved — IOC deferred to Wave 10 after core pipeline proven.
- **Coroutine streaming codecs**: Added as optional enhancement, not blocking any wave.

---

## Work Objectives

### Core Objective
Create a working ASN.1-to-C++20 toolchain: parse ASN.1 schemas, generate C++20 types with full codec support, starting with DER and expanding to all encoding rules.

### Concrete Deliverables
- `libs/buffer/buffer_view.hpp` + `.cpp` — Custom buffer abstraction
- `libs/codec/traits.hpp` — Type traits and concepts for ASN.1 codecs
- `libs/codec/ber/encoder.hpp` + `decoder.hpp` — BER/DER codec (wave 1)
- `libs/codec/per/encoder.hpp` + `decoder.hpp` — PER codec (wave 2)
- `libs/codec/oer/encoder.hpp` + `decoder.hpp` — OER/COER codec
- `libs/codec/xer/encoder.hpp` + `decoder.hpp` — XER/CXER/E-XER codec
- `libs/codec/jer/encoder.hpp` + `decoder.hpp` — JER codec
- `src/gen/lexer.cpp` + `parser.cpp` — ASN.1 parser
- `src/gen/emitter.cpp` — C++20 code emitter
- `src/gen/main.cpp` — CLI entry point
- `cmake/asn1pp.cmake` — CMake integration module

### Definition of Done
- [ ] `asn1pp-gen` parses X.509 ASN.1 module and generates compilable C++20
- [ ] Generated C++ types round-trip through DER (encode → decode → compare)
- [ ] All 12 encoding rules have passing TDD suites
- [ ] `asn1pp_generate()` CMake function works in a test project
- [ ] Zero compiler warnings at `-Wall -Wextra -Wpedantic`

### Must Have
- DER codec with round-trip fidelity
- Code generator producing valid C++20 that compiles with GCC 10+, Clang 10+, MSVC 2019 16.10+
- `buffer_view` supporting bit-level and octet-level access
- TDD test suites for all deliverables

### Must NOT Have (Guardrails)
- No external parser generator dependency (ANTLR, Bison, etc.)
- No RTTI or exceptions in the runtime codec library (compile to bare-metal)
- No macro-based code generation (`#define` magic)
- No heap allocations in hot paths (pre-allocated buffers where possible)
- No hidden coupling between codec implementations (each encoding rule is independent)
- No premature optimization before TDD suite passes
- Do NOT build all 12 codecs simultaneously — sequential encoding rules, one per wave
- Do NOT attempt full X.68x (IOC, parameterized types) before core pipeline proven

---

## Verification Strategy (MANDATORY)

> **ZERO HUMAN INTERVENTION** — ALL verification is agent-executed. No exceptions.

### Test Decision
- **Infrastructure exists**: NO (greenfield)
- **Automated tests**: TDD (RED → GREEN → REFACTOR)
- **Framework**: Google Test + Google Mock
- **Setup**: Included as Wave 1, Task 2

### QA Policy
Every task MUST include agent-executed QA scenarios. Evidence saved to `.sisyphus/evidence/task-{N}-{scenario-slug}.{ext}`.

- **Codec**: Use Bash to compile test program with `g++ -std=c++20`, run, assert output. Compare with known-good values.
- **Parser**: Feed curated .asn files, assert parse tree matches expected structure.
- **CLI**: Use Bash to invoke `asn1pp-gen`, assert exit code and output file existence.
- **Build**: `cmake --build build` must succeed with zero warnings.

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Start Immediately — foundation, MAX PARALLEL):
├── Task 1: Project scaffold + 3 CMake targets [quick]
├── Task 2: Google Test + GMock setup [quick]
├── Task 3: buffer_view class (TDD) [deep]
├── Task 4: Core type traits + ASN.1 concepts [deep]
├── Task 5: Error types (result<T>, error_code) [quick]
├── Task 6: Bit/octet manipulation utilities [quick]
└── Task 7: ASN.1 universal tag registry [quick]

Wave 2 (After Wave 1 — interfaces + compiler frontend):
├── Task 8: ASN.1 AST data structures [quick]
├── Task 9: Codec interface traits (encoder/decoder concepts) [deep]
├── Task 10: Source location + diagnostic framework [quick]
├── Task 11: ASN.1 lexer (TDD) [deep]
├── Task 12: ASN.1 parser — core grammar (depends: 8, 11) [deep]
└── Task 13: C++ type-to-tag mapping traits (depends: 4, 7) [quick]

Wave 3 (After Wave 2 — core type generators, MAX PARALLEL):
├── Task 14: INTEGER type generator (depends: 8, 13) [quick]
├── Task 15: OCTET STRING / BIT STRING generator (depends: 8, 13) [quick]
├── Task 16: BOOLEAN + NULL generator (depends: 8, 13) [quick]
├── Task 17: ENUMERATED generator (depends: 8, 13) [quick]
└── Task 18: SEQUENCE generator (depends: 8, 13) [deep]

Wave 4 (After Wave 3 — DER codec, MAX PARALLEL):
├── Task 19: Tag/Length encoder + decoder (depends: 3, 6, 7) [quick]
├── Task 20: BER encoder (depends: 3, 4, 6, 9, 19) [deep]
├── Task 21: BER decoder (depends: 3, 4, 6, 9, 19) [deep]
└── Task 22: DER canonical validation layer (depends: 20) [quick]

Wave 5 (After Wave 4 — more generators + PER prep):
├── Task 23: CHOICE generator (depends: 8, 13) [deep]
├── Task 24: SEQUENCE OF / SET OF generator (depends: 8, 13) [quick]
├── Task 25: OBJECT IDENTIFIER + RELATIVE-OID generator (depends: 8, 13) [quick]
├── Task 26: Time types generator (UTCTime, GeneralizedTime) (depends: 8, 13) [quick]
├── Task 27: PER constraint metadata emitter (depends: 8, 13) [deep]
└── Task 28: DER integration tests with X.509 schema (depends: 14-18, 22) [deep]

Wave 6 (After Wave 5 — PER codec, MAX PARALLEL):
├── Task 29: PER aligned encoder (depends: 3, 4, 6, 9, 27) [deep]
├── Task 30: PER aligned decoder (depends: 3, 4, 6, 9, 27) [deep]
├── Task 31: PER unaligned (UPER) encoder (depends: 29) [deep]
└── Task 32: PER unaligned (UPER) decoder (depends: 30) [deep]

Wave 7 (After Wave 6 — text codecs, MAX PARALLEL):
├── Task 33: OER encoder + decoder (depends: 3, 4, 9) [deep]
├── Task 34: COER canonical layer (depends: 33) [quick]
├── Task 35: XER encoder + decoder (depends: 3, 4, 9) [deep]
├── Task 36: CXER + E-XER layers (depends: 35) [quick]
└── Task 37: JER encoder + decoder (depends: 3, 4, 9) [deep]

Wave 8 (After Wave 7 — CLI + integration):
├── Task 38: CLI tool (asn1pp-gen main, args, file I/O) (depends: 12, 14-18, 23-27) [deep]
├── Task 39: CMake integration module (asn1pp_generate) (depends: 38) [quick]
├── Task 40: End-to-end test: X.509 DER round-trip (depends: 28, 38) [deep]
├── Task 41: End-to-end test: 3GPP RRC PER round-trip (depends: 29-32, 38) [deep]
└── Task 42: Documentation (README, usage guide, API docs) [writing]

Wave 9 (After Wave 8 — advanced ASN.1 constructs, DEFERRED):
├── Task 43: Tagged types (IMPLICIT/EXPLICIT) generator + codec [deep]
├── Task 44: Constrained types (value range, size, alphabet) [deep]
├── Task 45: Information object class support (X.681) [deep]
└── Task 46: Parameterized types + table constraints (X.682-X.683) [deep]

Wave FINAL (After ALL tasks — 4 parallel reviews, then user okay):
├── Task F1: Plan compliance audit (oracle)
├── Task F2: Code quality review (unspecified-high)
├── Task F3: Real manual QA (unspecified-high)
└── Task F4: Scope fidelity check (deep)
-> Present results -> Get explicit user okay

Critical Path: Task 3 → 4 → 9 → 19 → 20 → 28 → 40 → F1-F4
Parallel Speedup: ~65% faster than sequential
Max Concurrent: 7 (Wave 1)
```

---

## TODOs

- [x] 1. **Project scaffold + 3 CMake targets**

  **What to do**:
  - Create top-level `CMakeLists.txt` with C++20 requirement (`set(CMAKE_CXX_STANDARD 20)`)
  - Create `libs/CMakeLists.txt` for `libasn1pp-codec` (static library target)
  - Create `src/CMakeLists.txt` for `asn1pp-gen` (executable target)
  - Create `cmake/asn1pp.cmake` skeleton for `asn1pp-cmake`
  - Create directory structure: `libs/buffer/`, `libs/codec/`, `libs/codec/ber/`, `libs/codec/per/`, `libs/codec/oer/`, `libs/codec/xer/`, `libs/codec/jer/`, `src/gen/`, `test/`
  - Add `compile_commands.json` generation (`set(CMAKE_EXPORT_COMPILE_COMMANDS ON)`)
  - Set compiler flags: `-Wall -Wextra -Wpedantic -Werror` for GCC/Clang, `/W4 /WX` for MSVC
  - Add `test/` subdirectory in CMake (placeholder until Task 2)

  **Must NOT do**:
  - No vcpkg/Conan dependency managers — fetch dependencies via FetchContent or system packages
  - No `add_subdirectory` for external deps yet — defer to Task 2

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Boilerplate CMake scaffolding with standard patterns
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All skills: No domain overlap with project-scoped skills

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 2-7)
  - **Blocks**: Tasks 2, 3, 4, 5, 6, 7, 8 (all depend on build system)
  - **Blocked By**: None (can start immediately)

  **References**:
  - `.gitignore` — Existing build directory exclusions confirm CMake conventions
  - Standard CMake docs: Modern CMake project layout with `project()`, `add_library()`, `add_executable()`

  **Acceptance Criteria**:
  - [ ] `cmake -B build -DCMAKE_CXX_STANDARD=20` succeeds
  - [ ] `cmake --build build` succeeds (empty targets compile)
  - [ ] `build/compile_commands.json` exists
  - [ ] Directory tree matches: `libs/{buffer,codec/{ber,per,oer,xer,jer}}`, `src/gen/`, `test/`, `cmake/`

  **QA Scenarios**:

  ```
  Scenario: Build system initialization
    Tool: Bash
    Preconditions: Clean checkout, CMake ≥ 3.20 installed
    Steps:
      1. cmake -B build -DCMAKE_CXX_STANDARD=20 -DCMAKE_BUILD_TYPE=Debug
      2. Assert: exit code 0, "Build files have been written" in output
      3. cmake --build build
      4. Assert: exit code 0, no error messages
      5. ls build/
      6. Assert: CMakeCache.txt, Makefile (or .ninja), compile_commands.json exist
    Expected Result: Build system generates without errors
    Failure Indicators: CMake error, missing C++20 compiler, non-zero exit code
    Evidence: .sisyphus/evidence/task-1-scaffold.txt

  Scenario: Compiler flags verify
    Tool: Bash
    Preconditions: Task 1 build succeeds
    Steps:
      1. cmake -B build -DCMAKE_CXX_STANDARD=20 -DCMAKE_BUILD_TYPE=Debug
      2. grep -r "CMAKE_CXX_FLAGS" CMakeLists.txt libs/CMakeLists.txt src/CMakeLists.txt
      3. Assert: -Wall -Wextra -Wpedantic flags present (or MSVC equivalents)
    Expected Result: Strict warning flags configured
    Failure Indicators: Missing warning flags
    Evidence: .sisyphus/evidence/task-1-flags.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named: task-1-{scaffold,flags}.txt

  **Commit**: YES (Wave 1 group)
  - Message: `feat: project scaffold, build system, test infra, core primitives`

- [x] 2. **Google Test + GMock setup**

  **What to do**:
  - Fetch Google Test + GMock via CMake `FetchContent` (tag: v1.14.0)
  - Create `test/CMakeLists.txt` with `gtest_discover_tests()` integration
  - Create `test/main.cpp` with standard GTest `RUN_ALL_TESTS()` boilerplate
  - Create a one-test smoke test: `test/smoke_test.cpp` — `EXPECT_EQ(1, 1)`
  - Verify `ctest --test-dir build` discovers and runs the smoke test
  - Add a helper: `asn1pp_add_test(name sources...)` CMake macro wrapping `add_executable` + `target_link_libraries(... GTest::gtest_main ...)` + `gtest_discover_tests`

  **Must NOT do**:
  - No system-installed GTest (`find_package(GTest)`) — use FetchContent for reproducibility
  - No GTest as git submodule

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Standard GTest setup with FetchContent — well-trodden pattern
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - `unit-test-generation`: C++ project uses GTest, not the project-scoped harness

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1, 3-7)
  - **Blocks**: All test-writing tasks in later waves
  - **Blocked By**: Task 1 (build system must exist)

  **References**:
  - Google Test docs: `FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.14.0)`
  - Standard pattern: `include(GoogleTest)` + `gtest_discover_tests()`

  **Acceptance Criteria**:
  - [ ] `cmake --build build` compiles `test/smoke_test.cpp`
  - [ ] `ctest --test-dir build` discovers 1 test
  - [ ] `ctest --test-dir build --output-on-failure` shows `[  PASSED  ] 1 test.`
  - [ ] `asn1pp_add_test` CMake macro works (verify via smoke test)

  **QA Scenarios**:

  ```
  Scenario: GTest smoke test passes
    Tool: Bash
    Preconditions: Task 1 complete
    Steps:
      1. cmake -B build -DCMAKE_BUILD_TYPE=Debug
      2. cmake --build build
      3. ctest --test-dir build --output-on-failure
      4. Assert: output contains "1 test from SmokeTest", "[  PASSED  ] 1 test."
    Expected Result: GTest integration works, smoke test passes
    Failure Indicators: Link errors, GTest not found, test not discovered
    Evidence: .sisyphus/evidence/task-2-smoke.txt

  Scenario: GTest fetch from clean
    Tool: Bash
    Preconditions: Delete build/ and _deps/ directories
    Steps:
      1. cmake -B build -DCMAKE_BUILD_TYPE=Debug 2>&1
      2. Assert: output mentions "Downloading googletest" or "Performing download"
      3. cmake --build build
      4. ctest --test-dir build --output-on-failure
      5. Assert: tests pass
    Expected Result: GTest fetched and built from clean state
    Failure Indicators: Network error, fetch failure
    Evidence: .sisyphus/evidence/task-2-clean-fetch.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named: task-2-{smoke,clean-fetch}.txt

  **Commit**: YES (Wave 1 group)

- [x] 3. **`buffer_view` class (TDD)**

  **What to do**:
  - RED: Write failing tests for `buffer_view` — construction from `uint8_t*`, construction from `std::vector<uint8_t>`, `size()`, `empty()`, `data()`, `operator[]`, `subspan()`, `first()`, `last()`, bounds checking on out-of-range access
  - GREEN: Implement `libs/buffer/buffer_view.hpp` — a non-owning view over a contiguous byte range. Supports:
    - Implicit construction from `std::span<const uint8_t>` and `std::span<uint8_t>`
    - Construction from raw pointer + size
    - `subview(offset, length)` returning new `buffer_view` — no copy
    - `slice_at(offset)` — split into `[0..offset)` and `[offset..end)` — two views
    - `operator[]` with `assert()` bounds check in debug
  - Add bit-level access: `bit_at(bit_offset)` (reads single bit), `bit_span(bit_offset, bit_count)` returns packed view
  - REFACTOR: Ensure zero-copy throughout, const-correctness, `noexcept` where possible
  - Add `libs/buffer/buffer_view.cpp` for non-trivial methods (if any)

  **Must NOT do**:
  - No owning semantics — `buffer_view` never allocates
  - No virtual methods — this is a value type
  - No exceptions in release builds — use `assert()` for bounds
  - No implicit conversion to `std::span` — explicit `as_span()` method only

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Core data structure with zero-copy constraints, bit-level operations, TDD workflow
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - `mock-stub-creation`: No mocking needed for buffer_view testing

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1, 2, 4-7)
  - **Blocks**: Tasks 6, 9, 19, 20, 21, 29-37 (all codec tasks)
  - **Blocked By**: Tasks 1, 2 (build + test infra)

  **References**:
  - `std::span<T>` — `buffer_view` is conceptually a `std::span<uint8_t>` with ASN.1-specific extensions
  - C++20 `std::span` docs: `https://en.cppreference.com/w/cpp/container/span` — API shape to mirror

  **Acceptance Criteria**:
  - [ ] Test file: `test/buffer/buffer_view_test.cpp`
  - [ ] `ctest --test-dir build` shows `buffer_view` tests pass
  - [ ] `buffer_view` is trivially copyable (`static_assert(std::is_trivially_copyable_v<buffer_view>)`)
  - [ ] `sizeof(buffer_view)` == `2 * sizeof(void*)` (pointer + size, no vtable)

  **QA Scenarios**:

  ```
  Scenario: Basic construction and access
    Tool: Bash (compile + run GTest)
    Preconditions: Test file written, buffer_view.hpp exists
    Steps:
      1. g++ -std=c++20 -Ilibs test/buffer/buffer_view_test.cpp -lgtest -lgtest_main -o /tmp/test_bv
      2. /tmp/test_bv --gtest_filter="BufferViewTest.Construction*:BufferViewTest.Access*"
      3. Assert: "[  PASSED  ]" for all construction and access tests
    Expected Result: buffer_view constructs from pointer+size, std::span, std::vector; operator[] and size() work
    Failure Indicators: Compilation error, assertion failure, wrong size, wrong data
    Evidence: .sisyphus/evidence/task-3-basic.txt

  Scenario: Zero-copy verification
    Tool: Bash (compile + run)
    Preconditions: buffer_view_test includes zero-copy assertions
    Steps:
      1. g++ -std=c++20 -Ilibs test/buffer/buffer_view_test.cpp -lgtest -lgtest_main -o /tmp/test_bv
      2. /tmp/test_bv --gtest_filter="BufferViewTest.ZeroCopy*"
      3. Assert: subview()/slice_at() return views pointing to same underlying memory
      4. Assert: no allocation calls intercepted (if using custom allocator tracking)
    Expected Result: Sub-views share memory with parent, no allocation
    Failure Indicators: Data copied, allocator called, pointer mismatch
    Evidence: .sisyphus/evidence/task-3-zerocopy.txt

  Scenario: Bit-level access
    Tool: Bash
    Preconditions: Bit access methods implemented
    Steps:
      1. g++ -std=c++20 -Ilibs test/buffer/buffer_view_test.cpp -lgtest -lgtest_main -o /tmp/test_bv
      2. /tmp/test_bv --gtest_filter="BufferViewTest.BitAccess*"
      3. Assert: bit_at(0) on {0b10101010} returns 1; bit_at(1) returns 0
      4. Assert: bit_at(7) on {0b10000000} returns 1; bit_at(0) returns 0
    Expected Result: Bit-level reads correct, MSB-first order
    Failure Indicators: Wrong bit values, wrong endianness
    Evidence: .sisyphus/evidence/task-3-bit.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named: task-3-{basic,zerocopy,bit}.txt

  **Commit**: YES (Wave 1 group)

- [x] 4. **Core type traits + ASN.1 concepts**

  **What to do**:
  - RED: Write failing tests — verify that `has_tag_v<MyType>` is true for tagged types, `codec_encodable_v<MyType, ber_encoder>` for types with BER support, constraint concepts reject invalid types
  - GREEN: Implement `libs/codec/traits.hpp`:
    - `template<typename T> struct asn1_tag { static constexpr tag_t value = ...; }` — primary template, specializable
    - `has_tag_v<T>` — detect if `asn1_tag<T>` is defined
    - `template<typename T, typename Encoder> concept encodable = requires(T t, Encoder e, buffer_view b) { { e.encode(t, b) } -> std::same_as<result<void>>; };`
    - `template<typename T, typename Decoder> concept decodable = requires(Decoder d, buffer_view b) { { d.template decode<T>(b) } -> std::same_as<result<T>>; };`
    - `is_sequence_v<T>`, `is_choice_v<T>`, `is_set_v<T>` — detect generated types
    - `universal_tag` enum class: `end_of_content = 0`, `boolean = 1`, `integer = 2`, `bit_string = 3`, `octet_string = 4`, `null = 5`, `oid = 6`, ..., `sequence = 16`, `set = 17`, ...
  - REFACTOR: Ensure concepts produce readable error messages

  **Must NOT do**:
  - No runtime type information (RTTI) — all traits are compile-time
  - No macros for trait specialization — use template specialization or inline constexpr variables
  - No implicit conversions between tag types

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Template metaprogramming + C++20 concepts — requires careful SFINAE/concept design
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No domain overlap with available skills

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1-3, 5-7)
  - **Blocks**: Tasks 9, 13, 14-18, 20, 21, 29-37 (all codec and generator tasks)
  - **Blocked By**: Tasks 1, 2 (build + test infra)

  **References**:
  - C++20 concepts: `https://en.cppreference.com/w/cpp/language/constraints`
  - ITU-T X.680 universal tags: Table of universal class tag assignments
  - `std::same_as`, `std::convertible_to` — use standard concepts where applicable

  **Acceptance Criteria**:
  - [ ] Test file: `test/codec/traits_test.cpp`
  - [ ] `ctest --test-dir build` shows traits tests pass
  - [ ] `static_assert(encodable<integer_type, ber_encoder>)` compiles
  - [ ] `static_assert(!encodable<int, ber_encoder>)` compiles (int is not an ASN.1 type)
  - [ ] All universal tags defined and match X.680 Table 1

  **QA Scenarios**:

  ```
  Scenario: Concept rejects non-ASN.1 types
    Tool: Bash
    Preconditions: traits.hpp written, test file has static_assert tests
    Steps:
      1. g++ -std=c++20 -Ilibs test/codec/traits_test.cpp -lgtest -lgtest_main -o /tmp/test_traits
      2. /tmp/test_traits --gtest_filter="TraitsTest.ConceptsRejectNonAsn1*"
      3. Assert: compilation succeeds (negative concepts compile to false, not error)
    Expected Result: Plain int, std::string rejected by encodable/decodable concepts
    Failure Indicators: Compilation error, concept matches wrongly
    Evidence: .sisyphus/evidence/task-4-concepts.txt

  Scenario: Tag detection works
    Tool: Bash
    Preconditions: asn1_tag<int_type> specialized for test type
    Steps:
      1. /tmp/test_traits --gtest_filter="TraitsTest.TagDetection*"
      2. Assert: has_tag_v<test_integer_type> == true
      3. Assert: asn1_tag<test_integer_type>::value == universal_tag::integer
    Expected Result: Tag correctly detected and value correct
    Failure Indicators: Wrong tag value, has_tag_v false when specialized
    Evidence: .sisyphus/evidence/task-4-tags.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named: task-4-{concepts,tags}.txt

  **Commit**: YES (Wave 1 group)

- [x] 5. **Error types (`result<T>`, `error_code`)**

  **What to do**:
  - RED: Write tests for `result<int>` — success construction, error construction, `is_ok()`, `is_err()`, `value()`, `error()`, monadic `map()` and `and_then()`
  - GREEN: Implement `libs/codec/result.hpp`:
    - `enum class error_code { ok = 0, buffer_overflow, buffer_underflow, invalid_tag, invalid_length, constraint_violation, parse_error, ... };`
    - `template<typename T> class result` — simple `expected<T, error_code>` with:
      - `static result<T> ok(T val)` and `static result<T> err(error_code e)`
      - `bool is_ok()`, `bool is_err()`
      - `T& value()`, `const T& value() const` — asserts ok in debug
      - `error_code error() const`
      - `template<typename F> auto map(F&& f) -> result<...>`
      - `template<typename F> auto and_then(F&& f) -> result<...>`
    - `template<> class result<void>` — specialization for no-value success
  - REFACTOR: Ensure trivially copyable when T is trivially copyable

  **Must NOT do**:
  - No exceptions — result is the error handling mechanism
  - No `std::expected` dependency if compiler support is spotty (implement our own)
  - No `std::variant` for storage — use discriminated union with enum

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Well-defined vocabulary type with clear semantics, standard pattern
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1-4, 6, 7)
  - **Blocks**: Tasks 9, 20, 21, 29-37 (all encoder/decoder return result<T>)
  - **Blocked By**: Tasks 1, 2 (build + test infra)

  **References**:
  - `std::expected<T,E>` — API inspiration: `https://en.cppreference.com/w/cpp/utility/expected`
  - `error_code` enum values derived from common ASN.1 codec failure modes

  **Acceptance Criteria**:
  - [ ] Test file: `test/codec/result_test.cpp`
  - [ ] `ctest --test-dir build` shows result tests pass
  - [ ] `result<int>::ok(42).map([](int x){ return x*2; }).value()` == 84
  - [ ] `result<int>::err(error_code::buffer_overflow).is_err()` == true
  - [ ] `sizeof(result<int>)` ≤ `2 * sizeof(int)` (tag + value)

  **QA Scenarios**:

  ```
  Scenario: Happy path with map chain
    Tool: Bash
    Steps:
      1. g++ -std=c++20 -Ilibs test/codec/result_test.cpp -lgtest -lgtest_main -o /tmp/test_result
      2. /tmp/test_result --gtest_filter="ResultTest.MapChain*"
      3. Assert: result<int>::ok(5).map(+1).map(*3).value() == 18
    Expected Result: Monadic operations compose correctly
    Failure Indicators: Wrong value, assertion on error path
    Evidence: .sisyphus/evidence/task-5-map.txt

  Scenario: Error short-circuits and_then
    Tool: Bash
    Steps:
      1. /tmp/test_result --gtest_filter="ResultTest.ErrorShortCircuit*"
      2. Assert: err result does not invoke and_then callback
      3. Assert: error code preserved through chain
    Expected Result: Errors propagate without executing subsequent steps
    Failure Indicators: Callback invoked on error, error code changed
    Evidence: .sisyphus/evidence/task-5-error.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named: task-5-{map,error}.txt

  **Commit**: YES (Wave 1 group)

- [x] 6. **Bit/octet manipulation utilities**

  **What to do**:
  - RED: Write tests for `write_bits()`, `read_bits()`, `write_octets()`, `read_octets()`, `align_to_octet()`, `remaining_bits()`, `remaining_octets()`
  - GREEN: Implement `libs/buffer/bit_ops.hpp`:
    - `void write_bits(buffer_view& buf, uint64_t value, size_t bit_count)` — write bit_count LSBs, advance buffer
    - `uint64_t read_bits(buffer_view& buf, size_t bit_count)` — read bit_count bits, advance
    - `void write_octets(buffer_view& buf, const uint8_t* src, size_t count)`
    - `void read_octets(buffer_view& buf, uint8_t* dst, size_t count)`
    - `void align_to_octet(buffer_view& buf)` — advance to next octet boundary
    - `size_t remaining_bits(const buffer_view& buf)`
  - Track internal bit offset within octet for PER support
  - REFACTOR: Use `__builtin_bswap` or `std::byteswap` where beneficial

  **Must NOT do**:
  - No virtual dispatch — all free functions or static methods
  - No dynamic allocation
  - No assumptions about host endianness — output must be network byte order (big-endian per ASN.1)

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Focused bit-twiddling utilities with clear interfaces
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: Utility functions, no project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1-5, 7)
  - **Blocks**: Tasks 19, 20, 21, 29-37 (all codec tasks)
  - **Blocked By**: Tasks 1, 2, 3 (buffer_view needed as parameter type)

  **References**:
  - ITU-T X.691 (PER): Bit-level encoding rules for reference values
  - `buffer_view.hpp` — function signatures use buffer_view for read/write positions

  **Acceptance Criteria**:
  - [ ] Test file: `test/buffer/bit_ops_test.cpp`
  - [ ] `write_bits(buf, 0xABC, 12)` → `read_bits(buf, 12)` == 0xABC (round-trip)
  - [ ] `align_to_octet()` advances bit_offset to 0
  - [ ] `write_octets` + `read_octets` round-trip for 0, 1, 4, 256 byte arrays

  **QA Scenarios**:

  ```
  Scenario: Bit round-trip for various widths
    Tool: Bash
    Steps:
      1. g++ -std=c++20 -Ilibs test/buffer/bit_ops_test.cpp -lgtest -lgtest_main -o /tmp/test_bo
      2. /tmp/test_bo --gtest_filter="BitOps.RoundTrip*"
      3. Assert: write_bits(0x1, 1) → read_bits(1) == 1
      4. Assert: write_bits(0xFF, 8) → read_bits(8) == 0xFF
      5. Assert: write_bits(0xFFFF, 16) → read_bits(16) == 0xFFFF
      6. Assert: write_bits(0x7FFFFFFFFFFFFFFF, 63) → read_bits(63) matches
    Expected Result: All bit widths round-trip correctly, no truncation
    Failure Indicators: Wrong value, MSB/LSB confusion, truncated bits
    Evidence: .sisyphus/evidence/task-6-roundtrip.txt

  Scenario: Mixed bit/octet operations maintain alignment
    Tool: Bash
    Steps:
      1. /tmp/test_bo --gtest_filter="BitOps.Alignment*"
      2. Assert: write_bits(5 bits) + align_to_octet() → subsequent write_octets starts on byte boundary
      3. Assert: write_octets(3 bytes) + write_bits(3 bits) → bit offset is 3
    Expected Result: Bit/octet boundary tracking correct across mixed operations
    Failure Indicators: Byte misalignment, wrong bit offset after align
    Evidence: .sisyphus/evidence/task-6-align.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named: task-6-{roundtrip,align}.txt

  **Commit**: YES (Wave 1 group)

- [x] 7. **ASN.1 universal tag registry**

  **What to do**:
  - RED: Tests for tag lookup — `tag_to_name(universal_tag::integer) == "INTEGER"`, `name_to_tag("BOOLEAN") == universal_tag::boolean`
  - GREEN: Implement `libs/codec/tag.hpp`:
    - `enum class tag_class { universal = 0, application = 1, context_specific = 2, private_class = 3 };`
    - `struct tag { tag_class cls; bool constructed; uint32_t number; };`
    - `constexpr tag make_universal(universal_tag t, bool constructed = false)`
    - `constexpr tag make_context_specific(uint32_t number, bool constructed = false)`
    - `constexpr bool is_primitive(universal_tag t)` — BOOLEAN, INTEGER, NULL, OID, REAL, ENUMERATED are primitive
    - `constexpr bool is_constructed(universal_tag t)` — SEQUENCE, SET, SEQUENCE OF, SET OF, CHOICE-like are constructed
    - String table: `const char* universal_tag_name(universal_tag t)`

  **Must NOT do**:
  - No dynamic allocation for tag registry — everything `constexpr`
  - No duplicate tag numbers

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Simple enum + struct + constexpr functions — straightforward
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1-6)
  - **Blocks**: Tasks 13, 19, 20, 21
  - **Blocked By**: Tasks 1, 2 (build + test infra)

  **References**:
  - ITU-T X.680 Clause 8: Tag encoding rules
  - ITU-T X.680 Table 1: Universal class tag assignments
  - `traits.hpp` — `universal_tag` enum defined in Task 4

  **Acceptance Criteria**:
  - [ ] Test file: `test/codec/tag_test.cpp`
  - [ ] All X.680 universal tags have entries
  - [ ] `is_primitive(universal_tag::boolean)` == true, `is_constructed(universal_tag::sequence)` == true
  - [ ] `tag::operator==` works for comparison

  **QA Scenarios**:

  ```
  Scenario: Complete tag coverage
    Tool: Bash
    Steps:
      1. g++ -std=c++20 -Ilibs test/codec/tag_test.cpp -lgtest -lgtest_main -o /tmp/test_tag
      2. /tmp/test_tag --gtest_filter="TagTest.CompleteCoverage*"
      3. Assert: tags 0-30 all have name entries (no nullptr returns)
      4. Assert: reserved tag 15 not assigned
    Expected Result: All X.680 universal tags defined with names
    Failure Indicators: Missing tag, nullptr name, wrong tag number
    Evidence: .sisyphus/evidence/task-7-coverage.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named: task-7-coverage.txt

  **Commit**: YES (Wave 1 group)

---

- [x] 8. **ASN.1 AST data structures**

  **What to do**:
  - Define AST node types in `src/gen/ast.hpp`:
    - `struct module_definition { string_view name; tag_default default_tagging; extensibility_implied; vector<assignment> assignments; };`
    - `struct type_assignment { string_view name; type_ref type; };`
    - `struct value_assignment { string_view name; type_ref type; value_ref value; };`
    - Type nodes: `integer_type`, `boolean_type`, `null_type`, `enumerated_type`, `bit_string_type`, `octet_string_type`, `oid_type`, `sequence_type`, `set_type`, `choice_type`, `sequence_of_type`, `set_of_type`, `tagged_type`, `constrained_type`, `selection_type`, `any_type`
    - `sequence_type` has `vector<component_type> components` where `component_type = { name; type_ref; bool optional; optional<value_ref> default_value; }`
    - `choice_type` has `vector<choice_alternative>`
    - `enumerated_type` has `vector<enumeration_item>`
    - Constraint, extension marker, extensibility nodes
  - Write `src/gen/ast_print.cpp` for debug AST dumping

  **Must NOT do**:
  - No `std::variant` for AST nodes — use tagged union or inheritance hierarchy
  - No `std::unique_ptr` deep in AST — use `std::variant` for variants, value semantics
  - No source location embedded in AST nodes yet (defer to Task 10)

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Pure data structure definitions, no complex logic
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Tasks 9-13)
  - **Blocks**: Tasks 11, 12, 14-18, 23-27, 38 (parser, generators, CLI)
  - **Blocked By**: Task 1 (project must exist to add files)

  **References**:
  - ITU-T X.680 grammar — full syntactic constructs to model
  - `src/gen/` directory created in Task 1

  **Acceptance Criteria**:
  - [ ] `src/gen/ast.hpp` compiles (tested via simple `#include` test)
  - [ ] AST covers all types from X.680 core: INTEGER through SET OF
  - [ ] `ast_print()` produces readable tree output for debugging

  **QA Scenarios**:

  ```
  Scenario: AST compilation
    Tool: Bash
    Steps:
      1. g++ -std=c++20 -I. -fsyntax-only src/gen/ast.hpp
      2. Assert: exit code 0, no errors
    Expected Result: Header compiles independently
    Failure Indicators: Missing includes, syntax errors, incomplete types
    Evidence: .sisyphus/evidence/task-8-compile.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-8-compile.txt

  **Commit**: YES (Wave 2 group)

- [x] 9. **Codec interface traits (encoder/decoder concepts)**

  **What to do**:
  - RED: Write tests verifying concepts constrain encoder/decoder correctly
  - GREEN: Implement `libs/codec/codec_interface.hpp`:
    - `template<typename E, typename T> concept encoder_for = requires(E e, const T& val, buffer_view buf) { { e.encode(val, buf) } -> std::same_as<result<void>>; };`
    - `template<typename D, typename T> concept decoder_for = requires(D d, buffer_view buf) { { d.template decode<T>(buf) } -> std::same_as<result<T>>; };`
    - `template<typename C, typename T> concept codec_for = encoder_for<C, T> && decoder_for<C, T>;`
    - `template<typename Codec> concept stateless_codec = std::is_empty_v<Codec>;`
    - CRTP base classes: `template<derived D> class encoder_base` and `decoder_base` — provide `encode_sequence()`, `decode_sequence()` helpers using tag dispatch
  - REFACTOR: Ensure concepts compose (a BER encoder concept = encoder_for + has_tag_access)

  **Must NOT do**:
  - No virtual dispatch in codec interface — concepts + CRTP, not vtable
  - No encoder/decoder state in interface — stateless where possible

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: C++20 concepts + CRTP interface design — affects all codec implementations
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Tasks 8, 10-13)
  - **Blocks**: Tasks 20, 21, 29-37 (all codec implementations)
  - **Blocked By**: Tasks 3, 4, 5 (buffer_view, traits, result types)

  **References**:
  - Task 4: `traits.hpp` — `encodable`/`decodable` concepts to extend
  - Task 5: `result.hpp` — return type for encode/decode
  - Task 3: `buffer_view.hpp` — parameter type

  **Acceptance Criteria**:
  - [ ] Test file: `test/codec/codec_interface_test.cpp`
  - [ ] Mock encoder satisfies `encoder_for<mock_encoder, test_type>`
  - [ ] Non-encoder type fails `encoder_for<bad_type, test_type>`
  - [ ] CRTP base compiles for simple encoder

  **QA Scenarios**:

  ```
  Scenario: Concept validation
    Tool: Bash
    Steps:
      1. g++ -std=c++20 -Ilibs test/codec/codec_interface_test.cpp -lgtest -lgtest_main -o /tmp/test_ci
      2. /tmp/test_ci --gtest_filter="CodecInterface.Concepts*"
      3. Assert: valid encoder satisfies concept
      4. Assert: invalid encoder (missing encode method) rejected
    Expected Result: Concepts correctly constrain codec implementations
    Failure Indicators: Concept matches incorrectly, compilation error on valid usage
    Evidence: .sisyphus/evidence/task-9-concepts.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-9-concepts.txt

  **Commit**: YES (Wave 2 group)

- [x] 10. **Source location + diagnostic framework**

  **What to do**:
  - RED: Test source_location comparison, diagnostic formatting, error/warning emission
  - GREEN: Implement `src/gen/diagnostics.hpp`:
    - `struct source_location { string_view file; size_t line; size_t column; };`
    - `enum class severity { error, warning, note };`
    - `struct diagnostic { severity sev; source_location loc; string message; };`
    - `class diagnostic_engine` — collect diagnostics, emit to stderr with GCC-style format: `file:line:col: error: message`
    - `bool has_errors() const`, `size_t error_count() const`, `size_t warning_count() const`
    - Print source line with caret pointing to column
  - REFACTOR: Use `std::format` for diagnostic formatting

  **Must NOT do**:
  - No exceptions for diagnostics — accumulate in engine, check `has_errors()` after parsing
  - No iostream — use `fmt` or `std::format`

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Well-defined diagnostic system, common compiler pattern
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Tasks 8, 9, 11-13)
  - **Blocks**: Tasks 11, 12, 38 (lexer, parser, CLI)
  - **Blocked By**: Task 1

  **References**:
  - GCC diagnostic format: `file:line:col: error: message` — industry standard
  - `std::format`: C++20 — `https://en.cppreference.com/w/cpp/utility/format/format`

  **Acceptance Criteria**:
  - [ ] Test file: `test/gen/diagnostics_test.cpp`
  - [ ] Diagnostic output matches: `test.asn:10:5: error: unexpected token '}'`
  - [ ] `has_errors()` returns true after adding error diagnostic
  - [ ] Source line printing with caret works for multi-line files

  **QA Scenarios**:

  ```
  Scenario: Error formatting
    Tool: Bash
    Steps:
      1. g++ -std=c++20 -Isrc/gen test/gen/diagnostics_test.cpp -lgtest -lgtest_main -o /tmp/test_diag
      2. /tmp/test_diag --gtest_filter="Diagnostics.Format*"
      3. Assert: output matches GCC format
      4. Assert: caret points to correct column
    Expected Result: Diagnostics formatted correctly for IDE/editor integration
    Failure Indicators: Wrong format, wrong column, missing file:line prefix
    Evidence: .sisyphus/evidence/task-10-format.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-10-format.txt

  **Commit**: YES (Wave 2 group)

- [x] 11. **ASN.1 lexer (TDD)**

  **What to do**:
  - RED: Write tests for each token type: keywords (INTEGER, SEQUENCE, CHOICE, etc.), identifiers, number literals, string literals, operators (`::=`, `..`, `...`, `{`, `}`, `(`, `)`, `[`, `]`, `,`, `;`, `|`), comments (both `--` line and `/* */` block)
  - GREEN: Implement `src/gen/lexer.hpp` + `.cpp`:
    - Hand-written lexer (no flex/re2c)
    - `class lexer` holding `string_view` input + position
    - `token next_token()` — advance and return next token
    - `token peek_token()` — look ahead without advancing
    - `source_location current_location()` — for diagnostics
    - Keyword table: all X.680 reserved words mapped to token types
    - Handle: binary strings (`'0101'B`), hex strings (`'A0FF'H`), UTF-8 strings, bstring/hstring/ostring token types
  - REFACTOR: Ensure lexer is reentrant, no global state

  **Must NOT do**:
  - No regex-based tokenization — hand-written only
  - No `std::regex` — too slow for lexer
  - No lexer generator (flex, re2c)

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Hand-written lexer for a full programming language grammar — careful state machine design
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Tasks 8-10, 12, 13)
  - **Blocks**: Tasks 12, 38 (parser needs lexer)
  - **Blocked By**: Tasks 8, 10 (AST types + diagnostics)

  **References**:
  - ITU-T X.680 Annex A: ASN.1 lexical elements — complete token specification
  - `diagnostics.hpp` — `source_location` for token positions

  **Acceptance Criteria**:
  - [ ] Test file: `test/gen/lexer_test.cpp`
  - [ ] All X.680 keywords tokenized correctly (case-insensitive per ASN.1)
  - [ ] Identifier: `myModule` → `token_type::identifier` with value "myModule"
  - [ ] Number: `42` → `token_type::number` with value 42
  - [ ] String: `"-- comment\nINTEGER"` → comment skipped, next token is INTEGER
  - [ ] Error: unclosed string literal produces diagnostic

  **QA Scenarios**:

  ```
  Scenario: Complete keyword coverage
    Tool: Bash
    Steps:
      1. g++ -std=c++20 -Isrc/gen test/gen/lexer_test.cpp -lgtest -lgtest_main -o /tmp/test_lexer
      2. /tmp/test_lexer --gtest_filter="Lexer.Keywords*"
      3. Assert: "INTEGER", "SEQUENCE", "CHOICE", "SET", "OF", "OPTIONAL", "DEFAULT",
                "IMPLICIT", "EXPLICIT", "TAGS", "AUTOMATIC", "FROM", "IMPORTS",
                "EXPORTS", "BEGIN", "END", "DEFINITIONS", "UNIVERSAL", "APPLICATION",
                "PRIVATE", "COMPONENTS", "SIZE", "CONSTRAINED", "WITH", "INCLUDES",
                "MIN", "MAX", "ABSENT", "PRESENT", "PATTERN", "ALL", "EXCEPT"
                all tokenize correctly
    Expected Result: All ASN.1 keywords recognized
    Failure Indicators: Missing keyword, wrong token type, case sensitivity bug
    Evidence: .sisyphus/evidence/task-11-keywords.txt

  Scenario: Complex token stream
    Tool: Bash
    Steps:
      1. Feed: "MyType ::= SEQUENCE { a INTEGER (0..255), b BOOLEAN OPTIONAL }"
      2. /tmp/test_lexer --gtest_filter="Lexer.ComplexStream*"
      3. Assert: token sequence = [identifier"MyType", "::=", "SEQUENCE", "{", "a", "INTEGER", "(", "0", "..", "255", ")", ",", "b", "BOOLEAN", "OPTIONAL", "}"]
      4. Assert: each token has correct source location
    Expected Result: Realistic ASN.1 snippet tokenizes correctly
    Failure Indicators: Wrong token, missing token, wrong location
    Evidence: .sisyphus/evidence/task-11-complex.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named: task-11-{keywords,complex}.txt

  **Commit**: YES (Wave 2 group)

- [x] 12. **ASN.1 parser — core grammar (TDD)**

  **What to do**:
  - RED: Write tests for parser — module header parsing, type assignment, SEQUENCE type, INTEGER type, simple CHOICE, OID value, imports/exports
  - GREEN: Implement `src/gen/parser.hpp` + `.cpp`:
    - Recursive descent parser consuming tokens from lexer
    - `parse_module()` → `module_definition`
    - `parse_type_assignment()` → `type_assignment`
    - `parse_type()` → `type_ref` (dispatches on leading keyword)
    - `parse_sequence_type()`, `parse_choice_type()`, `parse_enumerated_type()`, `parse_integer_type()`, etc.
    - `parse_value()` → `value_ref`
    - `parse_imports()`, `parse_exports()`
    - Report errors via `diagnostic_engine`
    - Synchronization: skip to next `;` or `}` on error for resilience
  - Start with core types only (INTEGER, BOOLEAN, NULL, OCTET STRING, BIT STRING, ENUMERATED, SEQUENCE, CHOICE, SEQUENCE OF, OID)
  - REFACTOR: Ensure error recovery allows reporting multiple errors per parse

  **Must NOT do**:
  - No parser combinator library — raw recursive descent
  - No `exceptions` for parse errors — use result types and diagnostic engine
  - Do NOT parse information object classes, parameterized types, or tagged types yet

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Full recursive descent parser for programming language grammar — state management, error recovery, AST construction
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Tasks 8-11, 13)
  - **Blocks**: Tasks 14-18, 23-27, 38 (generators + CLI)
  - **Blocked By**: Tasks 8, 11 (AST types + lexer)

  **References**:
  - ITU-T X.680: Complete ASN.1 grammar in BNF (adapt to recursive descent)
  - `ast.hpp` — AST node types to construct
  - `lexer.hpp` — token stream interface
  - `diagnostics.hpp` — error reporting

  **Acceptance Criteria**:
  - [ ] Test file: `test/gen/parser_test.cpp`
  - [ ] Parses: `X ::= INTEGER` → AST with type_assignment
  - [ ] Parses: `S ::= SEQUENCE { a INTEGER, b BOOLEAN }` → correct sequence AST with 2 components
  - [ ] Parses: `C ::= CHOICE { a INTEGER, b BOOLEAN }` → correct choice AST
  - [ ] Parses: `E ::= ENUMERATED { red(0), green(1), blue(2) }` → enumerated with values
  - [ ] Error: `X ::= UNKNOWN` → diagnostic emitted, error count > 0
  - [ ] Error recovery: `X ::= INTEGER; Y ::= SEQUENCE { a WRONG, b INTEGER }` → both errors reported

  **QA Scenarios**:

  ```
  Scenario: SEQUENCE with OPTIONAL and DEFAULT
    Tool: Bash
    Steps:
      1. g++ -std=c++20 -Isrc/gen test/gen/parser_test.cpp -lgtest -lgtest_main -o /tmp/test_parser
      2. Feed: "S ::= SEQUENCE { a INTEGER OPTIONAL, b BOOLEAN DEFAULT TRUE }"
      3. /tmp/test_parser --gtest_filter="Parser.SequenceWithModifiers*"
      4. Assert: component 'a' has optional = true
      5. Assert: component 'b' has default_value with boolean true
    Expected Result: OPTIONAL and DEFAULT modifiers parsed correctly
    Failure Indicators: Missing modifier, wrong value, parse error
    Evidence: .sisyphus/evidence/task-12-sequence.txt

  Scenario: Invalid ASN.1 produces diagnostics
    Tool: Bash
    Steps:
      1. Feed: "Bad ::= SEQUENCE { a INTEGER b BOOLEAN }" (missing comma)
      2. /tmp/test_parser --gtest_filter="Parser.ErrorDiagnostics*"
      3. Assert: diagnostic_engine.error_count() > 0
      4. Assert: error message mentions "expected ','" or similar
    Expected Result: Parse errors detected and reported
    Failure Indicators: Silent failure, crash, no diagnostics
    Evidence: .sisyphus/evidence/task-12-error.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named: task-12-{sequence,error}.txt

  **Commit**: YES (Wave 2 group)

- [x] 13. **C++ type-to-tag mapping traits**

  **What to do**:
  - RED: Test that `asn1_tag<int_type>` gives `integer`, that generated SEQUENCE gets `constructed | sequence` tag
  - GREEN: Implement extensions to `traits.hpp`:
    - `tag_for_type_v<T>` — compile-time tag lookup for type T
    - `is_constructed_v<T>` — true for SEQUENCE, SET, CHOICE, SEQUENCE OF, SET OF
    - `is_primitive_v<T>` — true for INTEGER, BOOLEAN, NULL, etc.
    - Default specializations for common C++ types used in code generation: `int64_t → integer`, `std::string → octet_string`, `bool → boolean`
  - REFACTOR: Ensure trait specializations are discoverable by codec implementations

  **Must NOT do**:
  - No runtime tag computation — all constexpr
  - No macros for trait specialization

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Extends existing traits with tag mapping utilities
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Tasks 8-12)
  - **Blocks**: Tasks 14-18, 20, 21 (generators + codecs need tag lookup)
  - **Blocked By**: Tasks 4, 7 (traits + tag registry)

  **References**:
  - Task 4: `traits.hpp` for `asn1_tag<T>` pattern
  - Task 7: `tag.hpp` for tag structure

  **Acceptance Criteria**:
  - [ ] Test file: test/codec/tag_mapping_test.cpp
  - [ ] `tag_for_type_v<int64_type>` == `make_universal(universal_tag::integer)`
  - [ ] `is_constructed_v<sequence_type>` == true
  - [ ] `is_primitive_v<boolean_type>` == true

  **QA Scenarios**:

  ```
  Scenario: Tag mapping correctness
    Tool: Bash
    Steps:
      1. g++ -std=c++20 -Ilibs test/codec/tag_mapping_test.cpp -lgtest -lgtest_main -o /tmp/test_tmap
      2. /tmp/test_tmap --gtest_filter="TagMapping.Correctness*"
      3. Assert: each ASN.1 type maps to correct universal tag
    Expected Result: Tag mapping complete and correct
    Failure Indicators: Wrong tag, missing mapping, ambiguous mapping
    Evidence: .sisyphus/evidence/task-13-correct.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-13-correct.txt

  **Commit**: YES (Wave 2 group)

---

- [x] 14. **INTEGER type generator**

  **What to do**:
  - RED: Test that `generate_integer_type(ast_node)` emits C++ with correct `asn1_tag<>` specialization, `int64_t` storage, constraint enforcement
  - GREEN: Implement `src/gen/emitter_integer.cpp` — `emit_integer_type(ofstream&, const integer_type&, const string& name)`:
    - Emit: `struct name { int64_t value; };`
    - Emit: `template<> struct asn1_tag<name> { static constexpr auto value = universal_tag::integer; };`
    - If constraints present: emit runtime validation method `result<void> validate() const`
    - If named numbers: emit enum within struct
  - REFACTOR: Generate `operator==`, `operator!=` for generated types

  **Must NOT do**:
  - No generation of unbounded integer (ASN.1 unbounded INTEGER is rare — defer)
  - No codec methods in generated type — codec is separate

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Straightforward code emission from parsed AST — template-driven string generation
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 3 (with Tasks 15-18)
  - **Blocks**: Tasks 20, 21, 28, 38 (codecs + integration tests)
  - **Blocked By**: Tasks 8, 12, 13 (AST + parser + tag mapping)

  **References**:
  - Task 4: `traits.hpp` — type traits pattern to emit
  - Task 8: `ast.hpp` — `integer_type` AST node structure
  - Task 12: Parser produces `integer_type` nodes

  **Acceptance Criteria**:
  - [ ] Test: feed `X ::= INTEGER (0..255)` → generates `struct X { int64_t value; }` with validate()
  - [ ] Test: feed `Y ::= INTEGER { red(0), green(1) }` → generates enum
  - [ ] Generated code compiles with C++20

  **QA Scenarios**:

  ```
  Scenario: Simple INTEGER generation
    Tool: Bash
    Steps:
      1. Run code generator test harness with "X ::= INTEGER"
      2. Assert: generated .hpp contains struct X with int64_t value
      3. g++ -std=c++20 -fsyntax-only gen/X.hpp
      4. Assert: compilation succeeds
    Expected Result: Clean C++20 type generated
    Failure Indicators: Missing asn1_tag, wrong storage type, compilation error
    Evidence: .sisyphus/evidence/task-14-simple.txt

  Scenario: Constrained INTEGER
    Tool: Bash
    Steps:
      1. Feed: "X ::= INTEGER (0..255)"
      2. Assert: generated type has validate() returning error on out-of-range
      3. Assert: struct still stores int64_t (not uint8_t — defer narrow type optimization)
    Expected Result: Constraints generate validation logic
    Failure Indicators: Missing validate(), wrong range check, wrong type
    Evidence: .sisyphus/evidence/task-14-constrained.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named: task-14-{simple,constrained}.txt

  **Commit**: YES (Wave 3 group)

- [x] 15. **OCTET STRING / BIT STRING generator**

  **What to do**:
  - RED: Test both OCTET STRING and BIT STRING generation, including SIZE constraints
  - GREEN: Implement `src/gen/emitter_string.cpp`:
    - OCTET STRING → `struct name { std::vector<uint8_t> value; };` + tag = `octet_string`
    - BIT STRING → `struct name { std::vector<uint8_t> data; size_t bit_length; };` + tag = `bit_string`
    - SIZE constraint → validate() method checking `value.size()`
    - Named bits for BIT STRING → enum of bit positions
  - REFACTOR: Consider `std::span`-based views for zero-copy access

  **Must NOT do**:
  - No fixed-size array optimization for SIZE-constrained strings (defer)
  - No custom string class — use std::vector

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Similar pattern to INTEGER generator, string-specific variations
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 3 (with Tasks 14, 16-18)
  - **Blocks**: Tasks 20, 21, 28, 38
  - **Blocked By**: Tasks 8, 12, 13

  **References**:
  - Task 14: INTEGER generator pattern to follow
  - ITU-T X.680: OCTET STRING and BIT STRING syntax

  **Acceptance Criteria**:
  - [ ] OCTET STRING generates `std::vector<uint8_t>` with correct tag
  - [ ] BIT STRING generates vector + bit_length with tag
  - [ ] SIZE constraint produces validate()
  - [ ] Generated code compiles

  **QA Scenarios**:

  ```
  Scenario: OCTET STRING with SIZE
    Tool: Bash
    Steps:
      1. Feed: "X ::= OCTET STRING (SIZE(1..32))"
      2. Assert: validate() rejects empty vector (size 0 < 1)
      3. Assert: validate() accepts size 16
      4. g++ -std=c++20 -fsyntax-only gen/X.hpp → success
    Expected Result: String type with constraint validation
    Evidence: .sisyphus/evidence/task-15-octet.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-15-octet.txt

  **Commit**: YES (Wave 3 group)

- [x] 16. **BOOLEAN + NULL generator**

  **What to do**:
  - RED: Test boolean and null type generation — simplest types
  - GREEN: Implement `src/gen/emitter_boolean.cpp` and `src/gen/emitter_null.cpp`:
    - BOOLEAN → `struct name { bool value; };` + `asn1_tag<name>` = `universal_tag::boolean`
    - NULL → `struct name {};` (empty struct) + `asn1_tag<name>` = `universal_tag::null`
  - Both are trivial — group them together
  - REFACTOR: Add `operator bool()` for boolean type

  **Must NOT do**:
  - No unnecessary complexity — these are the simplest ASN.1 types

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Trivial code generation — single-line structs
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 3 (with Tasks 14, 15, 17, 18)
  - **Blocks**: Tasks 20, 21, 28
  - **Blocked By**: Tasks 8, 12, 13

  **Acceptance Criteria**:
  - [ ] BOOLEAN → `struct X { bool value; }` compiles, tag = boolean
  - [ ] NULL → `struct X {};` compiles, tag = null, sizeof(X) == 1 (or 0 if empty-base-optimized)

  **QA Scenarios**:

  ```
  Scenario: Basic types generate correctly
    Tool: Bash
    Steps:
      1. Feed: "X ::= BOOLEAN" → generate, compile
      2. Feed: "Y ::= NULL" → generate, compile
      3. Assert: both compile, tags correct
    Expected Result: Minimal correct code generation
    Evidence: .sisyphus/evidence/task-16-basic.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-16-basic.txt

  **Commit**: YES (Wave 3 group)

- [x] 17. **ENUMERATED generator**

  **What to do**:
  - RED: Test that `E ::= ENUMERATED { a(0), b(1) }` generates correct C++ with value mapping
  - GREEN: Implement `src/gen/emitter_enumerated.cpp`:
    - Emit: `enum class name : int64_t { a = 0, b = 1 };` + struct wrapper
    - `asn1_tag` specialization
    - Extension marker `...` → emit `unknown_extension = -1` sentinel or handle separately
  - REFACTOR: Generate `to_string()` and `from_string()` for debug purposes

  **Must NOT do**:
  - No automatic value assignment (if ASN.1 omits values, generate sequential starting from 0)
  - No runtime enum dispatch — tag-based codec handles this

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Enum generation is straightforward mapping from ASN.1 enumeration to C++ enum
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 3 (with Tasks 14-16, 18)
  - **Blocks**: Tasks 20, 21, 28
  - **Blocked By**: Tasks 8, 12, 13

  **Acceptance Criteria**:
  - [ ] `ENUMERATED { a(0), b(1), c(2) }` → `enum class : int64_t { a = 0, b = 1, c = 2 }`
  - [ ] `ENUMERATED { a(10), b(20) }` → values preserved
  - [ ] Extension marker handled (generated enum has extension sentinel or comment)

  **QA Scenarios**:

  ```
  Scenario: ENUMERATED with explicit values
    Tool: Bash
    Steps:
      1. Feed: "E ::= ENUMERATED { red(0), green(1), blue(2) }"
      2. Assert: generated enum has red=0, green=1, blue=2
      3. Assert: asn1_tag is universal_tag::enumerated
    Expected Result: Enum values preserved from ASN.1
    Evidence: .sisyphus/evidence/task-17-enum.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-17-enum.txt

  **Commit**: YES (Wave 3 group)

- [x] 18. **SEQUENCE generator**

  **What to do**:
  - RED: Test SEQUENCE generation — multiple fields, OPTIONAL, DEFAULT, nested SEQUENCE
  - GREEN: Implement `src/gen/emitter_sequence.cpp`:
    - Emit `struct name { field1_type field1; std::optional<field2_type> field2; /* if OPTIONAL */ field3_type field3 = default_value; /* if DEFAULT */ };`
    - Generate `asn1_tag<name>` = `{ universal, constructed, sequence }`
    - Generate field count constexpr and field name array for codec dispatch
    - Handle AUTOMATIC TAGS — assign context-specific tags to fields
    - Handle EXTENSIBILITY IMPLIED at module level
  - REFACTOR: Generate structured binding support

  **Must NOT do**:
  - No inheritance from base class for sequences — plain struct
  - No virtual destructor
  - No manual memory management — RAII only

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: SEQUENCE is the most complex generated type — OPTIONAL/DEFAULT, nested types, tag assignment
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 3 (with Tasks 14-17)
  - **Blocks**: Tasks 20, 21, 28, 38
  - **Blocked By**: Tasks 8, 12, 13

  **References**:
  - Task 14-17: Other generator patterns for consistency
  - ITU-T X.680: SEQUENCE type specification
  - `std::optional<T>` for OPTIONAL fields

  **Acceptance Criteria**:
  - [ ] `S ::= SEQUENCE { a INTEGER, b BOOLEAN OPTIONAL }` → correct struct with optional field
  - [ ] `S ::= SEQUENCE { a INTEGER DEFAULT 42 }` → default value in struct
  - [ ] Nested SEQUENCE generates nested struct types
  - [ ] AUTOMATIC TAGS: fields get [0], [1], ... context-specific tags

  **QA Scenarios**:

  ```
  Scenario: SEQUENCE with OPTIONAL and DEFAULT
    Tool: Bash
    Steps:
      1. Feed: "S ::= SEQUENCE { a INTEGER, b BOOLEAN OPTIONAL, c INTEGER DEFAULT 42 }"
      2. Assert: field_b is std::optional<bool>
      3. Assert: field_c is int64_t initialized to 42
      4. g++ -std=c++20 -fsyntax-only gen/S.hpp → success
    Expected Result: Complex SEQUENCE mapping correct
    Evidence: .sisyphus/evidence/task-18-sequence.txt

  Scenario: Nested SEQUENCE types
    Tool: Bash
    Steps:
      1. Feed: "Outer ::= SEQUENCE { inner SEQUENCE { x INTEGER, y INTEGER } }"
      2. Assert: generates both Outer struct and Inner struct
      3. Assert: Inner is separate type, not inline anonymous struct
    Expected Result: Nested types generated as separate named types
    Evidence: .sisyphus/evidence/task-18-nested.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named: task-18-{sequence,nested}.txt

  **Commit**: YES (Wave 3 group)

---

- [x] 19. **Tag/Length encoder + decoder**

  **What to do**:
  - RED: Test TLV encoding/decoding — short tag (≤30), long tag (≥31), short length (≤127), long length (≥128), indefinite length, constructed tag
  - GREEN: Implement `libs/codec/ber/tlv.hpp`:
    - `result<void> encode_tag(buffer_view& buf, tag t)` — encode tag class + number
    - `result<tag> decode_tag(buffer_view& buf)` — decode and return tag
    - `result<void> encode_length(buffer_view& buf, size_t len, bool indefinite = false)`
    - `result<size_t> decode_length(buffer_view& buf)` — decode length, return size
    - Handle: short form (≤127), long form (≥128, leading octet = 0x8N where N = number of subsequent octets)
    - Handle: indefinite length (0x80) for constructed types
    - `result<void> encode_end_of_content(buffer_view& buf)` — two zero octets
  - REFACTOR: Ensure encoder validates tag number fits in encoding

  **Must NOT do**:
  - No heap allocation in encode/decode
  - No BER-specific magic numbers — use named constants

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Well-specified TLV encoding per X.690 — algorithmic, not architectural
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 4 (with Tasks 20-22)
  - **Blocks**: Tasks 20, 21 (BER codec)
  - **Blocked By**: Tasks 3, 6, 7 (buffer_view, bit ops, tag registry)

  **References**:
  - ITU-T X.690 Clause 8.1: Identifier octets encoding
  - ITU-T X.690 Clause 8.2: Length octets encoding

  **Acceptance Criteria**:
  - [ ] Test: `encode_tag(buf, universal, false, 2)` → `0x02` (INTEGER tag)
  - [ ] Test: `encode_length(buf, 127)` → `0x7F` (short form)
  - [ ] Test: `encode_length(buf, 128)` → `0x81, 0x80` (long form)
  - [ ] Test: `encode_tag(buf, context_specific, false, 31)` → `0x9F, 0x1F` (two-octet tag)
  - [ ] Test: round-trip: encode_tag → decode_tag → tag matches

  **QA Scenarios**:

  ```
  Scenario: TLV round-trip all combinations
    Tool: Bash
    Steps:
      1. g++ -std=c++20 -Ilibs test/codec/ber/tlv_test.cpp -lgtest -lgtest_main -o /tmp/test_tlv
      2. /tmp/test_tlv --gtest_filter="TLV.RoundTrip*"
      3. Assert: tag values 0-200 round-trip
      4. Assert: lengths 0-10000 round-trip
      5. Assert: indefinite length encodes and decodes
    Expected Result: Full TLV fidelity
    Evidence: .sisyphus/evidence/task-19-roundtrip.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-19-roundtrip.txt

  **Commit**: YES (Wave 4 group)

- [ ] 20. **BER encoder (TDD)**

  **What to do**:
  - RED: Write tests — encode INTEGER, encode BOOLEAN, encode NULL, encode OCTET STRING, encode SEQUENCE, encode ENUMERATED; verify output bytes against known BER values
  - GREEN: Implement `libs/codec/ber/encoder.hpp`:
    - `class ber_encoder` (stateless, empty class)
    - `result<void> encode(const T& value, buffer_view& buf)` — dispatches on tag type:
      - INTEGER → encode tag + length + big-endian value
      - BOOLEAN → encode tag + length 1 + 0x00 or 0xFF
      - NULL → encode tag + length 0
      - OCTET STRING → encode tag + length + raw bytes
      - BIT STRING → encode tag + length + unused bits count + data
      - ENUMERATED → same as INTEGER
      - SEQUENCE → encode tag + construct TLV for each field in order
      - SEQUENCE OF → encode each element with its own TLV
      - OID → encode OID sub-identifiers
    - Handle OPTIONAL fields (skip if !has_value())
    - Use constexpr-if (`if constexpr`) for type-based dispatch
  - REFACTOR: Use `concept` to constrain to encodable types

  **Must NOT do**:
  - No dynamic dispatch in encode — template specialization + if constexpr
  - No state in encoder — pure function

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Core codec implementation — template metaprogramming + BER protocol logic
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 4 (with Tasks 19, 21, 22)
  - **Blocks**: Tasks 22, 28 (DER layer + integration tests)
  - **Blocked By**: Tasks 3, 4, 6, 9, 13, 14-18, 19 (buffer + traits + bit ops + interfaces + tag mapping + generators + TLV)

  **References**:
  - Generated types from Tasks 14-18 — encoder works on these types
  - ITU-T X.690: Full BER encoding specification
  - `tlv.hpp` — encode_tag, encode_length from Task 19

  **Acceptance Criteria**:
  - [ ] Test: `ber_encoder{}.encode(integer_type{42}, buf)` → `02 01 2A`
  - [ ] Test: `ber_encoder{}.encode(boolean_type{true}, buf)` → `01 01 FF`
  - [ ] Test: `ber_encoder{}.encode(null_type{}, buf)` → `05 00`
  - [ ] Test: `ber_encoder{}.encode(octet_string_type{"test"}, buf)` → `04 04 74 65 73 74`
  - [ ] Test: SEQUENCE with optional field missing → field not encoded

  **QA Scenarios**:

  ```
  Scenario: INTEGER encoding matches spec
    Tool: Bash
    Steps:
      1. Encode: integer_type{0} → assert output = {0x02, 0x01, 0x00}
      2. Encode: integer_type{127} → assert output = {0x02, 0x01, 0x7F}
      3. Encode: integer_type{128} → assert output = {0x02, 0x02, 0x00, 0x80}
      4. Encode: integer_type{-128} → assert output = {0x02, 0x01, 0x80}
    Expected Result: Correct BER INTEGER encoding
    Evidence: .sisyphus/evidence/task-20-integer.txt

  Scenario: SEQUENCE encoding
    Tool: Bash
    Steps:
      1. Define SEQUENCE { a: INTEGER(1), b: BOOLEAN(true) }
      2. Encode → assert: 30 [len] 02 01 01 01 01 FF
      3. Encode with b OPTIONAL and missing → assert: 30 [len] 02 01 01 (no boolean)
    Expected Result: Nested TLV structure correct
    Evidence: .sisyphus/evidence/task-20-sequence.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named: task-20-{integer,sequence}.txt

  **Commit**: YES (Wave 4 group)

- [ ] 21. **BER decoder (TDD)**

  **What to do**:
  - RED: Write tests — decode INTEGER, BOOLEAN, NULL, OCTET STRING, SEQUENCE from known BER bytes
  - GREEN: Implement `libs/codec/ber/decoder.hpp`:
    - `class ber_decoder` (stateless)
    - `template<typename T> result<T> decode(buffer_view& buf)` — dispatches on T's tag:
      - Read tag, assert matches expected
      - Read length
      - Dispatch on tag type to decode value
      - INTEGER → read big-endian value from length bytes
      - BOOLEAN → read 0x00 or 0xFF
      - NULL → assert length 0, return empty struct
      - OCTET STRING → copy length bytes to vector
      - SEQUENCE → decode each field in order, consume TLV boundaries
      - SEQUENCE OF → decode until end of container TLV
    - Handle OPTIONAL fields — if next tag doesn't match expected, skip
    - Handle unknown tags — return error, don't crash
  - REFACTOR: Use `if constexpr` for type dispatch

  **Must NOT do**:
  - No heap allocation for small types (INTEGER, BOOLEAN) — stack allocate
  - No unbounded memory on malicious input — length validation

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Decoder is harder than encoder — must handle malformed input gracefully
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 4 (with Tasks 19, 20, 22)
  - **Blocks**: Tasks 22, 28
  - **Blocked By**: Tasks 3, 4, 6, 9, 13, 14-18, 19

  **References**:
  - Task 20: Encoder output → decoder test vectors
  - ITU-T X.690: BER decoding rules

  **Acceptance Criteria**:
  - [ ] Test: decode `02 01 2A` → `integer_type{42}`
  - [ ] Test: decode `01 01 FF` → `boolean_type{true}`
  - [ ] Test: decode `05 00` → `null_type{}`
  - [ ] Test: decode `04 04 74 65 73 74` → octet_string with "test"
  - [ ] Test: incomplete input → error, not crash
  - [ ] Test: wrong tag → error, not misinterpret data

  **QA Scenarios**:

  ```
  Scenario: BER encode → decode round-trip
    Tool: Bash
    Steps:
      1. Create integer_type{12345}
      2. ber_encoder.encode(val, buf)
      3. ber_decoder.decode<integer_type>(buf)
      4. Assert: decoded.value == 12345
    Expected Result: Full round-trip fidelity for all core types
    Evidence: .sisyphus/evidence/task-21-roundtrip.txt

  Scenario: Malformed input handling
    Tool: Bash
    Steps:
      1. Feed: empty buffer → decode returns error
      2. Feed: wrong tag (01 instead of 02 for INTEGER) → error
      3. Feed: truncated length → error (not buffer overflow)
    Expected Result: All malformed inputs produce errors, no crashes
    Evidence: .sisyphus/evidence/task-21-malformed.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named: task-21-{roundtrip,malformed}.txt

  **Commit**: YES (Wave 4 group)

- [ ] 22. **DER canonical validation layer**

  **What to do**:
  - RED: Test that DER rejects non-canonical BER — e.g., BOOLEAN with value 0x42 (not 0x00 or 0xFF), INTEGER with unnecessary leading zero octets, constructed STRING with indefinite length
  - GREEN: Implement `libs/codec/ber/der_validator.hpp`:
    - `class der_encoder` wrapping `ber_encoder` with post-encode validation
    - Validate: BOOLEAN value is exactly 0x00 or 0xFF
    - Validate: INTEGER uses minimal encoding (no leading 0x00 for positive, no leading 0xFF for negative unless needed)
    - Validate: STRING types use primitive encoding (not constructed)
    - Validate: Length uses definite form (no indefinite length)
    - Validate: SET components are sorted by tag (DER sorting)
    - Validate: BIT STRING unused bits are 0
  - REFACTOR: DER encoder is a thin layer over BER

  **Must NOT do**:
  - No duplicate encoding logic — reuse BER encoder
  - No DER-specific encode methods — validate after BER encode

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Validation layer over existing BER encoder
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 4 (with Tasks 19-21)
  - **Blocks**: Task 28 (DER integration tests)
  - **Blocked By**: Task 20 (BER encoder)

  **References**:
  - ITU-T X.690 Clause 11: Distinguished Encoding Rules — all DER restrictions
  - Task 20: BER encoder to wrap

  **Acceptance Criteria**:
  - [ ] `der_encoder` rejects BOOLEAN with value 0x42
  - [ ] `der_encoder` accepts BOOLEAN with value 0xFF
  - [ ] `der_encoder` rejects INTEGER 0 encoded as `02 02 00 00` (should be `02 01 00`)
  - [ ] `der_encoder` rejects indefinite length in STRING

  **QA Scenarios**:

  ```
  Scenario: DER rejects non-canonical encoding
    Tool: Bash
    Steps:
      1. der_encoder.encode(boolean_type{true}, buf) → succeeds (0xFF)
      2. Manually set boolean byte to 0x42 → der_encoder.validate() returns error
    Expected Result: DER canonical rules enforced
    Evidence: .sisyphus/evidence/task-22-der.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-22-der.txt

  **Commit**: YES (Wave 4 group)

---

- [ ] 23. **CHOICE generator**

  **What to do**:
  - RED: Test CHOICE generation — `C ::= CHOICE { a INTEGER, b BOOLEAN }` generates variant type
  - GREEN: Implement `src/gen/emitter_choice.cpp`:
    - Emit `struct name { std::variant<alternative_a_type, alternative_b_type, ...> value; };`
    - Generate alternative index enum: `enum class which { a = 0, b = 1 };`
    - Generate tag dispatch helper for codec — CHOICE alternatives have context-specific tags
    - Handle extension marker `...` — add `std::vector<uint8_t> unknown_extension` alternative
  - REFACTOR: Use `std::visit` pattern for type-safe dispatch

  **Must NOT do**:
  - No `std::any` — use `std::variant` with known alternatives
  - No manual tagged union — use `std::variant`

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: CHOICE → std::variant mapping + tag assignment for each alternative
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 5 (with Tasks 24-28)
  - **Blocks**: Tasks 28, 38
  - **Blocked By**: Tasks 8, 12, 13

  **References**:
  - `std::variant<T...>`: `https://en.cppreference.com/w/cpp/utility/variant`
  - Task 18: SEQUENCE generator pattern to follow

  **Acceptance Criteria**:
  - [ ] CHOICE with 2 alternatives → `std::variant<type_a, type_b>`
  - [ ] Extension marker adds `std::vector<uint8_t>` variant
  - [ ] Tag mapping: each alternative gets context-specific [0], [1], etc.

  **QA Scenarios**:

  ```
  Scenario: CHOICE generation
    Tool: Bash
    Steps:
      1. Feed: "C ::= CHOICE { a INTEGER, b BOOLEAN }"
      2. Assert: variant type with int and bool alternatives
      3. Assert: which() helper returns correct index
      4. g++ -std=c++20 -fsyntax-only → success
    Expected Result: Correct C++ variant generation
    Evidence: .sisyphus/evidence/task-23-choice.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-23-choice.txt

  **Commit**: YES (Wave 5 group)

- [ ] 24. **SEQUENCE OF / SET OF generator**

  **What to do**:
  - RED: Test `SEQUENCE OF INTEGER` → `std::vector<int64_t>`, `SEQUENCE OF MyType` → `std::vector<MyType>`
  - GREEN: Implement `src/gen/emitter_sequence_of.cpp`:
    - SEQUENCE OF → `using name = std::vector<element_type>;` (type alias, not struct)
    - SET OF → same, but codec may require sorting
    - SIZE constraint → validate in codec, not in type
    - Tag: `sequence_of` / `set_of` universal
  - REFACTOR: Generate strong typedef wrapper if needed for type safety

  **Must NOT do**:
  - No custom container — use `std::vector`
  - No codec methods in type — codec handles ordering

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Simple type alias generation
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 5 (with Tasks 23, 25-28)
  - **Blocks**: Tasks 28, 38
  - **Blocked By**: Tasks 8, 12, 13

  **Acceptance Criteria**:
  - [ ] `SEQUENCE OF INTEGER` → type alias for `std::vector<int64_t>` with correct tag
  - [ ] `SET OF MyType` → `std::vector<MyType>` with `set_of` tag

  **QA Scenarios**:

  ```
  Scenario: SEQUENCE OF generation
    Tool: Bash
    Steps:
      1. Feed: "X ::= SEQUENCE OF INTEGER"
      2. Assert: using X = std::vector<int64_t>;
      3. g++ -std=c++20 -fsyntax-only → success
    Expected Result: Clean type alias generation
    Evidence: .sisyphus/evidence/task-24-seqof.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-24-seqof.txt

  **Commit**: YES (Wave 5 group)

- [ ] 25. **OBJECT IDENTIFIER + RELATIVE-OID generator**

  **What to do**:
  - RED: Test OID generation — value assignment, dotted notation, name form
  - GREEN: Implement `src/gen/emitter_oid.cpp`:
    - OID → `struct name { std::vector<uint32_t> arcs; };`
    - Add helper: `name from_string(const char*)` for dotted-decimal
    - RELATIVE-OID → similar with `relative_oid` tag
    - Generate well-known OID constants from value assignments
  - REFACTOR: Generate comparison operators

  **Must NOT do**:
  - No runtime OID tree resolution — arcs only
  - No ASN.1 OID registry — use string constants for well-known OIDs

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Simple struct generation with arc vector
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 5 (with Tasks 23, 24, 26-28)
  - **Blocks**: Tasks 28, 38
  - **Blocked By**: Tasks 8, 12, 13

  **Acceptance Criteria**:
  - [ ] OID → struct with `std::vector<uint32_t> arcs`
  - [ ] `from_string("1.2.840.113549")` parses correctly
  - [ ] Tag = `universal_tag::object_identifier`

  **QA Scenarios**:

  ```
  Scenario: OID generation and parsing
    Tool: Bash
    Steps:
      1. Feed: "O ::= OBJECT IDENTIFIER"
      2. Assert: struct with arcs vector
      3. Test: O::from_string("2.5.4.6") → arcs = {2, 5, 4, 6}
    Expected Result: OID correctly generated and parseable
    Evidence: .sisyphus/evidence/task-25-oid.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-25-oid.txt

  **Commit**: YES (Wave 5 group)

- [ ] 26. **Time types generator (UTCTime, GeneralizedTime)**

  **What to do**:
  - RED: Test time type generation with value formats
  - GREEN: Implement `src/gen/emitter_time.cpp`:
    - UTCTime → `struct name { std::string value; };` // or std::chrono::system_clock::time_point
    - GeneralizedTime → similar struct
    - Generate parsing helpers: `from_YMDHMS()`, `to_string()`
  - REFACTOR: Consider `std::chrono` integration for comparison/arithmetic

  **Must NOT do**:
  - No full ASN.1 time constraint validation (defer to constrained types wave)

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Simple struct with string/time_point storage
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 5 (with Tasks 23-25, 27, 28)
  - **Blocks**: Tasks 28, 38
  - **Blocked By**: Tasks 8, 12, 13

  **Acceptance Criteria**:
  - [ ] UTCTime → struct with tag
  - [ ] GeneralizedTime → struct with tag
  - [ ] Generated code compiles

  **QA Scenarios**:

  ```
  Scenario: Time type generation
    Tool: Bash
    Steps:
      1. Feed: "T1 ::= UTCTime" + "T2 ::= GeneralizedTime"
      2. Assert: both structs generated with correct tags
      3. g++ -std=c++20 -fsyntax-only → success
    Expected Result: Time types generated correctly
    Evidence: .sisyphus/evidence/task-26-time.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-26-time.txt

  **Commit**: YES (Wave 5 group)

- [ ] 27. **PER constraint metadata emitter**

  **What to do**:
  - RED: Test that PER metadata is emitted for constrained types — value ranges, size constraints, alphabet constraints
  - GREEN: Implement `src/gen/emitter_per_meta.cpp`:
    - For each constrained type, emit metadata struct:
      - INTEGER with range → min/max for PER normative/integer range
      - OCTET STRING with SIZE → lower/upper bound for PER length determinant
      - ENUMERATED → index range (0..N-1) for PER index encoding
      - SEQUENCE extension marker → emit extension bit position
      - CHOICE extension marker → emit extension index
    - Emit as `constexpr` static data, not runtime tables
    - Metadata consumed by PER encoder/decoder Tasks 29-32
  - REFACTOR: Use `consteval` if compiler supports it

  **Must NOT do**:
  - No runtime constraint tables — constexpr only
  - No duplication of constraint data between PER metadata and validate()

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: PER constraint propagation from ASN.1 constraints to C++ constexpr metadata — requires semantic analysis
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 5 (with Tasks 23-26, 28)
  - **Blocks**: Tasks 29-32 (PER encoder/decoder depend on metadata)
  - **Blocked By**: Tasks 8, 12, 13, 14, 15 (need generated types with constraints)

  **References**:
  - ITU-T X.691: PER encoding rules — constraint metadata requirements
  - Tasks 14-15: Constrained INTEGER and STRING generators produce the constraints to analyze

  **Acceptance Criteria**:
  - [ ] INTEGER (0..255) emits `constexpr int64_t per_min = 0, per_max = 255;`
  - [ ] OCTET STRING (SIZE(1..32)) emits size bounds
  - [ ] ENUMERATED emits max index for PER index range
  - [ ] SEQUENCE with `...` emits extension marker metadata

  **QA Scenarios**:

  ```
  Scenario: PER metadata for constrained types
    Tool: Bash
    Steps:
      1. Feed: "X ::= INTEGER (0..255)" + "Y ::= OCTET STRING (SIZE(1..32))"
      2. Assert: X_PER_meta has min=0, max=255
      3. Assert: Y_PER_meta has size_lower=1, size_upper=32
    Expected Result: Correct constraint metadata for PER codec
    Evidence: .sisyphus/evidence/task-27-permeta.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-27-permeta.txt

  **Commit**: YES (Wave 5 group)

- [ ] 28. **DER integration tests with X.509 schema**

  **What to do**:
  - RED: Test that generated X.509 types DER-encode and match known test vectors
  - GREEN: Implement `test/integration/der_x509_test.cpp`:
    - Create curated subset of X.509 ASN.1 (Certificate, TBSCertificate, etc.)
    - Run code generator → compile generated types
    - Known DER test vector: a self-signed certificate or well-known cert
    - Decode with ber_decoder → verify fields
    - Re-encode with der_encoder → compare to original (should be identical for DER)
  - REFACTOR: Add CI-friendly test that runs end-to-end

  **Must NOT do**:
  - No full X.509 implementation — just enough types to verify DER codec integrity
  - No real certificate generation — test with pre-computed vectors

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Integration test tying together code generator, generated types, BER/DER codec
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: NO (sequential)
  - **Parallel Group**: Wave 5 final (runs after Tasks 23-27)
  - **Blocks**: Task 40 (full end-to-end)
  - **Blocked By**: Tasks 14-18, 20, 21, 22 (generators + BER encoder + decoder + DER)

  **References**:
  - RFC 5280: X.509 certificate ASN.1 module
  - Known DER test vectors: `test/data/x509_test.der`

  **Acceptance Criteria**:
  - [ ] Generated X.509 types compile
  - [ ] Decode known DER certificate → structured data
  - [ ] Re-encode → bytes match original
  - [ ] Test passes in CI

  **QA Scenarios**:

  ```
  Scenario: X.509 DER round-trip
    Tool: Bash
    Steps:
      1. asn1pp-gen --input test/data/x509.asn --output gen/x509/
      2. g++ -std=c++20 gen/x509/*.cpp test/integration/der_x509_test.cpp -lgtest -lgtest_main -o /tmp/test_x509
      3. /tmp/test_x509
      4. Assert: decoded certificate fields non-null
      5. Assert: re-encoded bytes == original bytes (DER deterministic)
    Expected Result: Full DER round-trip on real X.509 data
    Evidence: .sisyphus/evidence/task-28-x509.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-28-x509.txt

  **Commit**: YES (Wave 5 group)

- [ ] 29. **PER aligned encoder (TDD)**

  **What to do**:
  - RED: Write tests — encode simple INTEGER with known PER output, SEQUENCE with PER, OCTET STRING
  - GREEN: Implement `libs/codec/per/encoder.hpp`:
    - `class per_aligned_encoder` — stateless
    - `result<void> encode(const T& value, buffer_view& buf)`:
      - Uses PER constraint metadata (Task 27) for bit-widths
      - INTEGER → encode value relative to range, using minimum bits: `ceil(log2(range_max - range_min + 1))`
      - BOOLEAN → 1 bit
      - NULL → 0 bits (nothing encoded)
      - OCTET STRING → length determinant (constrained or unconstrained) + data
      - BIT STRING → length + data with unused bits
      - ENUMERATED → index within range
      - SEQUENCE → encode each field; extension bit if present; OPTIONAL fields get presence bit
      - CHOICE → choice index bits + encoded alternative
      - SEQUENCE OF → count + repeated elements
      - OID → encoded per PER rules
    - All output aligned to octet boundaries after each TLV
  - REFACTOR: Use PER metadata structs for bit widths

  **Must NOT do**:
  - No BER-style TLV — PER has no tags or lengths for most types
  - No runtime constraint resolution — all from constexpr metadata

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: PER is the most complex binary encoding — constraint-driven bit allocation
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 6 (with Tasks 30-32)
  - **Blocks**: Tasks 31, 41
  - **Blocked By**: Tasks 3, 4, 6, 9, 27 (buffer + traits + bit ops + interfaces + PER metadata)

  **References**:
  - ITU-T X.691: PER encoding — complete specification
  - Task 27: PER constraint metadata consumed here
  - Task 6: `bit_ops.hpp` — write_bits for bit-level output

  **Acceptance Criteria**:
  - [ ] INTEGER (0..255) → encoded value in 8 bits
  - [ ] INTEGER (0..65535) → encoded value in 16 bits
  - [ ] BOOLEAN → exactly 1 bit
  - [ ] SEQUENCE with OPTIONAL → presence bit + conditional field
  - [ ] SEQUENCE with extension → extension bit

  **QA Scenarios**:

  ```
  Scenario: PER aligned INTEGER encoding
    Tool: Bash
    Steps:
      1. Encode integer_type{42} with constraint (0..255)
      2. Assert: output is exactly 8 bits = 0x2A (no tag, no length)
      3. Encode integer_type{1000} with constraint (0..65535)
      4. Assert: output is 16 bits = 0x03E8
    Expected Result: PER compact encoding without TLV overhead
    Evidence: .sisyphus/evidence/task-29-integer.txt

  Scenario: PER SEQUENCE with OPTIONAL
    Tool: Bash
    Steps:
      1. SEQUENCE { a INTEGER, b BOOLEAN OPTIONAL }
      2. Encode with b present → assert presence bit = 1 + b value (1 bit)
      3. Encode with b absent → assert presence bit = 0 (no b value)
    Expected Result: OPTIONAL correctly handled with presence bits
    Evidence: .sisyphus/evidence/task-29-optional.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named: task-29-{integer,optional}.txt

  **Commit**: YES (Wave 6 group)

- [ ] 30. **PER aligned decoder (TDD)**

  **What to do**:
  - RED: Decode PER bytes for INTEGER, BOOLEAN, SEQUENCE, OCTET STRING
  - GREEN: Implement `libs/codec/per/decoder.hpp`:
    - `class per_aligned_decoder` — stateless
    - `template<typename T> result<T> decode(buffer_view& buf)`:
      - Uses PER constraint metadata for bit reads
      - Inverse of encoder: read minimum bits, reconstruct value
      - Handle OPTIONAL presence bits → conditionally decode field
      - Handle extension bit → if set, decode unknown extensions and skip
      - Validate decoded values against constraints
  - REFACTOR: Match encoder bit-for-bit

  **Must NOT do**:
  - No dynamic allocation for small types
  - No assumptions about input buffer alignment

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: PER decoder must exactly inverse the encoder
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 6 (with Tasks 29, 31, 32)
  - **Blocks**: Tasks 32, 41
  - **Blocked By**: Tasks 3, 4, 6, 9, 27

  **References**:
  - Task 29: Encoder → test vectors for decoder
  - ITU-T X.691: PER decoding

  **Acceptance Criteria**:
  - [ ] PER encode → decode round-trip for all core types
  - [ ] Decode known PER test vector for 3GPP RRC (from published test data)
  - [ ] Extension handling: skip unknown extensions

  **QA Scenarios**:

  ```
  Scenario: PER aligned round-trip
    Tool: Bash
    Steps:
      1. Encode complex SEQUENCE with PER
      2. Decode with per_aligned_decoder
      3. Assert: decoded == original (all fields)
    Expected Result: PER round-trip fidelity
    Evidence: .sisyphus/evidence/task-30-roundtrip.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-30-roundtrip.txt

  **Commit**: YES (Wave 6 group)

- [ ] 31. **PER unaligned (UPER) encoder**

  **What to do**:
  - RED: Test UPER encoding — NO octet alignment between fields, bit-packing across boundaries
  - GREEN: Implement `libs/codec/per/uper_encoder.hpp`:
    - `class uper_encoder` — thin wrapper over `per_aligned_encoder` removing alignment
    - Key difference: bits are packed continuously, no padding to octet boundaries
    - Reuse aligned encoding logic but track bit position, not octet position
    - Final padding to octet boundary only at end of entire PDU
  - REFACTOR: Share encoding logic with aligned, differ only in flush strategy

  **Must NOT do**:
  - No duplicate encoding code — reuse aligned encoder internals
  - No octet alignment between individual elements

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: UPER is a variant of aligned PER — careful bit-packing logic
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 6 (with Tasks 29, 30, 32)
  - **Blocks**: Tasks 41
  - **Blocked By**: Task 29 (PER aligned encoder — share logic)

  **References**:
  - ITU-T X.691: UPER — same encoding rules, different alignment
  - Task 29: Aligned encoder internals

  **Acceptance Criteria**:
  - [ ] UPER output is more compact than aligned for same data
  - [ ] No octet-boundary padding between fields
  - [ ] Final PDU padded to full octet

  **QA Scenarios**:

  ```
  Scenario: UPER vs aligned size comparison
    Tool: Bash
    Steps:
      1. Encode SEQUENCE{BOOL, BOOL} with aligned → 2 bytes (1 bit + 7 pad + 1 bit + 7 pad)
      2. Encode same with UPER → 1 byte (2 bits + 6 pad)
      3. Assert: UPER smaller than aligned
    Expected Result: UPER more compact than aligned
    Evidence: .sisyphus/evidence/task-31-compact.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-31-compact.txt

  **Commit**: YES (Wave 6 group)

- [ ] 32. **PER unaligned (UPER) decoder**

  **What to do**:
  - RED: Decode UPER (unaligned) bytes
  - GREEN: Implement `libs/codec/per/uper_decoder.hpp`:
    - `class uper_decoder` — thin wrapper removing octet alignment expectation
    - Read bits continuously, not at octet boundaries
    - Final alignment check at end of decode
  - REFACTOR: Share decoder logic with aligned

  **Must NOT do**:
  - No duplicate decode code — reuse aligned decoder

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: UPER decoder mirroring encoder
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 6 (with Tasks 29-31)
  - **Blocks**: Task 41
  - **Blocked By**: Tasks 30, 31 (aligned decoder + UPER encoder)

  **Acceptance Criteria**:
  - [ ] UPER round-trip: encode → decode → compare
  - [ ] Decode known UPER test vector

  **QA Scenarios**:

  ```
  Scenario: UPER round-trip
    Tool: Bash
    Steps:
      1. UPER encode complex type
      2. UPER decode
      3. Assert: decoded == original
    Expected Result: UPER round-trip fidelity
    Evidence: .sisyphus/evidence/task-32-roundtrip.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-32-roundtrip.txt

  **Commit**: YES (Wave 6 group)

---

- [ ] 33. **OER encoder + decoder (TDD)**

  **What to do**:
  - RED: Test OER encoding/decoding for all core types — OER uses fixed-width INTEGERs, length prefixes for strings
  - GREEN: Implement `libs/codec/oer/encoder.hpp` + `decoder.hpp`:
    - OER (X.696): Octet Encoding Rules — binary, octet-aligned, simpler than PER
    - INTEGER: fixed-width encoding based on constraint range (8/16/32/64 bit), or variable-length with length prefix if unconstrained
    - BOOLEAN: 1 octet (0x00/0xFF) — same as BER
    - NULL: 0 bytes
    - OCTET STRING: length prefix (constrained or not) + data
    - BIT STRING: length prefix + data (with unused bits in last octet, OR padding to full octet depending on rule)
    - ENUMERATED: encoded as INTEGER index
    - SEQUENCE: each field in order, no wrapper TLV — lengths can be determined positionally or prefixed
    - CHOICE: choice index prefix + alternative
    - OID: OER OID encoding
  - REFACTOR: Stateless encoder/decoder using traits dispatch

  **Must NOT do**:
  - No BER-style TLV wrapping — OER is positional/prefix-based
  - No PER-style bit packing — OER is octet-aligned

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: New encoding rule — medium complexity, octet-aligned like BER but simpler structure
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 7 (with Tasks 34-37)
  - **Blocks**: Task 34 (COER)
  - **Blocked By**: Tasks 3, 4, 9 (buffer + traits + interfaces)

  **References**:
  - ITU-T X.696: OER specification
  - Task 20-21: BER codec patterns for octet-level encoding

  **Acceptance Criteria**:
  - [ ] INTEGER (0..255) → 1 octet in OER
  - [ ] Unconstrained INTEGER → length-prefixed
  - [ ] OER round-trip for all core types

  **QA Scenarios**:

  ```
  Scenario: OER INTEGER encoding
    Tool: Bash
    Steps:
      1. Encode integer_type{42} with constraint (0..255)
      2. Assert: output is 1 octet = 0x2A
      3. Encode integer_type{1000} without constraint
      4. Assert: output has length prefix + value octets
    Expected Result: OER size-optimized for constrained integers
    Evidence: .sisyphus/evidence/task-33-oer.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-33-oer.txt

  **Commit**: YES (Wave 7 group)

- [ ] 34. **COER canonical layer**

  **What to do**:
  - RED: Test COER canonical validation — rejects non-minimal length prefixes, non-canonical choice index
  - GREEN: Implement `libs/codec/oer/coer_validator.hpp`:
    - Wraps OER encoder with canonical checks
    - Validate: minimal length prefix encoding
    - Validate: SET components sorted (like DER)
    - Validate: canonical choice index
  - REFACTOR: Thin layer over OER encoder

  **Must NOT do**:
  - No duplicate OER encoding logic

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Validation layer — similar pattern to DER validator (Task 22)
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 7 (with Tasks 33, 35-37)
  - **Blocks**: None downstream
  - **Blocked By**: Task 33 (OER encoder)

  **References**:
  - ITU-T X.696 Annex: COER canonical rules
  - Task 22: DER validator pattern to follow

  **Acceptance Criteria**:
  - [ ] COER rejects non-minimal length prefix
  - [ ] COER accepts canonical encoding

  **QA Scenarios**:

  ```
  Scenario: COER canonical validation
    Tool: Bash
    Steps:
      1. Manually construct OER with inflated length prefix
      2. coer_validator.validate() → error
      3. Construct canonical OER → validate succeeds
    Expected Result: COER rules enforced
    Evidence: .sisyphus/evidence/task-34-coer.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-34-coer.txt

  **Commit**: YES (Wave 7 group)

- [ ] 35. **XER encoder + decoder (TDD)**

  **What to do**:
  - RED: Test XER XML round-trip — produce expected XML structure, parse back
  - GREEN: Implement `libs/codec/xer/encoder.hpp` + `decoder.hpp`:
    - XER (X.693): Basic XML Encoding Rules
    - Encoder: produce XML with ASN.1 type names as XML element names
    - SEQUENCE → `<TypeName><field1>val</field1><field2>val</field2></TypeName>`
    - CHOICE → `<TypeName><chosenAlternative>val</chosenAlternative></TypeName>`
    - INTEGER → text decimal representation
    - BOOLEAN → `<true/>` or `<false/>`
    - NULL → `<TypeName/>`
    - OPTIONAL absent → element omitted (not empty element)
    - Decoder: parse XML (simple XML parser, no full DOM), reconstruct types
  - REFACTOR: Minimal XML parser — no external XML library dependency

  **Must NOT do**:
  - No external XML library — hand-written simple XML parser is fine for XER subset
  - No namespace support initially (CXER/E-XER handles that)

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Text encoding with hand-written XML parser — significant implementation
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 7 (with Tasks 33, 34, 36, 37)
  - **Blocks**: Tasks 36
  - **Blocked By**: Tasks 3, 4, 9

  **References**:
  - ITU-T X.693: XER specification
  - Task 14-18: Generated types → element names

  **Acceptance Criteria**:
  - [ ] XER encode SEQUENCE → valid XML
  - [ ] XER decode XML → reconstructed type
  - [ ] Round-trip for all core types

  **QA Scenarios**:

  ```
  Scenario: XER round-trip
    Tool: Bash
    Steps:
      1. Encode SEQUENCE{a:1, b:true} → "<Seq><a>1</a><b><true/></b></Seq>"
      2. Decode back → assert a=1, b=true
    Expected Result: XML round-trip fidelity
    Evidence: .sisyphus/evidence/task-35-xer.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-35-xer.txt

  **Commit**: YES (Wave 7 group)

- [ ] 36. **CXER + E-XER layers**

  **What to do**:
  - RED: Test CXER canonical rules, E-XER extended markup
  - GREEN: Implement `libs/codec/xer/cxer_validator.hpp` + `libs/codec/xer/exer_encoder.hpp`:
    - CXER (Canonical XER): subset of XER with deterministic ordering, no optional whitespace
    - E-XER (Extended XER): adds XML attributes for ASN.1 metadata, supports namespaces, type annotations
    - CXER: validate whitespace rules, element ordering, attribute ordering
    - E-XER: encode with `xmlns:asn1` attributes for type identification
  - REFACTOR: Reuse XER encoder/decoder internals

  **Must NOT do**:
  - No full XML Schema generation — XSD is complex, defer
  - No duplicate XER logic

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Validation + attribute layers over existing XER
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 7 (with Tasks 33-35, 37)
  - **Blocks**: None downstream
  - **Blocked By**: Task 35 (XER encoder/decoder)

  **References**:
  - ITU-T X.693 Annex A: CXER
  - ITU-T X.693 Annex B: E-XER

  **Acceptance Criteria**:
  - [ ] CXER rejects non-canonical whitespace
  - [ ] E-XER output includes ASN.1 namespace attributes

  **QA Scenarios**:

  ```
  Scenario: CXER canonical validation
    Tool: Bash
    Steps:
      1. Generate non-canonical XER (extra whitespace)
      2. cxer_validator.validate() → error
    Expected Result: CXER rules enforced
    Evidence: .sisyphus/evidence/task-36-cxer.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-36-cxer.txt

  **Commit**: YES (Wave 7 group)

- [ ] 37. **JER encoder + decoder (TDD)**

  **What to do**:
  - RED: Test JER JSON round-trip — produce expected JSON, parse back
  - GREEN: Implement `libs/codec/jer/encoder.hpp` + `decoder.hpp`:
    - JER (X.697): JSON Encoding Rules
    - Encoder: produce JSON object/array structure
    - SEQUENCE → `{"field1": val, "field2": val}`
    - CHOICE → `{"chosenAlt": val}` (single key object)
    - INTEGER → JSON number
    - BOOLEAN → `true`/`false`
    - NULL → `null`
    - OCTET STRING → base64-encoded string (JER convention) or hex string
    - BIT STRING → base64 or hex string, plus unused bits metadata
    - ENUMERATED → string name
    - OID → dotted-decimal string
    - SEQUENCE OF → JSON array
    - OPTIONAL absent → key omitted
    - Extension → `...` key with JSON value
    - Decoder: parse JSON (simple JSON parser, no full DOM), reconstruct types
  - REFACTOR: Minimal JSON parser — no external JSON dependency

  **Must NOT do**:
  - No external JSON library — hand-written parser for JER subset
  - No JSON Schema generation

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: JSON is the most commonly requested text encoding — worth doing well
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 7 (with Tasks 33-36)
  - **Blocks**: None downstream
  - **Blocked By**: Tasks 3, 4, 9

  **References**:
  - ITU-T X.697: JER specification
  - RFC 8259: JSON

  **Acceptance Criteria**:
  - [ ] JER encode SEQUENCE → `{"a": 1, "b": true}`
  - [ ] JER decode JSON → reconstructed type
  - [ ] OCTET STRING → base64 string (or hex with configuration)
  - [ ] Round-trip for all core types

  **QA Scenarios**:

  ```
  Scenario: JER JSON round-trip
    Tool: Bash
    Steps:
      1. Encode SEQUENCE{a:1, b:true} → JSON
      2. Assert: valid JSON with correct keys
      3. Decode → assert a=1, b=true
    Expected Result: JSON round-trip fidelity
    Evidence: .sisyphus/evidence/task-37-jer.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-37-jer.txt

  **Commit**: YES (Wave 7 group)

---

- [ ] 38. **CLI tool (`asn1pp-gen` main, args, file I/O)**

  **What to do**:
  - RED: Test CLI — `asn1pp-gen --input foo.asn --output gen/` produces output files, handles errors
  - GREEN: Implement `src/gen/main.cpp` + `src/gen/cli.cpp`:
    - Parse command-line: `--input <file>`, `--output <dir>`, `--encoding <rule>` (optional, for targeted generation), `--help`, `--version`
    - Read input .asn file
    - Run lexer → parser → diagnostic check → code emitter pipeline
    - Write generated .hpp/.cpp files to output directory
    - Return exit code 0 on success, non-zero on errors
    - Print diagnostics to stderr
    - Add `--dry-run` flag: parse + validate but don't emit files
  - REFACTOR: Separate concerns — main just wires pipeline, cli handles args

  **Must NOT do**:
  - No hardcoded paths
  - No iostream for logging — use stderr for diagnostics, stdout for --help output only
  - No interactive mode — batch tool only

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: End-to-end pipeline integration — ties parser, emitters, file I/O together
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 8 (with Tasks 39-42)
  - **Blocks**: Tasks 39, 40, 41 (CMake module + e2e tests)
  - **Blocked By**: Tasks 12, 14-18, 23-27 (parser + all generators)

  **References**:
  - Tasks 11-12: Lexer + parser to invoke
  - Tasks 14-18, 23-27: Code emitters to invoke

  **Acceptance Criteria**:
  - [ ] `./asn1pp-gen --input test/data/simple.asn --output /tmp/gen/` → creates .hpp files
  - [ ] `./asn1pp-gen --input test/data/bad.asn` → prints errors to stderr, exit code != 0
  - [ ] `./asn1pp-gen --help` → prints usage
  - [ ] Generated files compile with C++20

  **QA Scenarios**:

  ```
  Scenario: Happy path CLI
    Tool: Bash
    Steps:
      1. echo "X ::= INTEGER" > /tmp/test.asn
      2. ./build/bin/asn1pp-gen --input /tmp/test.asn --output /tmp/gen/
      3. Assert: exit code 0
      4. ls /tmp/gen/X.hpp && assert file exists
      5. g++ -std=c++20 -fsyntax-only /tmp/gen/X.hpp → success
    Expected Result: CLI generates compilable C++ from ASN.1
    Evidence: .sisyphus/evidence/task-38-happy.txt

  Scenario: Error handling
    Tool: Bash
    Steps:
      1. echo "X ::= UNKNOWN" > /tmp/bad.asn
      2. ./build/bin/asn1pp-gen --input /tmp/bad.asn --output /tmp/gen/ 2>&1
      3. Assert: exit code != 0
      4. Assert: stderr contains error message with line number
    Expected Result: Errors reported clearly
    Evidence: .sisyphus/evidence/task-38-error.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named: task-38-{happy,error}.txt

  **Commit**: YES (Wave 8 group)

- [ ] 39. **CMake integration module (`asn1pp_generate`)**

  **What to do**:
  - RED: Test CMake module — `asn1pp_generate(TARGET mylib INPUT schema.asn)` creates build dependency
  - GREEN: Implement `cmake/asn1pp.cmake`:
    - `function(asn1pp_generate)`:
      - Parameters: `TARGET`, `INPUT` (ASN.1 file), `OUTPUT_DIR` (optional, default `${CMAKE_CURRENT_BINARY_DIR}/gen`)
      - Creates custom command: runs `asn1pp-gen --input ${INPUT} --output ${OUTPUT_DIR}`
      - Adds generated sources to target
      - Tracks dependency: if INPUT changes, regenerate
    - Define: `asn1pp_generate(TARGET mylib INPUT ${CMAKE_CURRENT_SOURCE_DIR}/schema.asn)`
    - `find_package(asn1pp)` support — export asn1pp targets
  - REFACTOR: Handle multiple .asn files, dependency tracking

  **Must NOT do**:
  - No hardcoded `asn1pp-gen` path — locate via `find_program` or imported target
  - No modification of caller's CMakeLists beyond adding custom command

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: CMake module — well-defined pattern (like protobuf's `protobuf_generate_cpp`)
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 8 (with Tasks 38, 40-42)
  - **Blocks**: None downstream
  - **Blocked By**: Task 38 (CLI must exist to test CMake module)

  **References**:
  - `protobuf_generate_cpp()` — proven CMake pattern for code generators
  - CMake `add_custom_command` + `add_custom_target` docs

  **Acceptance Criteria**:
  - [ ] CMake project using `asn1pp_generate(TARGET mylib INPUT schema.asn)` builds
  - [ ] Changing .asn file triggers regeneration on next build
  - [ ] Generated sources compiled into target correctly

  **QA Scenarios**:

  ```
  Scenario: CMake integration end-to-end
    Tool: Bash
    Steps:
      1. Create test CMake project with asn1pp_generate()
      2. cmake -B build && cmake --build build
      3. Assert: asn1pp-gen invoked during build
      4. Assert: generated files compiled into target
      5. Modify .asn file, rebuild → assert: generator re-runs
    Expected Result: Seamless build integration
    Evidence: .sisyphus/evidence/task-39-cmake.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-39-cmake.txt

  **Commit**: YES (Wave 8 group)

- [ ] 40. **End-to-end test: X.509 DER round-trip**

  **What to do**:
  - Test: full pipeline — .asn → asn1pp-gen → compile → load DER cert → decode → re-encode → compare
  - Implement `test/e2e/x509_der_test.cpp`:
    - Uses curated X.509 ASN.1 module (TBSCertificate, AlgorithmIdentifier, etc.)
    - Load test certificate DER bytes from file
    - Decode with `ber_decoder`
    - Verify: issuer, subject, serial number fields populated
    - Re-encode with `der_encoder`
    - Assert: re-encoded bytes == original (DER is deterministic)
  - Add as CI test in `test/CMakeLists.txt`

  **Must NOT do**:
  - No certificate validation (signature check) — just ASN.1 encode/decode
  - No full X.509 chain support — single certificate

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: End-to-end verification — validates entire DER pipeline
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 8 (with Tasks 38, 39, 41, 42)
  - **Blocks**: Task F1 (compliance audit checks this)
  - **Blocked By**: Tasks 28, 38 (DER integration + CLI)

  **References**:
  - Task 28: DER integration test — this extends it with CLI pipeline
  - RFC 5280: X.509 ASN.1 module

  **Acceptance Criteria**:
  - [ ] E2E test passes in CI
  - [ ] DER round-trip on real certificate
  - [ ] No crash, no undefined behavior

  **QA Scenarios**:

  ```
  Scenario: Full X.509 pipeline
    Tool: Bash
    Steps:
      1. asn1pp-gen --input test/data/x509.asn --output gen/
      2. Build test binary including generated code
      3. Run: load DER → decode → re-encode → compare
      4. Assert: bytes match, fields populated
    Expected Result: Complete pipeline works end-to-end
    Evidence: .sisyphus/evidence/task-40-x509-e2e.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-40-x509-e2e.txt

  **Commit**: YES (Wave 8 group)

- [ ] 41. **End-to-end test: 3GPP RRC PER round-trip**

  **What to do**:
  - Test: 3GPP RRC fragment → PER encode/decode
  - Implement `test/e2e/rrc_per_test.cpp`:
    - Curate small 3GPP RRC ASN.1 fragment (e.g., RRCSetupRequest or similar small message)
    - Generate C++ with asn1pp-gen
    - Create known PER test vector (from 3GPP test data or hand-computed small example)
    - PER decode → verify fields → PER re-encode → compare
  - Add as CI test

  **Must NOT do**:
  - No full RRC module — fragment only
  - No 3GPP test data redistribution without license check

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: PER end-to-end validation on real telecom protocol
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 8 (with Tasks 38-40, 42)
  - **Blocks**: Task F1
  - **Blocked By**: Tasks 29-32, 38 (PER codec + CLI)

  **References**:
  - 3GPP TS 36.331: RRC protocol specification (ASN.1 in Annex)
  - Tasks 29-32: PER codec

  **Acceptance Criteria**:
  - [ ] RRC fragment PER round-trip
  - [ ] E2E test passes in CI

  **QA Scenarios**:

  ```
  Scenario: RRC PER pipeline
    Tool: Bash
    Steps:
      1. asn1pp-gen --input test/data/rrc_fragment.asn --output gen/
      2. Build test + run PER encode/decode
      3. Assert: round-trip fidelity
    Expected Result: PER works on telecom protocol
    Evidence: .sisyphus/evidence/task-41-rrc.txt
  ```

  **Evidence to Capture**:
  - [ ] Evidence file: task-41-rrc.txt

  **Commit**: YES (Wave 8 group)

- [ ] 42. **Documentation (README, usage guide, API reference)**

  **What to do**:
  - Write comprehensive `README.md`:
    - What asn1pp is and isn't
    - Quick start: install, write .asn, generate, use
    - Supported encoding rules table
    - ASN.1 → C++ mapping reference (INTEGER → int64_t, SEQUENCE → struct, etc.)
    - Codec usage examples (encode, decode for each encoding rule)
    - CLI reference: all flags, examples
    - CMake integration guide
    - FAQ / common pitfalls
  - Write `CONTRIBUTING.md` if desired
  - Add architecture overview doc in `docs/architecture.md`

  **Must NOT do**:
  - No tutorial for ASN.1 itself — link to ITU-T
  - No generated API docs (Doxygen) unless explicitly requested

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Documentation — prose-heavy task
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - All: No project skill overlap

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 8 (with Tasks 38-41)
  - **Blocks**: None downstream
  - **Blocked By**: None significant (can document patterns from earlier waves)

  **Commit**: YES (Wave 8 group)

---

- [ ] 43. **Tagged types (IMPLICIT/EXPLICIT) generator + codec**

  **What to do**:
  - Parse `[APPLICATION 1] IMPLICIT INTEGER`, `[2] EXPLICIT SEQUENCE`
  - Generator: for IMPLICIT → emit type with overridden tag; for EXPLICIT → emit wrapper with original type inside
  - Codec: BER/DER handles tagged types — IMPLICIT replaces tag, EXPLICIT adds outer constructed TLV
  - PER: IMPLICIT tagged type — tag not encoded (context-specific), but encoding rules of underlying type apply

  **Recommended Agent Profile**: `deep`
  **Parallelization**: Wave 9 (with Tasks 44-46)
  **Commit**: YES (Wave 9 group)

- [ ] 44. **Constrained types (value range, size, alphabet)**

  **What to do**:
  - Parse `INTEGER (0..255 | 1000)`, `OCTET STRING (SIZE(1..32))`, `BIT STRING (SIZE(16))`, alphabet constraints
  - Generate compile-time constraint metadata (extending Task 27)
  - Generate runtime validation (extending validate() in generated types)
  - PER: constraints drive bit allocation — critical for PER correctness
  - OER: constraints determine fixed-width encoding

  **Recommended Agent Profile**: `deep`
  **Parallelization**: Wave 9 (with Tasks 43, 45, 46)
  **Commit**: YES (Wave 9 group)

- [ ] 45. **Information object class support (X.681)**

  **What to do**:
  - Parse `CLASS`, `WITH SYNTAX`, information object definitions, object sets
  - Semantic analysis: resolve `@field` references, table constraint linkage
  - Generate C++ equivalents: CLASS → concept/traits, object → constexpr data, object set → std::array of objects
  - This is the hardest part — asn1c's IOC support is incomplete. Research-grade work.
  - Expected scope: basic CLASS without full table constraint resolution in this wave

  **Recommended Agent Profile**: `deep`
  **Parallelization**: Wave 9 (with Tasks 43, 44, 46)
  **Commit**: YES (Wave 9 group)

- [ ] 46. **Parameterized types + table constraints (X.682-X.683)**

  **What to do**:
  - Parse parameterized type definitions: `MyType { INTEGER : param } ::= SEQUENCE { field param }`
  - Resolve parameter substitution in type instances
  - Table constraints: link information object sets to component types
  - Generate: C++ templates for parameterized types, with concept constraints based on parameter types

  **Recommended Agent Profile**: `deep`
  **Parallelization**: Wave 9 (with Tasks 43-45)
  **Commit**: YES (Wave 9 group)

---

### Dependency Matrix

```
Task   | Blocked By          | Blocks
-------|---------------------|------------------------
1      | -                   | 2-8
2      | 1                   | 3-46 (all test tasks)
3      | 1,2                 | 6,9,19-21,29-37
4      | 1,2                 | 9,13-18,20,21,29-37
5      | 1,2                 | 9,20,21,29-37
6      | 1,2,3               | 19-21,29-37
7      | 1,2                 | 13,19-21
8      | 1                   | 11,12,14-18,23-27,38
9      | 3,4,5               | 20,21,29-37
10     | 1                   | 11,12,38
11     | 8,10                | 12,38
12     | 8,11                | 14-18,23-27,38
13     | 4,7                 | 14-18,20,21
14-17  | 8,12,13             | 20,21,28,38
18     | 8,12,13             | 20,21,28,38
19     | 3,6,7               | 20,21
20     | 3,4,6,9,13,14-18,19 | 22,28
21     | 3,4,6,9,13,14-18,19 | 22,28
22     | 20                  | 28
23-26  | 8,12,13             | 28,38
27     | 8,12,13,14,15       | 29-32
28     | 14-18,20-22         | 40
29     | 3,4,6,9,27          | 31,41
30     | 3,4,6,9,27          | 32,41
31     | 29                  | 41
32     | 30,31               | 41
33     | 3,4,9               | 34
34     | 33                  | -
35-37  | 3,4,9               | -
38     | 12,14-18,23-27      | 39,40,41
39     | 38                  | -
40     | 28,38               | F1
41     | 29-32,38            | F1
42     | -                   | -
43-46  | Prior generators    | -
```

### Agent Dispatch Summary

- **Wave 1**: **7** — T1→`quick`, T2→`quick`, T3→`deep`, T4→`deep`, T5→`quick`, T6→`quick`, T7→`quick`
- **Wave 2**: **6** — T8→`quick`, T9→`deep`, T10→`quick`, T11→`deep`, T12→`deep`, T13→`quick`
- **Wave 3**: **5** — T14→`quick`, T15→`quick`, T16→`quick`, T17→`quick`, T18→`deep`
- **Wave 4**: **4** — T19→`quick`, T20→`deep`, T21→`deep`, T22→`quick`
- **Wave 5**: **6** — T23→`deep`, T24→`quick`, T25→`quick`, T26→`quick`, T27→`deep`, T28→`deep`
- **Wave 6**: **4** — T29→`deep`, T30→`deep`, T31→`deep`, T32→`deep`
- **Wave 7**: **5** — T33→`deep`, T34→`quick`, T35→`deep`, T36→`quick`, T37→`deep`
- **Wave 8**: **5** — T38→`deep`, T39→`quick`, T40→`deep`, T41→`deep`, T42→`writing`
- **Wave 9**: **4** — T43→`deep`, T44→`deep`, T45→`deep`, T46→`deep`
- **FINAL**: **4** — F1→`oracle`, F2→`unspecified-high`, F3→`unspecified-high`, F4→`deep`

---

## Final Verification Wave (MANDATORY — after ALL implementation tasks)

> 4 review agents run in PARALLEL. ALL must APPROVE. Present consolidated results to user and get explicit "okay" before completing.
>
> **Do NOT auto-proceed after verification. Wait for user's explicit approval.**

- [ ] F1. **Plan Compliance Audit** — `oracle`
  Read the plan end-to-end. For each "Must Have": verify implementation exists (read file, run command). For each "Must NOT Have": search codebase for forbidden patterns — reject with file:line if found. Check evidence files exist in .sisyphus/evidence/. Compare deliverables against plan.
  Output: `Must Have [N/N] | Must NOT Have [N/N] | Tasks [N/N] | VERDICT: APPROVE/REJECT`

- [ ] F2. **Code Quality Review** — `unspecified-high`
  Run `cmake --build build` (zero warnings). Run `ctest --test-dir build` (all pass). Review all changed files for: raw pointers, manual memory management, C-style casts, `using namespace` in headers, commented-out code. Check C++20 feature usage: concepts used, spans used, constexpr used.
  Output: `Build [PASS/FAIL] | Tests [N pass/N fail] | Files [N clean/N issues] | VERDICT`

- [ ] F3. **Real Manual QA** — `unspecified-high`
  Start from clean build. Execute: (1) Generate X.509 types from ASN.1 → compile → DER encode known certificate → DER decode → compare. (2) Generate PER types → encode → decode → compare. (3) Cross-validate BER output against `asn1c` tool. (4) Cross-validate PER output against known test vectors.
  Output: `Scenarios [N/N pass] | Cross-validation [N/N] | VERDICT`

- [ ] F4. **Scope Fidelity Check** — `deep`
  For each task: read "What to do", read actual diff (git log/diff). Verify 1:1 — everything in spec was built, nothing beyond spec. Check "Must NOT do" compliance. Detect cross-task contamination. Flag unaccounted changes.
  Output: `Tasks [N/N compliant] | Contamination [CLEAN/N issues] | Unaccounted [CLEAN/N files] | VERDICT`

---

## Commit Strategy

Commits at wave boundaries (NOT per-task to avoid noise):
- **Wave 1**: `feat: project scaffold, build system, test infra, core primitives`
- **Wave 2**: `feat: ASN.1 AST, lexer, parser core, codec interfaces`
- **Wave 3**: `feat: core type generators (INTEGER, STRING, BOOLEAN, ENUMERATED, SEQUENCE)`
- **Wave 4**: `feat: BER/DER codec with tag/length layer`
- **Wave 5**: `feat: CHOICE, SEQUENCE OF, OID generators + PER prep`
- **Wave 6**: `feat: PER aligned + unaligned codec`
- **Wave 7**: `feat: OER, XER, JER text codecs`
- **Wave 8**: `feat: CLI tool, CMake module, e2e tests, docs`
- **Wave 9**: `feat: advanced ASN.1 constructs (tagged, constrained, IOC, parameterized)`

---

## Success Criteria

### Verification Commands
```bash
cmake -B build -DCMAKE_CXX_STANDARD=20 -DCMAKE_BUILD_TYPE=Debug && cmake --build build
# Expected: zero warnings, zero errors

ctest --test-dir build --output-on-failure
# Expected: all tests pass

./build/bin/asn1pp-gen --input test/data/x509.asn --output gen/x509/
# Expected: generates .hpp/.cpp files, exit code 0

g++ -std=c++20 -Ilibs -Igen/x509 gen/x509/*.cpp test_roundtrip.cpp -o test_roundtrip
./test_roundtrip
# Expected: DER encode → decode → assert equality, exit code 0
```

### Final Checklist
- [ ] All "Must Have" present
- [ ] All "Must NOT Have" absent
- [ ] All TDD tests pass (green)
- [ ] DER round-trip with X.509 succeeds
- [ ] PER round-trip with 3GPP RRC fragment succeeds
- [ ] Zero compiler warnings at `-Wall -Wextra -Wpedantic`
- [ ] `asn1pp_generate()` CMake function works end-to-end
