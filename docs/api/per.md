---
title: PER/UPER API Reference
---

# PER/UPER API Reference

PER (Packed Encoding Rules) and UPER (Unaligned PER) are defined in ITU-T X.691. They provide compact binary encodings optimized for bandwidth-constrained channels. PER is significantly more compact than BER because it eliminates type tags and uses constraint information to pack values tightly.

## Aligned vs Unaligned

The library provides two PER variants:

**APER (Aligned PER)** — after each field, output is aligned to the next byte boundary before the next field is encoded. This makes APER easier to debug and process with byte-oriented hardware, but less compact.

**UPER (Unaligned PER)** — bits are packed continuously with no inter-field padding. This produces smaller encodings, especially for records with many small fields, but requires bit-level processing.

Both APER and UPER use the same constraint mechanism and encoding algorithms. The choice affects only the bit-packing behavior.

## The Meta Template Pattern

All constrained types in PER use a template parameter called `PerMeta` (or just `Meta`). This is a compile-time struct containing constraint information. The code generator emits these structs for each ASN.1 type.

The canonical example is `Int0To255`:

```cpp
struct Int0To255 {
    static constexpr bool has_range_constraint = true;
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 255;
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 255;
    static constexpr bool extensible = false;
    static constexpr bool has_size_constraint = true;
};
```

### Meta Struct Requirements

For INTEGER encoding, the Meta struct must provide:

| Member | Type | Description |
|--------|------|-------------|
| `has_range_constraint` | `bool` | True if the type has a range constraint |
| `min_value` | `int64_t` | Minimum value (inclusive) |
| `max_value` | `int64_t` | Maximum value (inclusive) |
| `has_size_constraint` | `bool` | True if the type has a size constraint |
| `min_size` | `size_t` | Minimum size in octets |
| `max_size` | `size_t` | Maximum size in octets |
| `extensible` | `bool` | True if the type uses extension markers |

The `has_size_constraint` and size members are relevant for OCTET STRING, BIT STRING, and SEQUENCE OF types. For INTEGER, focus on the range constraint members.

The value is encoded as `value - min_value` in the minimum number of bits needed to represent the range `max_value - min_value`.

## APER: per_aligned_encoder and per_aligned_decoder

APER packs values to byte boundaries after each field. This simplifies hardware processing and debugging at the cost of additional padding bits.

### per_aligned_encoder

```cpp
#include "codec/per/encoder.hpp"

asn1pp::per::per_aligned_encoder enc;
uint8_t out[16]{};
asn1pp::buffer_view buf(out, sizeof(out));

enc.encode_integer<Int0To255>(42, buf);
```

Signature:

```cpp
template<typename PerMeta>
asn1pp::result<void> encode_integer(int64_t value, buffer_view& buf);
```

The encoder tracks bit position internally. After encoding, call `buf.size()` to see how many bytes were written.

### per_aligned_decoder

```cpp
asn1pp::per::per_aligned_decoder dec;
uint8_t per_bytes[] = {0x1A};  // 42 in APER format
asn1pp::buffer_view bv(per_bytes, sizeof(per_bytes));

auto r = dec.decode_integer<Int0To255>(bv);
if (r.is_ok()) {
    int64_t val = r.value();
}
// IMPORTANT: must call flush_decode() after APER decode operations
dec.flush_decode();
```

Signature:

```cpp
template<typename PerMeta>
asn1pp::result<int64_t> decode_integer(const buffer_view& buf);
```

## UPER: uper_encoder and uper_decoder

UPER packs bits continuously without byte alignment between fields. This produces smaller encodings for complex records.

### uper_encoder

```cpp
#include "codec/per/uper_encoder.hpp"

asn1pp::per::uper_encoder enc;
uint8_t out[16]{};
asn1pp::buffer_view buf(out, sizeof(out));

enc.encode_integer<Int0To255>(42, buf);
```

Signature (same as APER encoder):

```cpp
template<typename PerMeta>
asn1pp::result<void> encode_integer(int64_t value, buffer_view& buf);
```

### uper_decoder

```cpp
asn1pp::per::uper_decoder dec;
uint8_t uper_bytes[] = {0x54};  // 42 in UPER format
asn1pp::buffer_view bv(uper_bytes, sizeof(uper_bytes));

auto r = dec.decode_integer<Int0To255>(bv);
if (r.is_ok()) {
    int64_t val = r.value();
}
dec.flush_decode();
```

Signature (same as APER decoder):

```cpp
template<typename PerMeta>
asn1pp::result<int64_t> decode_integer(const buffer_view& buf);
```

## flush_decode()

The `flush_decode()` function is required after APER decode operations. Both `per_aligned_decoder` and `uper_decoder` use SIMD-accelerated batch decoding for INTEGER types. Multiple decode calls are accumulated and processed together for efficiency.

```cpp
error_code flush_decode() noexcept;
```

**When to call it:**

- After all decode_integer calls for a complete PDU
- Before decoding the next value that might have different constraints
- At end of decoding session

**Why it matters:** Without calling `flush_decode()`, any pending integer operations that haven't been flushed will not produce their results. The function is idempotent, so calling it multiple times is safe.

**Example showing the pattern:**

```cpp
asn1pp::per::per_aligned_decoder dec;
asn1pp::buffer_view bv(encoded_data, encoded_len);

auto val1 = dec.decode_integer<Int0To255>(bv);
auto val2 = dec.decode_integer<Int0To255>(bv);
auto val3 = dec.decode_integer<OtherMeta>(bv);
// Process val1, val2 first
// val3 uses different Meta, so pending integers must be flushed
dec.flush_decode();
// Now val3 is available
auto v3 = val3.value();
```

## APER vs UPER Encoded Size Comparison

The same value encodes differently in APER vs UPER. APER aligns after each field, while UPER packs bits continuously.

```cpp
#include <cstdint>
#include <cstdio>
#include "codec/per/encoder.hpp"
#include "codec/per/uper_encoder.hpp"
#include "codec/per/decoder.hpp"
#include "codec/per/uper_decoder.hpp"
#include "buffer/buffer_view.hpp"

struct Int0To255 {
    static constexpr bool has_range_constraint = true;
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 255;
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 255;
    static constexpr bool extensible = false;
    static constexpr bool has_size_constraint = true;
};

int main() {
    uint8_t aper_out[16]{};
    uint8_t uper_out[16]{};

    asn1pp::buffer_view aper_buf(aper_out, sizeof(aper_out));
    asn1pp::buffer_view uper_buf(uper_out, sizeof(uper_out));

    // Encode the same value
    asn1pp::per::per_aligned_encoder aper_enc;
    asn1pp::per::uper_encoder uper_enc;

    aper_enc.encode_integer<Int0To255>(42, aper_buf);
    uper_enc.encode_integer<Int0To255>(42, uper_buf);

    // APER: 0x1A = 1 byte (aligned after field)
    // UPER: 0x54 = 1 byte (bit-packed)
    // For larger values, the difference becomes more apparent:
    // APER aligns after each field, wasting padding bits
    // UPER packs bits continuously, more compact

    printf("APER size: %zu bytes\n", aper_buf.size());
    printf("UPER size: %zu bytes\n", uper_buf.size());

    return 0;
}
```

## Complete Encode/Decode Example

```cpp
#include <cstdint>
#include <cstring>
#include "codec/per/encoder.hpp"
#include "codec/per/decoder.hpp"
#include "codec/per/uper_encoder.hpp"
#include "codec/per/uper_decoder.hpp"
#include "buffer/buffer_view.hpp"
#include "codec/result.hpp"

struct Int0To255 {
    static constexpr bool has_range_constraint = true;
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 255;
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 255;
    static constexpr bool extensible = false;
    static constexpr bool has_size_constraint = true;
};

int main() {
    // Encode with APER
    uint8_t aper_buf[16]{};
    asn1pp::buffer_view aper_out(aper_buf, sizeof(aper_buf));
    asn1pp::per::per_aligned_encoder aper_enc;
    auto enc_result = aper_enc.encode_integer<Int0To255>(42, aper_out);
    if (enc_result.is_err()) {
        return 1;
    }

    // Decode with APER
    asn1pp::buffer_view aper_in(aper_buf, aper_out.size());
    asn1pp::per::per_aligned_decoder aper_dec;
    auto dec_result = aper_dec.decode_integer<Int0To255>(aper_in);
    if (dec_result.is_err()) {
        return 1;
    }
    aper_dec.flush_decode();
    int64_t aper_value = dec_result.value();

    // Encode with UPER
    uint8_t uper_buf[16]{};
    asn1pp::buffer_view uper_out(uper_buf, sizeof(uper_buf));
    asn1pp::per::uper_encoder uper_enc;
    enc_result = uper_enc.encode_integer<Int0To255>(42, uper_out);
    if (enc_result.is_err()) {
        return 1;
    }

    // Decode with UPER
    asn1pp::buffer_view uper_in(uper_buf, uper_out.size());
    asn1pp::per::uper_decoder uper_dec;
    dec_result = uper_dec.decode_integer<Int0To255>(uper_in);
    if (dec_result.is_err()) {
        return 1;
    }
    uper_dec.flush_decode();
    int64_t uper_value = dec_result.value();

    // Both should give the same result
    return (aper_value == 42 && uper_value == 42) ? 0 : 1;
}
```

## Return Type: result<T>

All encode and decode operations return `asn1pp::result<T>`:

```cpp
template<typename T>
class result {
public:
    bool is_ok() const noexcept;
    bool is_err() const noexcept;
    T value();           // throws if error
    T value_or(T fallback);
    error_code error() const;
};
```

For decode operations, `result<T>` holds the decoded value or an error code. Check `is_ok()` before reading the value.

## Error Handling

```cpp
auto r = dec.decode_integer<Int0To255>(bv);
if (r.is_err()) {
    asn1pp::error_code ec = r.error();
    // handle error: value_out_of_range, buffer_overflow, etc.
    return;
}
int64_t val = r.value();
```

Common error codes:
- `error_code::value_out_of_range` — value exceeds min/max bounds
- `error_code::buffer_overflow` — output buffer too small
- `error_code::invalid_encoding` — malformed input data

## Summary

| Class | Purpose | Key Method |
|-------|---------|------------|
| `per_aligned_encoder` | APER encode | `encode_integer<Meta>(val, buf)` |
| `per_aligned_decoder` | APER decode | `decode_integer<Meta>(buf)` |
| `uper_encoder` | UPER encode | `encode_integer<Meta>(val, buf)` |
| `uper_decoder` | UPER decode | `decode_integer<Meta>(buf)` |
| `flush_decode()` | Drain pending decodes | Both decoders |

The Meta template parameter carries constraint information at compile time, enabling efficient bit-level packing without runtime overhead.
