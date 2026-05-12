---
title: OER API Reference
---

# OER (Octet Encoding Rules)

OER is a compact, octet-aligned encoding per ITU-T X.696. Unlike BER, it omits
TLV tags. Unlike PER, it does not bit-pack. The result is a middle ground:
simple and predictable, yet reasonably compact.

## Overview

OER encodes each value as whole octets. Variable-length types use a length
determinant prefix, but there are no tag bytes and no bit-level alignment.
This makes OER easier to parse than BER and avoids the alignment overhead of
PER.

### When to use OER

- Constrained INTEGER types where you want predictable byte widths
- Embedded systems with simple hardware that cannot handle bit-level parsing
- Situations where BER overhead is unacceptable but PER complexity is excessive
- Types that are not yet supported by PER (OER often implements first)

### Key characteristics

| Property | Value |
|----------|-------|
| Alignment | Octet-aligned (always whole bytes) |
| Tag format | None (no TLV) |
| Length format | Determinant prefix per X.696 §4 |
| Bit packing | None |
| Constraint handling | Compile-time via `OerMeta` template parameter |

### Comparison with BER and PER

| Feature | BER | PER (aligned) | OER |
|---------|-----|---------------|-----|
| Tags | TLV | None | None |
| Bit packing | No | Yes | No |
| Length determinant | Yes | No (preamble) | Yes |
| Constrained INTEGER | Variable | Bit-packed | Fixed-width |
| Requires `flush_decode()` | No | Yes (aligned) | No |
| Simplicity | Medium | High | Medium |

## Meta Struct

The `OerMeta` template parameter carries compile-time constraint information.
It is emitted by the code generator, but can also be written by hand for
testing or special cases.

```cpp
// INTEGER meta
struct Int0To255 {
    static constexpr bool has_range_constraint = true;
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 255;
};

// OCTET STRING meta
struct FixedOctets {
    static constexpr bool has_size_constraint = true;
    static constexpr size_t min_size = 4;
    static constexpr size_t max_size = 4;
};

// BIT STRING meta
struct UnconstrainedBits {
    static constexpr bool has_size_constraint = false;
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = SIZE_MAX;
};
```

### INTEGER encoding width

The number of bytes used for a constrained INTEGER depends on the range:

| Range | Byte width |
|-------|------------|
| 0–255 | 1 |
| 0–65535 | 2 |
| 0–4294967295 | 4 |
| larger | 8 |

Unconstrained INTEGER values are prefixed with a length determinant followed by
the minimum number of two's complement bytes needed to represent the value.

## oer_encoder Class

Header: `codec/oer/encoder.hpp`

### Primitive Encoders

```cpp
// Encode a constrained or unconstrained INTEGER per X.696 §7.2.
template<typename OerMeta>
result<void> encode_integer(int64_t value, buffer_view& buf);

// Encode BOOLEAN per X.696 §7.3 — always 1 octet: 0xFF (true) or 0x00 (false).
result<void> encode_boolean(bool value, buffer_view& buf);

// Encode NULL per X.696 §7.4 — no bytes are written.
result<void> encode_null(buffer_view& buf);

// Encode OCTET STRING per X.696 §16.
// Fixed size (min_size==max_size): data bytes only, no length prefix.
// Constrained size: length determinant + data.
// Unconstrained: length determinant + data.
template<typename OerMeta>
result<void> encode_octet_string(std::span<const uint8_t> data, buffer_view& buf);

// Encode BIT STRING per X.696 §14.
// Fixed size: data octets only.
// Unconstrained: length determinant + unused_bits octet + data.
template<typename OerMeta>
result<void> encode_bit_string(std::span<const uint8_t> data,
                                uint8_t unused_bits,
                                buffer_view& buf);

// Encode ENUMERATED per X.696 §7.7.
// Width: ≤255 → 1 octet, ≤65535 → 2 octets, else → 4 octets.
template<typename OerMeta>
result<void> encode_enumerated(int64_t index, buffer_view& buf);
```

### Constructed Type Helpers

```cpp
// Start a SEQUENCE.
// Writes extension bit (if extensible) + optional-field presence bitmap,
// packed into full octets (MSB first). No preamble if no extensions and no
// optional fields.
template<typename OerMeta>
result<void> encode_sequence_start(buffer_view& buf,
                                    const bool* optional_present = nullptr,
                                    size_t optional_count = 0);

// End a SEQUENCE — no-op for OER (no alignment padding needed).
result<void> encode_sequence_end(buffer_view& buf);

// Encode CHOICE index per X.696 §19.
// Width: ≤255 → 1 octet, ≤65535 → 2 octets, else → 4 octets.
template<typename OerMeta>
result<void> encode_choice_index(int64_t choice_index, buffer_view& buf);

// Encode SEQUENCE OF / SET OF length per X.696 §20.
template<typename OerMeta>
result<void> encode_sequence_of_length(size_t length, buffer_view& buf);

// Encode pre-encoded OBJECT IDENTIFIER per X.696 §7.6.
// Accepts BER-style subidentifier bytes; encodes as length determinant + content.
result<void> encode_oid(std::span<const uint8_t> encoded_oid, buffer_view& buf);

// Encode a length determinant per X.696 §4:
//   0–127:         single octet `0xxxxxxx`
//   128–16383:     two octets `10xxxxxx xxxxxxxx`
//   16384+:        `11000000 nnnnnnnn nnnnnnnn` (16-bit count)
result<void> encode_length_determinant(size_t length, buffer_view& buf);
```

### Batch Operations

```cpp
// Drain any pending batch encode operations via SIMD.
// Idempotent — calling when no pending ops is a no-op.
error_code flush_encode() noexcept;
```

## oer_decoder Class

Header: `codec/oer/decoder.hpp`

### Primitive Decoders

```cpp
// Decode INTEGER per X.696 §7.2.
template<typename OerMeta>
result<int64_t> decode_integer(buffer_view& buf);

// Decode BOOLEAN per X.696 §7.3 — reads 1 octet.
result<bool> decode_boolean(buffer_view& buf);

// Decode NULL per X.696 §7.4 — reads nothing.
result<void> decode_null(buffer_view& buf);

// Decode OCTET STRING per X.696 §16.
// Returns the raw content octets.
template<typename OerMeta>
result<std::vector<uint8_t>> decode_octet_string(buffer_view& buf);

// Decode BIT STRING per X.696 §14.
// Returns (data, unused_bits).
template<typename OerMeta>
result<std::pair<std::vector<uint8_t>, uint8_t>> decode_bit_string(buffer_view& buf);

// Decode ENUMERATED per X.696 §7.7.
template<typename OerMeta>
result<int64_t> decode_enumerated(buffer_view& buf);
```

### Constructed Type Helpers

```cpp
// Decode CHOICE index per X.696 §19.
template<typename OerMeta>
result<int64_t> decode_choice_index(buffer_view& buf);

// Decode SEQUENCE OF / SET OF length per X.696 §20.
template<typename OerMeta>
result<size_t> decode_sequence_of_length(buffer_view& buf);

// Decode length determinant per X.696 §4.
result<size_t> decode_length_determinant(buffer_view& buf);
```

### Batch Operations

```cpp
// Drain any pending batch decode operations via SIMD.
// Idempotent. Returns error_code::ok on success, or the first error encountered.
error_code flush_decode() noexcept;

// Number of consecutive decode_integer() calls before flushing.
// 4 for AVX2, 2 for SSE4.2/NEON, 1 for scalar.
static size_t batch_size() noexcept;
```

**Note:** Unlike PER's aligned variant, OER does not require `flush_decode()`
to be called by the application. The decoder handles buffering internally.
Call it only if you want to drain pending SIMD operations early.

## Code Example

Encode the integer 42 using a constraint that permits 0–255, then decode it back.

```cpp
#include "codec/oer/encoder.hpp"
#include "codec/oer/decoder.hpp"
#include "buffer/buffer_view.hpp"

using namespace asn1pp;
using namespace asn1pp::oer;

// Meta for integers in range [0, 255]
struct Int0To255 {
    static constexpr bool has_range_constraint = true;
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 255;
};

// Encode
uint8_t out[16]{};
buffer_view obv(out, sizeof(out));
oer_encoder enc;
auto rv = enc.encode_integer<Int0To255>(42, obv);
(void)rv;

// At this point:
//   out[0] = 0x01  (length determinant = 1 octet)
//   out[1] = 0x2A  (42 in hexadecimal)

// Decode
uint8_t oer_bytes[] = {0x01, 0x2A};
buffer_view bv(oer_bytes, sizeof(oer_bytes));
oer_decoder dec;
auto rv2 = dec.decode_integer<Int0To255>(bv);
// rv2 holds value 42
```

## Limitations

OER in this implementation currently supports:

- INTEGER (constrained and unconstrained)
- BOOLEAN
- NULL
- OCTET STRING (fixed, constrained, unconstrained)
- BIT STRING (fixed, unconstrained)
- ENUMERATED
- SEQUENCE (with optional fields and extensions)
- CHOICE
- SEQUENCE OF / SET OF
- OBJECT IDENTIFIER (pre-encoded input)

Not yet implemented:

- REAL (floating-point)
- UTF8String and other character types
- Nested SEQUENCEs with mixed optionality (handled case-by-case)

Check the [changelog](https://github.com/aschokinatgmail/asn1pp/blob/main/CHANGELOG.md) for updates.
