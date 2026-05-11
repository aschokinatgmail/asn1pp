# Decisions — hw-simd-backend

## Architecture
- libs/arch/ directory: interface library target asn1pp-arch
- simd.hpp: unified dispatch header (single include for codec code)
- detect.hpp: CPU feature detection (cpuid/getauxval)
- Backend files: scalar.hpp, x86_sse42.hpp, x86_avx2.hpp, arm_neon.hpp
- arch_codec.hpp: ASN.1-specific SIMD codec primitives
- batch_buffer.hpp: transparent batching ring buffer (template class)
- config.hpp: central compile-time configuration (ASN1PP_EMBEDDED, ASN1PP_MAX_PDU)

## Dispatch Pattern
- Compile-time: inline constexpr simd_level compile_level based on #ifdef
- Runtime: available_simd_level() returns best available level
- Function pointers: cached via std::call_once after first resolution
- No per-call cpuid

## Embedded Profile
- config.hpp: single central configuration header
- Compile-time gating only — no runtime config
- Heap-allocating decode returns replaced with fixed-buffer variants in embedded mode
- Text codecs (XER/JER) strippable via ASN1PP_NO_TEXT_CODECS

## Commit Strategy
- Wave 1: feat(arch): portable SIMD abstraction layer
- Wave 2: feat(codec): SIMD-accelerated decode
- Wave 3: feat(codec): SIMD-accelerated encode + transparent batching