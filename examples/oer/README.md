# OER Encode/Decode Example

This example demonstrates **Octet Encoding Rules (OER)** encoding and decoding of a constrained integer (0–255).

## What is OER?

OER (ITU-T X.696) is a binary encoding ruleset that:
- Uses **fixed-width encoding** for constrained types — no tag, no length prefix
- Is more compact than BER/DER for constrained integers
- Is simpler than PER (no bit-level packing needed)

## Running the Example

```bash
# From project root:
cmake -B build -GNinja
ninja -C build oer-demo
./build/examples/oer/oer-demo
```

## Expected Output

```
Encoded 2 bytes: 01 2A
PASS: encoded bytes match expected [01 2A]
Decoded value: 42
PASS: round-trip decode returned 42

Size comparison:
  BER encoding of 42: 3 bytes (02 01 2A) — tag + length + value
  OER encoding of 42: 2 bytes (01 2A)   — just the value
  Savings: 33% fewer bytes with OER when constraints are known
```

## Key Concepts

### Int0To255 Meta struct

```cpp
struct Int0To255 {
    static constexpr bool has_range_constraint = true;
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 255;
    // ...
};
```

The `has_range_constraint=true` flag tells the OER encoder to use fixed-width encoding based on the range size (≤256 → 1 octet).

### BER vs OER

| Encoding | Bytes | Format |
|----------|-------|--------|
| BER | `02 01 2A` | Tag + Length + Value |
| OER (constrained) | `01 2A` | Value only |

### No flush_decode() needed

Unlike APER (Aligned Packed Encoding Rules), OER does not require a `flush_decode()` call after decoding. The OER decoder is stateless per-call.