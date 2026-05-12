# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **KConfig build configuration system**: Kconfig file with 17 options across 6 menus (embedded profile, buffer sizing, SIMD, encoding rules, build options) and cmake/kconfig.cmake integration
- **Versioning**: version.hpp.in template with MAJOR.MINOR.PATCH, git SHA, dirty flag, at_least()/is_exact() API (8 tests)
- **ARM cross-compilation support**: STM32F407 (Cortex-M4F) toolchain file, linker script with ENTRY(_start) and _end/end symbols
- **Docker builder/measurer containers**: docker/docker-compose.yml with builder and measurer images for CI/CD
- **Bare-metal demo**: examples/bare-metal/ standalone demo exercising BER+PER+UPER+OER codecs on embedded hardware
- **Measured embedded footprint**: STM32F407, MinSizeRel, BER+DER+PER+UPER+OER: 24,328 bytes Flash (23.8 KB), 420 bytes RAM

### Changed

- **traits.hpp**: std::string/std::string_view guards now respect ASN1PP_NO_TEXT_CODECS
- **Conditional codec compilation**: XER/JER source files excluded when EMBEDDED or NO_TEXT_CODECS is enabled
- **EMBEDDED KConfig**: auto-selects NO_TEXT_CODECS via select directive

### Fixed

- **Linker script**: added missing ENTRY(_start) and _end/end symbols for bare-metal targets

## [0.1.0] - 2026-05-11

### Added

- **Codec families**: BER, DER, PER, UPER, OER, XER, JER encoding/decoding
- **SIMD backends**: SSE4.2, AVX2, NEON with portable abstraction layer (libs/arch/)
- **Hybrid dispatch**: compile-time + runtime backend selection
- **Embedded profile** (ASN1PP_EMBEDDED): zero heap allocation, fixed PDU buffers, configurable text codec stripping (ASN1PP_NO_TEXT_CODECS), bounded ASN1PP_MAX_PDU
- **Code generator CLI**: asn1pp code generation tool
- **Test suite**: 1020+ tests with zero regressions

### Performance

- BER decode: 6.63M ops/s (11x threshold)
- BER encode: 3.99M ops/s (5x threshold)

[Unreleased]: https://github.com/aschokinatgmail/asn1pp/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/aschokinatgmail/asn1pp/releases/tag/v0.1.0
