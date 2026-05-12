# BER Encode/Decode Tutorial

This example demonstrates basic BER (Basic Encoding Rules) encoding and decoding using the asn1pp codec library.

## What This Demonstrates

1. **INTEGER encoding/decoding** — Encoding integer value 42, then decoding it back and verifying the result matches.

2. **OCTET STRING encoding/decoding** — Encoding a byte sequence `{0x01, 0x02, 0x03}`, then decoding and verifying.

3. **SEQUENCE encoding/decoding** — Building a SEQUENCE containing three integers `{10, 20, 30}`:
   - First calculates total content length
   - Writes SEQUENCE header (tag + length)
   - Writes each integer into the content area
   - Decodes by first reading the TLV header, then extracting and decoding each integer

## Running

```bash
cmake -B build -GNinja -DBER_DEMO=ON
ninja -C build ber-demo
./build/examples/ber/ber-demo
```

## Expected Output

```
=== BER Encode/Decode Tutorial ===

INTEGER 42 encoded (3 bytes): 02 01 2a
[PASS] INTEGER 42: round-trip OK

OCTET STRING {01 02 03} encoded (5 bytes): 04 03 01 02 03
[PASS] OCTET STRING {01 02 03}: round-trip OK

SEQUENCE {10, 20, 30} encoded (11 bytes): 10 09 02 01 0a 02 01 14 02 01 1e
[INFO] SEQUENCE: tag=16 length=9
[PASS] SEQUENCE {10, 20, 30}: round-trip OK

=== Results ===
All 3 round-trips passed.
```

## Key Patterns

- **Result error handling**: Check `r.is_err()` after each encode/decode operation
- **Buffer reuse**: Each operation uses a fresh `buffer_view` created from the buffer
- **Two-pass SEQUENCE encoding**: First pass calculates content length, second pass writes the actual encoding
- **TLV decoding**: SEQUENCE content is decoded by first reading the TLV structure, then extracting value bytes