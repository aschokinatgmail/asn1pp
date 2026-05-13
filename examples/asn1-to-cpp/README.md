# ASN.1 to C++ Workflow Example

This standalone example demonstrates the full asn1pp workflow:

1. Define a protocol message in `message.asn`.
2. Generate C++ data types with `asn1pp-gen` into `message.hpp`.
3. Include the generated header from `main.cpp`.
4. BER encode a `ProtocolMessage`, decode it back, and verify the round-trip.

`message.hpp` is intentionally checked in as pre-generated example data so the demo can be built without running the generator first.

Regenerate the header from this directory:

```sh
../../build_gen/src/asn1pp-gen --input message.asn --output message.hpp
```

Build and run the standalone demo from this directory after building the parent project:

```sh
cmake -B build -GNinja
ninja -C build
./build/asn1-to-cpp-demo
```

Expected output includes:

```text
PASS: round-trip verified
```
