---
title: Home
---

# asn1pp

**asn1pp** is a C++20 header-only library for encoding and decoding ASN.1 formats with a focus on embedded systems and high-performance applications.

## Features

- **Multiple Encoding Rules**: BER, DER, PER (aligned and unaligned), UPER, OER
- **SIMD Optimized**: Leverage AVX2/AVX-512 for batch processing
- **Embedded Friendly**: No dynamic allocation, works on bare-metal systems
- **Zero Dependencies**: Header-only, portable C++17
- **KConfig Support**: Compile-time feature selection

## Supported Encoding Rules

| Format | Description |
|--------|-------------|
| **BER** | Basic Encoding Rules - Most flexible, supports indefinite length |
| **DER** | Distinguished Encoding Rules - Canonical, used for certificates |
| **PER** | Packed Encoding Rules - Compact, efficient for protocol buffers |
| **UPER** | Unaligned PER - Bit-efficient, widely used in telecommunications |
| **OER** | Open Encoding Rules - Human-readable lengths, fast parsing |

## Quick Start

Get started with asn1pp in minutes:

[Get Started :material-arrow-right:](getting-started.md){ .md-button .md-button--primary }

## Architecture

```text
┌─────────────────────────────────────────────────────────┐
│                     asn1pp Library                      │
├─────────────────────────────────────────────────────────┤
│  Schema Definition  │  Runtime API  │  SIMD Backend     │
├─────────────────────────────────────────────────────────┤
│              Encoding / Decoding Core                  │
├─────────────────────────────────────────────────────────┤
│     BER      │     DER      │     PER      │    OER    │
└─────────────────────────────────────────────────────────┘
```

## Use Cases

- **Telecommunications**: LTE/5G protocol stacks
- **IoT Devices**: Constrained embedded systems
- **Security**: X.509 certificates, CMS signed data
- **Industrial**: SCADA protocols, smart grid

## API Reference

- [BER Encoding](api/ber.md) - Basic Encoding Rules
- [PER Encoding](api/per.md) - Packed Encoding Rules
- [OER Encoding](api/oer.md) - Open Encoding Rules

## License

asn1pp is released under the [MIT License](https://github.com/aschokinatgmail/asn1pp/blob/main/LICENSE).

## Source Code

- [GitHub Repository](https://github.com/aschokinatgmail/asn1pp)
- [Star us on GitHub](https://github.com/aschokinatgmail/asn1pp) if you find this useful
- [Open an Issue](https://github.com/aschokinatgmail/asn1pp/issues) for bugs or feature requests
