---
title: Code Generator
---

# asn1pp-gen Code Generator

`asn1pp-gen` is a command-line tool that transforms ASN.1 schema definitions into human-readable C++ code. It parses `.asn` files following the ITU-T X.680 standard and emits type-safe C++ structures that integrate with the asn1pp codec library for encoding and decoding.

## Overview

The generator reads ASN.1 module definitions and produces C++ header files containing:

- Type definitions (structs, enums, variant types)
- Named member fields matching the ASN.1 component names
- Serialization methods compatible with asn1pp encoders
- Equality operators, validation, and string conversion where requested

Unlike stub generators that emit placeholder classes, `asn1pp-gen` produces complete, compilation-ready C++ code with proper type mappings and codec integration points.

## Building asn1pp-gen

The generator is built as part of the asn1pp CMake project. Enable it with the `BUILD_GEN` option:

```bash
cmake -DBUILD_GEN=ON -S . -B build
cmake --build build --target asn1pp-gen
```

The binary ends up at `build/bin/asn1pp-gen`. You can also run it directly from the build tree without installing.

## Command Line Interface

### CLI Reference

| Option | Description |
|--------|-------------|
| `--input <file>` | Input ASN.1 file (required unless `--dry-run` is used) |
| `--output <dir>` | Output file path. Writes to stdout if omitted. |
| `--encoding <rule>` | Target encoding rule. Currently accepted but not used (reserved for future codec generation). |
| `--dry-run` | Parse and validate the input without generating output. |
| `--help` | Print usage information and exit. |
| `--version` | Print version string and exit. |

### Usage Examples

**Validate an ASN.1 file:**
```bash
asn1pp-gen --dry-run --input protocol.asn
```

**Generate C++ header:**
```bash
asn1pp-gen --input protocol.asn --output protocol.hpp
```

**Use in a CMake custom command:**
```cmake
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/protocol.hpp
    COMMAND asn1pp-gen --input ${CMAKE_SOURCE_DIR}/protocol.asn
                            --output ${CMAKE_CURRENT_BINARY_DIR}/protocol.hpp
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol.asn
)
```

## Workflow: ASN.1 Schema to C++ Code

The typical workflow involves three steps:

1. **Author** your ASN.1 schema in a `.asn` file
2. **Generate** C++ header with `asn1pp-gen`
3. **Integrate** the header into your project and use the asn1pp codecs for encoding/decoding

### Step 1: Write the Schema

Create a file named `SimpleProtocol.asn`:

```asn
SimpleProtocol DEFINITIONS ::= BEGIN
    Message ::= SEQUENCE {
        id       INTEGER,
        payload  OCTET STRING,
        active   BOOLEAN
    }
END
```

### Step 2: Generate C++ Code

```bash
asn1pp-gen --input SimpleProtocol.asn --output simple_protocol.hpp
```

### Step 3: Generated Output

The tool produces the following C++ code:

```cpp
#ifndef SIMPLEPROTOCOL_HPP_INCLUDED
#define SIMPLEPROTOCOL_HPP_INCLUDED

#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <stdexcept>

namespace simple_protocol {

struct message {
    std::int64_t id;
    std::vector<std::uint8_t> payload;
    bool active;

    bool operator==(const message& other) const = default;

    void validate() const {
        // codec integration point
    }

    static message decode_ber(std::span<const std::uint8_t> data);
    std::vector<std::uint8_t> encode_ber() const;
};

}  // namespace simple_protocol

#endif  // SIMPLEPROTOCOL_HPP_INCLUDED
```

## Supported ASN.1 Types

The generator handles the following ASN.1 types and maps them to appropriate C++ constructs:

| ASN.1 Type | C++ Output | Notes |
|------------|------------|-------|
| `INTEGER` | `std::int64_t` | Signed 64-bit integer |
| `BOOLEAN` | `bool` | |
| `NULL` | `std::monostate` or custom `null_value` struct | |
| `OCTET STRING` | `std::vector<std::uint8_t>` | |
| `OBJECT IDENTIFIER` | `std::vector<std::uint32_t>` | OID components as uint32 array |
| `RELATIVE-OID` | `std::vector<std::uint32_t>` | |
| `SEQUENCE` | `struct` with named members | Each component becomes a field |
| `SET` | `struct` with named members | Order not guaranteed in output |
| `CHOICE` | `std::variant` over alternative types | |
| `ENUMERATED` | `enum class` | |
| `BIT STRING` | `std::vector<std::uint8_t>` with named bit access | |
| `SEQUENCE OF` | `std::vector<T>` | |
| `SET OF` | `std::vector<T>` | |
| `Tagged Type` | Tags are parsed but do not affect C++ type | |
| `Constrained Type` | Constraint is parsed, underlying type is generated | |

### Detailed Type Mappings

#### INTEGER

ASN.1:
```asn
PacketNumber ::= INTEGER
```

C++ Output:
```cpp
std::int64_t packet_number;
```

#### SEQUENCE

ASN.1:
```asn
Packet ::= SEQUENCE {
    sequence_number  INTEGER,
    source_address   OCTET STRING,
    checksum         BIT STRING
}
```

C++ Output:
```cpp
struct packet {
    std::int64_t sequence_number;
    std::vector<std::uint8_t> source_address;
    std::vector<std::uint8_t> checksum;

    bool operator==(const packet&) const = default;
};
```

#### CHOICE

ASN.1:
```asn
Address ::= CHOICE {
    ipv4  OCTET STRING,
    ipv6  OCTET STRING
}
```

C++ Output:
```cpp
#include <variant>

struct address {
    std::variant<
        std::vector<std::uint8_t>,  // ipv4
        std::vector<std::uint8_t>   // ipv6
    > choice;

    // accessors: ipv4(), ipv6(), which()
};
```

#### ENUMERATED

ASN.1:
```asn
Status ::= ENUMERATED {
    pending   (0),
    active    (1),
    completed (2)
}
```

C++ Output:
```cpp
enum class status {
    pending = 0,
    active = 1,
    completed = 2
};
```

#### SEQUENCE OF

ASN.1:
```asn
IpAddressList ::= SEQUENCE OF OCTET STRING
```

C++ Output:
```cpp
std::vector<std::vector<std::uint8_t>> ip_address_list;
```

## Quick Start

Here is a complete example demonstrating the full workflow from schema to encoded output.

### 1. Create the Schema

Save the following as `network_packet.asn`:

```asn
NetworkProtocol DEFINITIONS ::= BEGIN
    IPAddress ::= SEQUENCE {
        version     ENUMERATED { ipv4(4), ipv6(6) },
        address     OCTET STRING
    }

    NetworkPacket ::= SEQUENCE {
        header_size  INTEGER,
        source       IPAddress,
        destination  IPAddress,
        payload      OCTET STRING,
        checksum     BIT STRING
    }
END
```

### 2. Generate the Header

```bash
asn1pp-gen --input network_packet.asn --output network_packet.hpp
```

### 3. Use in Your Application

```cpp
#include <iostream>
#include <vector>
#include "network_packet.hpp"

using namespace network_protocol;

int main() {
    // Create a network packet
    network_packet pkt;
    pkt.header_size = 20;
    pkt.source.version = ipaddress::ipv4;
    pkt.source.address = {192, 168, 1, 1};
    pkt.destination.version = ipaddress::ipv4;
    pkt.destination.address = {10, 0, 0, 1};
    pkt.payload = {'H', 'e', 'l', 'l', 'o'};
    pkt.checksum = {0xAB, 0xCD};

    // Encode to BER
    auto encoded = pkt.encode_ber();

    std::cout << "Encoded " << encoded.size() << " bytes\n";

    // Decode back
    auto decoded = network_packet::decode_ber(encoded);

    std::cout << "Header size: " << decoded.header_size << "\n";
    std::cout << "Source: "
              << static_cast<int>(decoded.source.version) << " "
              << decoded.source.address.size() << " bytes\n";

    return 0;
}
```

## Limitations

`asn1pp-gen` intentionally omits certain features to keep the generated code clean and maintainable:

### No Cross-File IMPORTS

The generator does not resolve `IMPORTS` statements that reference types defined in other ASN.1 modules. Each `.asn` file must be self-contained. To work around this limitation:

- Combine all required definitions into a single `.asn` file
- Generate headers separately and merge the results manually
- Write type references as plain `T` and provide the actual definition in a different header

### No Parameterized Types

Parameterized type definitions (`SomeType { T } ::= SEQUENCE { field T }`) are not supported. The parser accepts them but the emitter skips them silently. Use concrete type instantiations instead.

### No Value Assignments

The generator processes type definitions only. Value assignments (`foo INTEGER ::= 42`) are parsed but do not produce C++ output. Encode values directly in your application code.

### No XML/JSON Codegen

The `--encoding` flag is accepted for future use but does not currently affect output. The generator always produces C++ type definitions. Encoding and decoding to specific formats is handled at runtime by the asn1pp codec library.

### No Embedded Constraints in Output

While the parser extracts constraint information (value ranges, size limits), the generated C++ types do not enforce these constraints at construction time. Call the `validate()` method to check constraints before encoding.

## Error Handling

The generator reports errors through a diagnostic engine:

```bash
$ asn1pp-gen --input broken.asn
error: line 5: unexpected token '}' expected type expression
error: line 7: module definition is incomplete
```

If the parser encounters syntax errors, no output file is produced and the tool returns exit code 1.

### Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Parse error or CLI argument error |
| 2 | Code generation error |