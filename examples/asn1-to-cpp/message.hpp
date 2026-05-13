#ifndef ASN1PP_EXAMPLEPROTOCOL_HPP
#define ASN1PP_EXAMPLEPROTOCOL_HPP

#include "asn1pp/codec.hpp"
#include "asn1pp/traits.hpp"
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

struct ProtocolMessage {
    int64_t version{};
    int64_t messageId{};
    bool urgent{};
    std::vector<uint8_t> payload{};

    bool operator==(const ProtocolMessage&) const = default;
};

template<> struct asn1pp::asn1_tag<ProtocolMessage> { static constexpr auto value = asn1pp::make_universal(asn1pp::universal_tag::sequence, true); };


#endif // ASN1PP_EXAMPLEPROTOCOL_HPP
