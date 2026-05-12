---
title: BER API Reference
---

# BER API Reference

BER (Basic Encoding Rules) is defined in ITU-T X.690 and is the most flexible ASN.1 encoding format. It uses a self-describing TLV (Tag-Length-Value) structure where each value is identified by its tag, allowing encoding and decoding without prior knowledge of the schema.

## TLV Structure

Every BER element follows this three-part structure:

- **Tag** — identifies the type of the value (e.g., 0x02 for INTEGER, 0x04 for OCTET STRING)
- **Length** — number of bytes in the Value field (definite or indefinite form)
- **Value** — the actual data bytes

Because BER is self-describing, the same INTEGER value can be encoded in multiple valid ways (e.g., leading zero bytes are allowed but not required). DER (Distinguished Encoding Rules) is a restricted subset of BER that imposes canonical forms for deterministic signatures.

## ber_encoder class

Header: `codec/ber/encoder.hpp`

Namespace: `asn1pp::ber`

The encoder writes into a caller-provided `buffer_view`. No heap allocation occurs in hot paths; integer encoding uses a stack-allocated buffer of at most 9 bytes.

### encode_integer

```cpp
result<void> encode_integer(int64_t value, buffer_view& buf);
```

Encodes INTEGER (universal tag 0x02) in two's complement big-endian form. The encoder uses SIMD-accelerated batch encoding transparently when multiple consecutive `encode_integer` calls are made. Call `flush_encode()` to drain pending operations.

**Parameters**
- `value` — the signed 64-bit integer to encode
- `buf` — mutable output buffer; advances past bytes written on success

**Returns** `result<void>` — `error_code::ok` on success, `error_code::buffer_overflow` if the buffer is too small.

### encode_octet_string

```cpp
result<void> encode_octet_string(std::span<const uint8_t> data, buffer_view& buf);
```

Encodes OCTET STRING (universal tag 0x04). Raw bytes are written without transformation. Uses SIMD-accelerated copy for payloads larger than 32 bytes.

### encode_boolean

```cpp
result<void> encode_boolean(bool value, buffer_view& buf);
```

Encodes BOOLEAN (universal tag 0x01). True becomes 0xFF, false becomes 0x00.

### encode_tlv

```cpp
result<void> encode_tlv(const tag& t, std::span<const uint8_t> value, buffer_view& buf);
```

Writes a complete TLV (tag + length + value). Delegates to the TLV helper module.

### flush_encode

```cpp
error_code flush_encode() noexcept;
```

Drains any pending batch encode operations via SIMD. Idempotent; calling when no pending operations is a no-op. Returns `error_code::ok` on success or the first error encountered.

---

## ber_decoder class

Header: `codec/ber/decoder.hpp`

Namespace: `asn1pp::ber`

The decoder reads from a caller-provided `buffer_view` and advances it past the bytes consumed. SIMD batch decoding is transparent; call `flush_decode()` to explicitly drain pending operations.

### decode_integer

```cpp
result<int64_t> decode_integer(buffer_view& buf);
```

Decodes INTEGER (universal tag 0x02) from two's complement big-endian wire format. When SIMD is available, value decoding may be batched internally.

**Parameters**
- `buf` — input buffer; advances past bytes consumed on success

**Returns** `result<int64_t>` — decoded value on success. Returns `error_code::buffer_underflow` on truncated input, `error_code::invalid_tag` on wrong tag, `error_code::value_out_of_range` if the value exceeds int64_t range.

### decode_octet_string

```cpp
result<std::vector<uint8_t>> decode_octet_string(buffer_view& buf);  // non-embedded
result<octet_string_result> decode_octet_string(buffer_view& buf);  // embedded
```

Decodes OCTET STRING (universal tag 0x04) and returns the raw content bytes. In embedded builds, returns data inline via `octet_string_result { data[N], length }`.

### decode_sequence_header

```cpp
result<tag> decode_sequence_header(buffer_view& buf, size_t& content_length);
```

Decodes a constructed SEQUENCE or SET header (tag + definite length). Returns the tag and sets `content_length` to the number of content bytes that follow. The caller then decodes `content_length` bytes of inner elements.

### decode_tlv

```cpp
result<tlv_data> decode_tlv(buffer_view& buf);
```

Decodes a full TLV (tag + length + value). The returned value span points into the caller's buffer and is valid only until the buffer is destroyed.

### flush_decode

```cpp
error_code flush_decode() noexcept;
```

Drains any pending batch decode operations via SIMD. Idempotent. Returns `error_code::ok` or the first error encountered.

---

## der_encoder class

Header: `codec/ber/der_encoder.hpp`

Namespace: `asn1pp::ber`

DER (Distinguished Encoding Rules) is a canonical subset of BER. It removes BER's encoding flexibility by enforcing canonical forms: no leading zero bytes, minimal length encoding, definite length only. Use DER when you need deterministic output, such as digital signatures.

### validate_integer_der

```cpp
result<void> validate_integer_der(std::span<const uint8_t> content) const;
```

Checks that the INTEGER content obeys DER canonical rules: no leading zero bytes (unless the high bit is set), minimal number of octets. Returns `error_code::constraint_violation` if canonical form is violated.

### validate_boolean_der

```cpp
result<void> validate_boolean_der(std::span<const uint8_t> content) const;
```

Validates BOOLEAN content for DER: must be exactly one byte, either 0x00 or 0xFF.

### is_integer_minimal

```cpp
static bool is_integer_minimal(std::span<const uint8_t> content);
```

Returns true if the INTEGER content uses minimal encoding (no leading zero octets that could have been omitted). This is the core canonical check used by DER validators.

### encode_* methods

The DER encoder exposes the same `encode_integer`, `encode_boolean`, `encode_octet_string`, and related methods as `ber_encoder`, but it enforces canonical encoding rules internally. The `flush_encode()` method delegates to the internal `ber_encoder`.

---

## Error Handling

All encoder and decoder functions return `result<T>` or `result<void>`. The `result<T>` type is a custom sum type similar to `std::expected`:

```cpp
result<int64_t> r = decoder.decode_integer(buf);
if (r.is_ok()) {
    int64_t value = r.value();
} else {
    error_code e = r.error();
}
```

Common error codes:

| error_code | Meaning |
|---|---|
| `ok` | Success |
| `buffer_overflow` | Output buffer too small |
| `buffer_underflow` | Input truncated |
| `invalid_tag` | Unexpected tag byte |
| `invalid_length` | Malformed length field |
| `constraint_violation` | DER canonical rule violated |
| `value_out_of_range` | Integer too large for int64_t |
| `encoding_error` | Generic encoding failure |

---

## Complete Round-Trip Example

The following example encodes an INTEGER, decodes it back, and verifies the result:

```cpp
#include <vector>
#include <cstdint>
#include "codec/ber/encoder.hpp"
#include "codec/ber/decoder.hpp"

using namespace asn1pp::ber;

int main() {
    // Encode
    std::vector<uint8_t> buf(16);
    buffer_view out(buf);
    ber_encoder encoder;
    auto r = encoder.encode_integer(42, out);
    if (r.is_err()) { return 1; }

    // Decode
    buffer_view in(buf.data(), static_cast<size_t>(out.data() - buf.data()));
    ber_decoder decoder;
    auto dr = decoder.decode_integer(in);
    if (dr.is_err()) { return 2; }

    // Verify
    if (dr.value() != 42) { return 3; }
    return 0;
}
```

---

## Buffer View

`buffer_view` (defined in `buffer/buffer_view.hpp`) is the buffer abstraction used throughout the codec library. It is a non-owning span-like wrapper:

```cpp
buffer_view() noexcept;
buffer_view(const uint8_t* data, size_t size) noexcept;
explicit buffer_view(std::span<const uint8_t> sp) noexcept;
explicit buffer_view(std::span<uint8_t> sp) noexcept;
explicit buffer_view(std::vector<uint8_t>& vec) noexcept;

size_t size() const noexcept;
bool empty() const noexcept;
const uint8_t* data() const noexcept;
buffer_view subview(size_t offset, size_t length) const noexcept;
```

Both encoders and decoders advance the `buffer_view` past the bytes they process, making it easy to chain multiple operations on the same buffer.
