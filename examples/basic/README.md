# BER Basic Example

A minimal BER encode/decode demonstration using asn1pp.
Encodes integer 42 to BER bytes `02 01 2a`, decodes back, and verifies the round-trip.

Run:
```
cmake -B build -GNinja -DBASIC_DEMO=ON
ninja -C build basic-demo
./build/examples/basic/basic-demo
```