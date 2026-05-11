# HW-Accelerated SIMD Backend Architecture

## TL;DR

> **Quick Summary**: Add a portable SIMD abstraction layer (`libs/arch/`) with platform-specific backends (SSE4.2, AVX2, NEON) and hybrid compile-time+runtime dispatch. Accelerate the codec layer's encode and decode hot paths using SIMD primitives for integer parsing, buffer copying, and tag comparison. Add compile-time embedded profile with static-allocation-only, user-configurable codec stripping, and fixed PDU buffers. Transparent PDU batching to maximize SIMD lane utilization.
>
> **Deliverables**:
> - `libs/arch/` — Portable SIMD abstraction: `simd.hpp` (dispatch), `x86_sse42.hpp`, `x86_avx2.hpp`, `arm_neon.hpp`, `scalar.hpp` (fallback)
> - `libs/codec/config.hpp` — Central compile-time configuration: `ASN1PP_EMBEDDED`, `ASN1PP_NO_TEXT_CODECS`, `ASN1PP_MAX_PDU`
> - `libs/codec/arch_codec.hpp` — SIMD-accelerated codec primitives (batch integer decode/encode, tag compare, batch copy)
> - Accelerated decoders: `ber_decoder`, `per_decoder`, `oer_decoder`, `xer_decoder`, `jer_decoder`
> - Accelerated encoders: mirror for all 5 codec families
> - `test/arch/` — SIMD layer + codec acceleration tests
> - `test/embedded/` — Embedded profile tests (static allocation, fixed buffers, no-heap enforcement)
>
> **Estimated Effort**: XL (18 implementation + 4 review tasks)
> **Parallel Execution**: YES — 5 waves
> **Critical Path**: T1 → T2 → T5 → T8 → T9 → T13 → T16 → F1-F4

---

## Context

### Original Request
Extend the asn1pp codec with HW-specific backends using CPU streaming extensions (SSE4.2, AVX2, NEON) to perform encoding/decoding at very high speed. Multiple backends depending on the HW. Transparent internal batching.

### Interview Summary

**Key Decisions**:
- **Dispatch**: Hybrid compile-time (#ifdef) + runtime (cpuid/getauxval) — matches simdjson pattern
- **SIMD layer**: Custom portable abstraction (no Google Highway dependency) — `libs/arch/`
- **Scope**: Encode + decode, all 5 codec families (BER, PER, OER, XER, JER)
- **Batching**: Transparent internal ring buffer — single-PDU public API, SIMD batch processing hidden
- **Min target**: SSE4.2 / AVX2 / NEON (no bare-metal scalar-only tier needed)
- **Order**: Decode hot paths first, encode second
- **Embedded profile**: `ASN1PP_EMBEDDED` compile-time define — zero heap, fixed buffers, configurable codec stripping
- **Text codecs in embedded**: User-configurable via `ASN1PP_NO_TEXT_CODECS` (XER/JER stripped if defined)
- **Embedded PDU buffers**: Compile-time `ASN1PP_MAX_PDU` define (default 2048 bytes)

**Research Findings**:
- No prior art for SIMD ASN.1 BER decoding — greenfield optimization
- simdjson: runtime dispatch via implementation selection (haswell/westmere/fallback)
- ASN.1 integers are small (1-8 bytes) — SIMD per-PDU doesn't help; batch N PDUs is required
- Tag+length parsing is serial (1-3 bytes) — SIMD benefit is in parallel tag comparison across PDUs
- OCTET STRING payload copies (memcpy) benefit from SIMD at >32 bytes
- simdparse: AVX-512 integer parsing ~2.3 GB/s vs 0.8 GB/s scalar — batch integer parsing is the primary SIMD win

---

## Work Objectives

### Core Objective
Add a portable SIMD backend layer under the existing codec dispatch, transparently accelerating encode/decode for all encoding rules with zero public API changes.

### Concrete Deliverables
- `libs/arch/simd.hpp` — Unified dispatch header
- `libs/arch/detect.hpp` — CPU feature detection
- `libs/arch/x86_sse42.hpp` — SSE4.2 backend
- `libs/arch/x86_avx2.hpp` — AVX2 backend
- `libs/arch/arm_neon.hpp` — ARM NEON backend
- `libs/arch/scalar.hpp` — Scalar fallback
- `libs/codec/arch_codec.hpp` — SIMD codec acceleration primitives
- Modified decoders: `ber/ber_decoder.cpp`, `per/per_decoder.cpp`, `oer/oer_decoder.cpp`, `xer/xer_decoder.cpp`, `jer/jer_decoder.cpp`
- Modified encoders: mirror for 5 codec families
- `test/arch/simd_test.cpp` — SIMD layer tests
- `test/arch/arch_codec_test.cpp` — Codec acceleration tests

### Definition of Done
- [ ] Build: `cmake --build build` succeeds with zero warnings
- [ ] All existing 738+ tests pass
- [ ] New arch/ tests pass
- [ ] BER decode_integer throughput: >600k ops/s (vs 410k asn1c, 308k baseline)
- [ ] BER encode_integer throughput: >800k ops/s (vs 614k baseline)
- [ ] No public API changes to codec classes

### Must Have
- Portable SIMD abstraction across x86/ARM
- Hybrid dispatch: compile-time + runtime
- Custom implementation (no third-party SIMD library)
- Transparent batching infrastructure
- All codecs accelerated (not just BER)

### Must NOT Have (Guardrails)
- No Google Highway or other SIMD library dependency
- No public API breakage
- No heap allocation in hot decode/encode paths
- No per-PDU vtable dispatch
- No #ifdef scattered across codec files (all platform logic in arch/)
- No premature transparent batching before SIMD layer is proven

---

## Verification Strategy

> **ZERO HUMAN INTERVENTION** — ALL verification is agent-executed.

### Test Decision
- **Infrastructure exists**: YES (GTest + GMock)
- **Automated tests**: YES (tests-after implementation)
- **Framework**: Google Test + Google Mock

### QA Policy
Every task MUST include agent-executed QA scenarios.

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Foundation — 2 sub-waves, MAX PARALLEL):
  [1a] Task 1: arch/ directory + CMake [quick] (unblocks 2-6, 15)

  [1b - ALL 6 in PARALLEL after Task 1]:
  ├── Task 2:  CPU feature detection [deep]
  ├── Task 3:  Scalar fallback backend [quick]
  ├── Task 4:  SSE4.2 backend [deep]
  ├── Task 5:  AVX2 backend [deep]
  ├── Task 6:  ARM NEON backend [deep]
  └── Task 15: Embedded config.hpp + ASN1PP_EMBEDDED [quick]

Wave 2 (Bridges — sequential, narrow):
  ├── Task 7:  Unified dispatch + SIMD tests (depends: 2-6) [deep]
  └── Task 8:  SIMD codec primitives arch_codec.hpp (depends: 7) [deep]

Wave 3 (Codec integration — 5 tasks, MAX PARALLEL, all depend: 8):
  ├── Task 9:  BER decoder SIMD batch [deep]
  ├── Task 10: PER decoder SIMD batch [deep]
  ├── Task 11: OER / XER / JER decoder SIMD batch [quick]
  ├── Task 12: All 5 encoder families SIMD batch [deep]
  └── Task 13: Ring buffer batch_buffer.hpp [deep]

Wave 4 (Guard + tests — 2 sub-waves):
  [4a] Task 16: Embedded #ifdef guards in codec (depends: 9-12, 15) [quick]

  [4b - 2 in PARALLEL]:
  ├── Task 14: Codec SIMD integration tests (depends: 9-13) [deep]
  └── Task 17: Embedded profile tests (depends: 16) [deep]

Wave FINAL (After ALL — 4 parallel reviews):
  ├── Task F1: Plan compliance audit (oracle)
  ├── Task F2: Code quality review (unspecified-high)
  ├── Task F3: Real manual QA — benchmark (unspecified-high)
  └── Task F4: Scope fidelity check (deep)
  -> Present results -> Get explicit user okay

Critical Path: T1 → T5 → T7 → T8 → T9 → T16 → T14 → F1-F4
Parallel Speedup: ~42% faster than sequential (18 tasks across 5 waves)
Max Concurrent: 6 (Wave 1b) | 5 (Wave 3)
```

---

## TODOs

- [x] 1. **arch/ directory + CMake integration**

  **What to do**:
  - Create `libs/arch/` directory
  - Create `libs/arch/CMakeLists.txt` as interface library target `asn1pp-arch`
  - Add `arch/arch.cpp` with platform detection singleton
  - Wire `asn1pp-arch` as dependency of `asn1pp-codec`
  - Add compiler flags: `-msse4.2`, `-mavx2` for x86 backends, `-mfpu=neon` for ARM
  - Add `test/arch/` directory

  **Must NOT do**:
  - No SIMD code yet — infrastructure only
  - No modification to existing codec files

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Directory structure + CMake — straightforward scaffolding
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (Wave 1, with Tasks 2-7)
  - **Blocks**: All other arch tasks
  - **Blocked By**: None

  **QA Scenarios**:
  ```
  Scenario: arch target compiles
    Tool: Bash
    Steps:
      1. cmake -B build -DCMAKE_CXX_COMPILER=/usr/bin/c++ -DCMAKE_CXX_STANDARD=20
      2. cmake --build build --target asn1pp-arch
    Expected Result: Target builds successfully, libasn1pp-arch.a created
    Evidence: .sisyphus/evidence/task-hw1-arch-build.txt
  ```

  **Commit**: NO (groups with Wave 1)

- [x] 2. **CPU feature detection (detect.hpp)**

  **What to do**:
  - Implement `arch/detect.hpp` with `arch::cpu_features` struct
  - x86: use `__builtin_cpu_supports("sse4.2")` / `__builtin_cpu_supports("avx2")`
  - ARM: use `getauxval(AT_HWCAP)` and check `HWCAP_NEON` / `HWCAP_ASIMD`
  - Implement `arch::cpu_features arch::detect_cpu()` that runs cpuid/getauxval once
  - Implement `arch::simd_level` enum: { scalar, sse42, avx2, neon }
  - Implement `arch::available_simd_level()` returning current level
  - Thread-safe lazy initialization (std::call_once)

  **Must NOT do**:
  - No platform-specific code in headers used outside arch/
  - No global constructors

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Architecture-specific intrinsics + thread-safety requires careful implementation
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (Wave 1)
  - **Blocks**: Task 7 (unified dispatch)
  - **Blocked By**: Task 1

  **References**:
  - `libs/arch/detect.hpp` (to be created) — All detection logic lives here
  - gcc docs: `__builtin_cpu_supports` for feature detection

  **QA Scenarios**:
  ```
  Scenario: detect_cpu runs and returns valid level
    Tool: Bash
    Steps:
      1. Build test binary that calls arch::available_simd_level()
      2. On x86 Mac: assert level >= sse42
      3. On Apple Silicon: assert level == neon
    Expected Result: Detection returns correct platform level
    Evidence: .sisyphus/evidence/task-hw2-detect.txt
  ```

  **Commit**: NO

- [x] 3. **Scalar fallback backend (scalar.hpp)**

  **What to do**:
  - Implement `arch/scalar.hpp` — no-SIMD fallback for platforms without extensions
  - Define namespace `asn1pp::arch::scalar` with inline functions:
    - `load_u64_be(const uint8_t* p)` — load 1-8 bytes big-endian → uint64_t
    - `store_u64_be(uint8_t* p, uint64_t v, size_t n)` — store uint64_t as n bytes big-endian
    - `batch_load_u64_be(const uint8_t** ptrs, size_t* sizes, uint64_t* out, size_t count)` — scalar loop over N PDUs
    - `batch_store_u64_be(uint8_t** ptrs, uint64_t* values, size_t* sizes, size_t count)` — scalar loop over N PDUs
    - `copy_bytes(uint8_t* dst, const uint8_t* src, size_t n)` — basic memcpy

  **Must NOT do**:
  - No SIMD — this is the scalar reference implementation
  - No platform-specific code

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Simple scalar implementations, well-defined API
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (Wave 1)
  - **Blocks**: Task 7 (dispatch references scalar as fallback)
  - **Blocked By**: Task 1

  **QA Scenarios**:
  ```
  Scenario: load_u64_be correct for 1-8 byte inputs
    Tool: Bash (compile + run test)
    Steps:
      1. Feed {0x7F} → assert 127
      2. Feed {0xFF, 0xFF} → assert 65535
      3. Feed {0x00, 0x00, 0x00, 0x01} → assert 1
    Expected Result: All conversions match two's complement semantics
    Evidence: .sisyphus/evidence/task-hw3-scalar.txt
  ```

  **Commit**: NO

- [x] 4. **SSE4.2 backend (x86_sse42.hpp)**

  **What to do**:
  - Implement `arch/x86_sse42.hpp` using SSE4.2 intrinsics (`<smmintrin.h>`)
  - Implement same API as scalar.hpp but using 128-bit SIMD:
    - `batch_load_u64_be`: load 2 integers at once via `_mm_loadl_epi64` + byte swap
    - `batch_store_u64_be`: store 2 integers via byte swap + `_mm_storel_epi64`
    - Use `_mm_shuffle_epi8` (PSHUFB) for byte reversal
    - `copy_bytes`: use `_mm_loadu_si128` / `_mm_storeu_si128` for 16-byte chunks
    - `compare_tags`: compare 2 tag bytes at once
  - Compile with `-msse4.2`

  **Must NOT do**:
  - No AVX intrinsics (that's the avx2 backend)
  - No runtime dispatch in this file (dispatch is in simd.hpp)

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: SIMD intrinsics require careful byte-layout reasoning
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (Wave 1)
  - **Blocks**: Task 7
  - **Blocked By**: Task 1

  **QA Scenarios**:
  ```
  Scenario: batch_load matches scalar reference
    Tool: Bash
    Steps:
      1. Generate 1000 random 2-8 byte integers
      2. Decode with scalar::batch_load_u64_be
      3. Decode with sse42::batch_load_u64_be
      4. Assert all values identical
    Expected Result: SSE output matches scalar bit-exact
    Evidence: .sisyphus/evidence/task-hw4-sse42.txt
  ```

  **Commit**: NO

- [x] 5. **AVX2 backend (x86_avx2.hpp)**

  **What to do**:
  - Implement `arch/x86_avx2.hpp` using AVX2 intrinsics (`<immintrin.h>`)
  - Same API, 256-bit SIMD:
    - `batch_load_u64_be`: load 4 integers at once via `_mm256_loadu_si256` + byte reversal
    - Use `_mm256_shuffle_epi8` (VPSHUFB) for 32-byte byte reversal
    - `copy_bytes`: 32-byte chunks via `_mm256_loadu_si256` / `_mm256_storeu_si256`
    - `compare_4_tags`: compare 4 tag bytes simultaneously
  - Compile with `-mavx2`

  **Must NOT do**:
  - No AVX-512 (we don't target it yet)
  - No SSE code (delegate to x86_sse42.hpp for <256-bit ops)

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: AVX2 byte-level manipulation is non-trivial
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (Wave 1)
  - **Blocks**: Task 7
  - **Blocked By**: Task 1

  **QA Scenarios**:
  ```
  Scenario: batch_load 4 integers matches scalar reference
    Tool: Bash
    Steps:
      1. Generate 4000 random integers
      2. Decode with scalar::batch_load_u64_be (4 at a time)
      3. Decode with avx2::batch_load_u64_be (4 at a time)
      4. Assert bit-exact match
    Expected Result: AVX2 matches scalar for all 1000 batches
    Evidence: .sisyphus/evidence/task-hw5-avx2.txt
  ```

  **Commit**: NO

- [x] 6. **ARM NEON backend (arm_neon.hpp)**

  **What to do**:
  - Implement `arch/arm_neon.hpp` using ARM NEON intrinsics (`<arm_neon.h>`)
  - Same API, 128-bit SIMD:
    - `batch_load_u64_be`: load 2 integers via `vld1_u8` + `vrev64_u8` + extract
    - Use `vrev` family for byte reversal
    - `copy_bytes`: `vld1q_u8` / `vst1q_u8` for 16-byte chunks
  - Compile with `-mfpu=neon` (or auto-detected on aarch64)

  **Must NOT do**:
  - No x86 intrinsics
  - No ARM SVE (Scalable Vector Extension) — NEON only

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: ARM NEON intrinsics differ significantly from x86
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (Wave 1)
  - **Blocks**: Task 7
  - **Blocked By**: Task 1

  **QA Scenarios**:
  ```
  Scenario: batch_load matches scalar reference
    Tool: Bash
    Steps:
      1. Compile on ARM machine (or QEMU)
      2. Verify batch_load_u64_be matches scalar output
    Expected Result: NEON output matches scalar bit-exact
    Evidence: .sisyphus/evidence/task-hw6-neon.txt
  ```

  **Commit**: NO

- [x] 7. **Unified dispatch + SIMD tests (simd.hpp, simd_test.cpp)**

  **What to do**:
  - Implement `arch/simd.hpp` — single include for all codec code
  - Dispatch logic:
    ```cpp
    namespace asn1pp::arch {
        // Compile-time base level
        #ifdef __AVX2__
        inline constexpr simd_level compile_level = simd_level::avx2;
        #elif defined(__SSE4_2__)
        inline constexpr simd_level compile_level = simd_level::sse42;
        #elif defined(__ARM_NEON)
        inline constexpr simd_level compile_level = simd_level::neon;
        #else
        inline constexpr simd_level compile_level = simd_level::scalar;
        #endif

        // Runtime-overridable dispatch functions
        void batch_load_u64_be(const uint8_t* const* ptrs, size_t count,
                               const size_t* sizes, uint64_t* out);
        // ... etc for all primitives
    }
    ```
  - Runtime dispatch: `batch_load_u64_be` checks `available_simd_level()` and calls the right backend
  - Function pointers cached after first call (std::call_once)
  - Implement `test/arch/simd_test.cpp`: test each backend on its respective platform
  - Add to `test/CMakeLists.txt`

  **Must NOT do**:
  - No per-call cpuid — cache after first check
  - No #ifdef in simd.hpp except for compile_level constant

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Dispatch logic must be correct and fast — this is the hardest integration point
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on Tasks 3-6)
  - **Blocks**: Task 8 (codec primitives need dispatch)
  - **Blocked By**: Tasks 2-6

  **QA Scenarios**:
  ```
  Scenario: dispatch calls correct backend
    Tool: Bash
    Steps:
      1. Build + run simd_test on x86 Mac
      2. Assert simd level == sse42 (or avx2 if available)
      3. Call batch_load_u64_be with 2 integers
      4. Verify result matches scalar reference
    Expected Result: Correct backend selected, correct output produced
    Evidence: .sisyphus/evidence/task-hw7-dispatch.txt
  ```

  **Commit**: NO

- [x] 8. **SIMD codec primitives (arch_codec.hpp)**

  **What to do**:
  - Implement `libs/codec/arch_codec.hpp` — codec-layer acceleration
  - Higher-level SIMD operations for ASN.1:
    - `batch_decode_integers(buffer_view* bufs, size_t count, int64_t* out, error_code* errors)` — decode N integer PDUs
    - `batch_encode_integers(const int64_t* values, size_t count, buffer_view* bufs, error_code* errors)`
    - `batch_copy_octet_strings(const uint8_t* src, size_t* lengths, uint8_t** dsts, size_t count)`
    - `batch_compare_tags(const uint8_t* tag_bytes, uint8_t expected, size_t count, bool* results)`
  - These wrap the `arch::` primitives with ASN.1-specific logic
  - Handle edge cases: different integer sizes per PDU in a batch, buffer overflow per PDU
  - All operations are `noexcept`

  **Must NOT do**:
  - No codec class modifications yet (that's Tasks 9-12)
  - No heap allocation

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Bridges SIMD layer to ASN.1 semantics — non-trivial integration
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on Task 7)
  - **Blocks**: Tasks 9-12
  - **Blocked By**: Task 7

  **References**:
  - `libs/arch/simd.hpp` — Underlying SIMD primitives
  - `libs/codec/ber/decoder.hpp` — Existing decode_integer signature
  - `libs/codec/result.hpp` — error_code enum

  **QA Scenarios**:
  ```
  Scenario: batch_decode 4 integers produces correct results
    Tool: Bash
    Steps:
      1. Pre-encode 4 DER integers: {7F, 0100, FFFFFF, 00000001}
      2. Call batch_decode_integers on 4 buffers
      3. Assert: [127, 256, 16777215, 1]
      4. Assert: all error_codes == ok
    Expected Result: Batch decode matches individual decode
    Evidence: .sisyphus/evidence/task-hw8-codec-prims.txt
  ```

  **Commit**: NO

- [x] 9. **Integrate SIMD into BER decoder (ber_decoder.cpp)**

  **What to do**:
  - Modify `ber_decoder::decode_integer()` to use internal ring buffer
  - Accumulate up to N pending decode_integer calls, then batch-process:
    - N = 4 for AVX2, 2 for SSE4.2/NEON, 1 for scalar
    - Ring buffer holds pointer+size per pending PDU
    - On Nth call (or flush): call `arch_codec::batch_decode_integers`
    - Distribute results back to queued result<> returns
  - Add explicit `flush_decode()` method for draining pending batch before reading results
  - Same pattern for `decode_boolean`, `decode_enumerated`
  - For OCTET STRING: use `batch_copy_octet_strings` via SIMD memcpy
  - Add internal `pending_batch_` state to ber_decoder (stack-allocated, small)

  **Must NOT do**:
  - No change to public method signatures
  - No heap allocation in the ring buffer
  - No breaking existing tests

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Ring buffer + deferred execution — requires careful state management
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (Wave 2, with Task 10)
  - **Blocks**: Task 14 (integration tests)
  - **Blocked By**: Task 8

  **References**:
  - `libs/codec/ber/ber_decoder.cpp:34-65` — current decode_integer
  - `libs/codec/arch_codec.hpp` — batch primitives

  **QA Scenarios**:
  ```
  Scenario: decode 4 integers triggers batch
    Tool: Bash
    Steps:
      1. Create ber_decoder
      2. Encode 4 BER integers
      3. Call decode_integer 4 times sequentially
      4. Assert all 4 values correct
      5. Call decode_integer 5th time — should auto-flush previous batch
    Expected Result: Values correct, no assertion failures
    Evidence: .sisyphus/evidence/task-hw9-ber-batch.txt

  Scenario: existing 738 tests still pass
    Tool: Bash
    Steps:
      1. cmake --build build && ctest --test-dir build --exclude-regex NOT_BUILT
      2. Assert 100% pass
    Expected Result: Zero regressions
    Evidence: .sisyphus/evidence/task-hw9-ber-regression.txt
  ```

  **Commit**: NO

- [x] 10. **Integrate SIMD into PER decoder (per_decoder.cpp)**

  **What to do**:
  - Apply same ring-buffer batch pattern to PER aligned decoder
  - PER specifics: integers may be constrained (small range) — different batch strategy
  - For unconstrained PER integers: same batch_decode path as BER
  - For constrained PER integers: SIMD-accelerated range check via batch_compare
  - For PER length determinants: batch decode via same integer path
  - PER bit-level operations: SIMD bit manipulation for aligned bit writes (mask + shift in parallel)

  **Must NOT do**:
  - No change to PER encoder (that's Task 12)
  - No breaking PER test suites

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: PER has constraint logic — careful integration needed
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (Wave 2, with Task 9, 11)
  - **Blocks**: Task 14
  - **Blocked By**: Task 8

  **QA Scenarios**:
  ```
  Scenario: PER decode batch produces correct values
    Tool: Bash
    Steps:
      1. Run existing per_decoder_test (1170 tests)
      2. Assert all pass
    Expected Result: Zero regressions
    Evidence: .sisyphus/evidence/task-hw10-per-regression.txt
  ```

  **Commit**: NO

- [x] 11. **Integrate SIMD into OER/XER/JER decoders**

  **What to do**:
  - OER decoder: batch integer decode (fixed-width: 1/2/4/8 byte integers)
  - XER decoder: batch tag comparison in XML element matching
  - JER decoder: batch integer parsing from JSON number tokens
  - All three use same `arch_codec::batch_*` primitives
  - Less benefit than binary codecs (text parsing dominates), but architecture is consistent

  **Must NOT do**:
  - No encoder changes
  - No text parser rewrite — just use SIMD where integers are involved

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Pattern is established from Tasks 9-10, just replicate
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (Wave 2, with Tasks 9-10)
  - **Blocks**: Task 14
  - **Blocked By**: Task 8

  **QA Scenarios**:
  ```
  Scenario: OER/XER/JER tests still pass
    Tool: Bash
    Steps:
      1. Run oer_test (1088 tests), xer_test (563 tests), jer_test (604 tests)
      2. Assert all pass
    Expected Result: Zero regressions across all text codecs
    Evidence: .sisyphus/evidence/task-hw11-text-regression.txt
  ```

  **Commit**: NO

- [x] 12. **Integrate SIMD into all 5 encoder families**

  **What to do**:
  - Mirror the batch pattern for encode paths:
    - BER encoder: batch encode integers (values → DER wire format)
    - PER encoder: batch encode integers with constraint metadata
    - OER encoder: batch encode to fixed-width
    - XER/JER: batch integer-to-text conversion (itoa)
  - Use `arch_codec::batch_encode_integers` across all
  - Ring buffer same pattern as decode path
  - Add `flush_encode()` to encoder classes

  **Must NOT do**:
  - No public API change
  - No breaking encoder tests

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: 5 encoder families to modify — wide impact
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on Task 8-11 establishing decoder pattern)
  - **Blocks**: Task 14
  - **Blocked By**: Tasks 8-11

  **QA Scenarios**:
  ```
  Scenario: Encode round-trip matches
    Tool: Bash
    Steps:
      1. Run all encoder tests + decoder tests
      2. Run der_roundtrip_test (X.509)
      3. Run rrc_per_test (PER round-trip)
      4. Assert 100% pass
    Expected Result: Encode + decode still round-trip correctly
    Evidence: .sisyphus/evidence/task-hw12-encoder-regression.txt
  ```

  **Commit**: NO

- [x] 13. **Transparent batching ring buffer infrastructure**

  **What to do**:
  - Implement `libs/codec/batch_buffer.hpp` — reusable ring buffer for transparent batching
  - Template class `batch_buffer<T, N>` where T = pending operation, N = SIMD width
  - Operations: `push(T)`, `is_full()`, `drain()`, `flush()`
  - Non-allocating: fixed-size array on the stack
  - Use in all encoder/decoder classes instead of ad-hoc arrays
  - Thread-local if needed (single-threaded by default)

  **Must NOT do**:
  - No dynamic allocation
  - No thread-synchronization overhead (single-threaded design)

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Template design with SIMD-width compile-time constant
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (Wave 3, with Task 12)
  - **Blocks**: Task 14
  - **Blocked By**: Task 8 (needs arch_codec::batch pattern)

  **QA Scenarios**:
  ```
  Scenario: ring buffer drains correctly at capacity
    Tool: Bash
    Steps:
      1. Create batch_buffer<int, 4>
      2. Push 4 values: [10, 20, 30, 40]
      3. assert is_full() == true
      4. drain() returns vector of 4, buffer now empty
    Expected Result: FIFO semantics, capacity respected
    Evidence: .sisyphus/evidence/task-hw13-ring-buffer.txt
  ```

  **Commit**: NO

- [x] 14. **Codec acceleration integration tests (arch_codec_test.cpp)**

  **What to do**:
  - Implement `test/arch/arch_codec_test.cpp` — comprehensive integration tests
  - Tests cover:
    - Batch decode 1/2/4 BER integers, verify values
    - Batch decode with mixed sizes (1-byte, 4-byte, 8-byte integers in same batch)
    - Batch decode overflow: value > 8 bytes → error_code per PDU
    - Batch encode → batch decode round-trip
    - BER + PER + OER batch encode/decode for integers
    - Ring buffer auto-flush: push N items, auto-drain, verify
  - Add to `test/CMakeLists.txt`

  **Must NOT do**:
  - No test that depends on specific HW (use scalar fallback for cross-platform)

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Comprehensive test suite — validates entire SIMD pipeline
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (must run after all implementation tasks)
  - **Blocks**: F1-F4
  - **Blocked By**: Tasks 9-13

  **QA Scenarios**:
  ```
  Scenario: full integration test suite passes
    Tool: Bash
    Steps:
      1. cmake --build build --target arch_codec_test
      2. ./build/test/arch_codec_test
      3. Assert all tests pass
    Expected Result: All integration tests pass
    Evidence: .sisyphus/evidence/task-hw14-integration.txt
  ```

  **Commit**: YES (Wave 3 group)

- [x] 15. **Embedded config.hpp + ASN1PP_EMBEDDED**

  **What to do**:
  - Create `libs/codec/config.hpp` — single central compile-time configuration header
  - Define `ASN1PP_EMBEDDED` guard:
    ```cpp
    #ifdef ASN1PP_EMBEDDED
    inline constexpr bool is_embedded = true;
    #define ASN1PP_NO_HEAP  // enforce no dynamic allocation
    static_assert(ASN1PP_MAX_PDU > 0, "ASN1PP_MAX_PDU must be defined in embedded mode");
    #else
    inline constexpr bool is_embedded = false;
    #endif
    ```
  - `ASN1PP_NO_TEXT_CODECS`: user-configurable define to strip XER/CXER/E-XER/JER
  - `ASN1PP_MAX_PDU`: default 2048 in embedded, unlimited otherwise
  - Add CMake option: `option(ASN1PP_EMBEDDED "Embedded profile" OFF)`
  - Add CMake option: `option(ASN1PP_NO_TEXT_CODECS "Strip text codecs" OFF)`
  - When `-DASN1PP_EMBEDDED=ON`: set `-DASN1PP_EMBEDDED` and `-DASN1PP_MAX_PDU=2048`
  - When `-DASN1PP_NO_TEXT_CODECS=ON`: set `-DASN1PP_NO_TEXT_CODECS`
  - Add `#include "codec/config.hpp"` to all codec headers that need embedded awareness

  **Must NOT do**:
  - No runtime config — compile-time only
  - No per-file #ifdef scattered across codebase (centralize in config.hpp)

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Configuration header + CMake options — straightforward
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (Wave 1b, with Tasks 2-6)
  - **Blocks**: Task 16 (embedded #ifdef guards)
  - **Blocked By**: Task 1

  **QA Scenarios**:
  ```
  Scenario: embedded config compiles
    Tool: Bash
    Steps:
      1. cmake -B build_emb -DASN1PP_EMBEDDED=ON -DASN1PP_NO_TEXT_CODECS=ON
      2. Build — verify config.hpp macros set correctly
      3. Check: is_embedded == true
    Expected Result: Embedded profile active, builds clean
    Evidence: .sisyphus/evidence/task-hw15-embedded-config.txt
  ```

  **Commit**: NO (groups with Wave 1)

- [x] 16. **Embedded #ifdef guards in codec files**

  **What to do**:
  - Wrap all heap-using code with `#ifndef ASN1PP_EMBEDDED` / `#endif`
  - Files to guard:
    - `libs/codec/ber/ber_decoder.cpp` — `decode_octet_string` (returns `std::vector`), replace with fixed-buffer decode in embedded
    - `libs/codec/ber/ber_encoder.cpp` — any dynamic buffer growth
    - `libs/codec/per/per_decoder.cpp` — same pattern
    - `libs/codec/oer/oer_decoder.cpp` — same
    - `libs/codec/xer/xer_decoder.cpp` — `#ifdef ASN1PP_NO_TEXT_CODECS` strips entire file
    - `libs/codec/xer/xer_encoder.cpp` — strip if text codecs off
    - `libs/codec/jer/jer_decoder.cpp` — strip if text codecs off
    - `libs/codec/jer/jer_encoder.cpp` — strip if text codecs off
    - `src/gen/` — code generator is a host tool, never embedded — no change
  - Replace `std::vector` decode returns with fixed-buffer alternatives:
    - `decode_octet_string_embedded(buffer_view& buf, uint8_t* out, size_t max_len, size_t& actual_len)`
    - Same for BIT STRING, OID
  - Add static_assert in decode paths: `"ASN1PP_EMBEDDED requires ASN1PP_MAX_PDU"`
  - Ensure all SIMD batch code paths work in embedded mode (they use stack allocs, already fine)

  **Must NOT do**:
  - No #ifdef in public headers (config.hpp controls everything)
  - No runtime fallback — embedded mode is compile-time gated
  - No removal of error checking

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Wrapping existing code with #ifdefs — mechanical, but must be thorough across all files
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (must run after SIMD codec integration to wrap the combined code)
  - **Blocks**: Task 17 (embedded tests)
  - **Blocked By**: Tasks 9-12, 15

  **References**:
  - `libs/codec/config.hpp` — embedded define source
  - `libs/codec/ber/ber_decoder.cpp` — decode_octet_string (heap allocation)
  - `libs/codec/per/uper_decoder.hpp` — PER string decode
  - `libs/codec/result.hpp` — error_code enum

  **QA Scenarios**:
  ```
  Scenario: embedded build strips text codecs
    Tool: Bash
    Steps:
      1. cmake -B build_emb -DASN1PP_EMBEDDED=ON -DASN1PP_NO_TEXT_CODECS=ON
      2. cmake --build build_emb --target asn1pp-codec
      3. nm build_emb/libs/libasn1pp-codec.a | grep -c "xer\|jer" 
      4. Assert: 0 xer/jer symbols
    Expected Result: Text codecs stripped at link level
    Evidence: .sisyphus/evidence/task-hw16-stripped.txt
  ```

  **Commit**: NO

- [x] 17. **Embedded profile tests**

  **What to do**:
  - Create `test/embedded/` directory
  - Implement `test/embedded/embedded_test.cpp`:
    - Test: `ASN1PP_EMBEDDED` is defined, `is_embedded == true`
    - Test: `ASN1PP_MAX_PDU == 2048` (default)
    - Test: decode_integer on fixed buffer works
    - Test: decode_octet_string_embedded fills fixed buffer
    - Test: buffer overflow → returns buffer_overflow error
    - Test: SIMD batch decode works in embedded mode (if arch supports it)
    - Test: no std::vector or std::string used in decode path (compile-time check: static_assert on `is_embedded`)
  - Add CMake target:
    ```cmake
    if(ASN1PP_EMBEDDED)
      asn1pp_add_test(embedded_test embedded/embedded_test.cpp)
    endif()
    ```
  - Cross-platform: embedded tests run on host with `-DASN1PP_EMBEDDED=ON` (not actual bare-metal)

  **Must NOT do**:
  - No actual embedded hardware required — all tests run on host
  - No heap allocation in test code when `ASN1PP_EMBEDDED` is set

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Embedded-specific validation requires careful buffer management
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (Wave 4b, with Task 14)
  - **Blocks**: F1-F4
  - **Blocked By**: Task 16

  **References**:
  - `libs/codec/config.hpp` — embedded defines
  - `test/e2e/x509_der_test.cpp` — test pattern reference
  - `test/CMakeLists.txt` — test registration

  **QA Scenarios**:
  ```
  Scenario: fixed-buffer decode rejects oversized PDU
    Tool: Bash
    Steps:
      1. Build in embedded mode
      2. Create PDU larger than ASN1PP_MAX_PDU (2048)
      3. Attempt decode → assert buffer_overflow error
    Expected Result: Error correctly propagated, no buffer overflow
    Evidence: .sisyphus/evidence/task-hw17-embedded-test.txt
  ```

  **Commit**: YES (Wave 4 group)

---

## Final Verification Wave

- [x] F1. **Plan Compliance Audit** — `oracle`
  Read the plan end-to-end. Verify all Must Have present, all Must NOT Have absent.
  Output: `Must Have [N/N] | Must NOT Have [N/N] | Tasks [N/N] | VERDICT: APPROVE/REJECT`

- [x] F2. **Code Quality Review** — `unspecified-high`
  Build with zero warnings. All tests pass. Review SIMD code for correctness, portability, no UB.
  Output: `Build [PASS/FAIL] | Tests [N pass/N fail] | VERDICT: APPROVE/REJECT`

- [x] F3. **Real Manual QA** — `unspecified-high`
  Benchmark: BER decode_integer >600k ops/s, encode >800k ops/s. Run full test suite.
  Output: `Benchmarks [PASS/FAIL] | Tests [N/N] | VERDICT: APPROVE/REJECT`

- [x] F4. **Scope Fidelity Check** — `deep`
  Verify all tasks compliant, no cross-contamination, no unaccounted files.
  Output: `Tasks [N/N compliant] | Contamination [CLEAN/N] | VERDICT: APPROVE/REJECT`

---

## Commit Strategy

| Wave | Message |
|------|---------|
| 1 | `feat(arch): portable SIMD abstraction layer (SSE4.2/AVX2/NEON/scalar)` |
| 2 | `feat(codec): SIMD-accelerated decode (BER/PER/OER/XER/JER)` |
| 3 | `feat(codec): SIMD-accelerated encode + transparent batching` |

---

## Success Criteria

### Verification Commands
```bash
cmake -B build -DCMAKE_CXX_COMPILER=/usr/bin/c++ -DCMAKE_CXX_STANDARD=20
cmake --build build -j$(sysctl -n hw.ncpu)     # zero warnings
ctest --test-dir build --exclude-regex NOT_BUILT  # 100% pass
```

### Final Checklist
- [x] All Must Have present
- [x] All Must NOT Have absent
- [x] All existing 738+ tests pass (zero regressions)
- [x] New arch/ tests pass
- [x] BER decode >600k ops/s
- [x] BER encode >800k ops/s
- [x] Zero compiler warnings
- [x] No public API breakage
