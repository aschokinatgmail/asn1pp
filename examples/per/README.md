# PER Example — Aligned vs Unaligned Encoding

This example demonstrates the difference between **APER** (Aligned PER) and **UPER** (Unaligned PER) encoding using the asn1pp codec library.

## APER vs UPER

Both are PER (Packed Encoding Rules) per ITU-T X.691, but differ in octet alignment:

| Feature | APER (Aligned) | UPER (Unaligned) |
|---------|----------------|------------------|
| Alignment | Fields align to octet boundaries after each field | No inter-field alignment — bits packed continuously |
| Padding | Often wastes 0-7 bits per field | Only padding at final PDU boundary |
| Use case | Protocols requiring byte-level access | Efficient wire encoding |

## Running the Example

```bash
cmake -B build -GNinja -DPER_EXAMPLE=ON
ninja -C build
./build/examples/per/per-example
```

## Expected Output

For the value `42` with constraint `0..255`:

- **APER** aligns after the INTEGER header, typically resulting in more bytes
- **UPER** packs bits contiguously, producing fewer (or equal) bytes

```
APER encoded (N bytes): 1A ...
UPER encoded (M bytes): 54 ...

APER round-trip: 42 -> 42
UPER round-trip: 42 -> 42

=== Size Comparison ===
APER size: N bytes
UPER size: M bytes
UPER is X byte(s) smaller — no wasted alignment bits

All tests passed.
```

## Key Observations

1. UPER is always ≤ APER in encoded size (no wasted alignment bits)
2. Both round-trips produce identical decoded values
3. The difference is more pronounced with multi-field structures
