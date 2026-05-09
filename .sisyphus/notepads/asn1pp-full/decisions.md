# Decisions — asn1pp-full

## Interview Decisions
- **Approach**: Code Generator (reads .asn → outputs .hpp/.cpp)
- **C++ Standard**: C++20
- **Encoding Rules**: Full suite (BER/DER/CER, PER/UPER/CPER, OER/COER, XER/CXER/E-XER, JER)
- **Use Case**: General purpose
- **ASN.1 Coverage**: Full X.68x (IOC, parameterized types, table constraints — deferred to Wave 9)
- **Code Generator Role**: Both standalone CLI + CMake integration module
- **Parser**: Hand-written recursive descent
- **Buffer**: Custom buffer_view class
- **Constraints**: Both compile-time (concepts) and runtime validation
- **Build**: Separate CMake targets
- **Test Strategy**: TDD with GTest + GMock
- **C++20 Features**: All modern (concepts, std::span, constexpr, coroutines, std::expected)

## Metis Review Guardrails
- DER as wave-1 encoding rule
- No building all 12 codecs simultaneously — sequential, one per wave
- IOC blocked until core pipeline proven (Wave 9)
- PER constraint metadata must be constexpr, not runtime
- Cross-validation with asn1c for BER/DER
- Custom buffer_view for bit-level PER operations