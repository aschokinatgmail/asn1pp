#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <span>
#include <vector>

#include "message.hpp"

#include "buffer/buffer_view.hpp"
#include "codec/ber/decoder.hpp"
#include "codec/ber/encoder.hpp"
#include "codec/result.hpp"
#include "codec/traits.hpp"

using asn1pp::buffer_view;
using asn1pp::error_code;

namespace {

constexpr std::size_t encoded_length_size(std::size_t length) noexcept {
    if (length <= 127) {
        return 1;
    }

    std::size_t octets = 0;
    while (length > 0) {
        length >>= 8;
        ++octets;
    }
    return 1 + octets;
}

asn1pp::result<void> encode_length(std::size_t length, buffer_view& out) {
    if (out.size() < encoded_length_size(length)) {
        return asn1pp::result<void>::err(error_code::buffer_overflow);
    }

    auto* bytes = const_cast<uint8_t*>(out.data());
    if (length <= 127) {
        bytes[0] = static_cast<uint8_t>(length);
        out = out.subview(1, out.size() - 1);
        return asn1pp::result<void>::ok();
    }

    const auto length_octets = encoded_length_size(length) - 1;
    bytes[0] = static_cast<uint8_t>(0x80U | length_octets);
    for (std::size_t i = 0; i < length_octets; ++i) {
        const auto shift = (length_octets - 1 - i) * 8;
        bytes[1 + i] = static_cast<uint8_t>((length >> shift) & 0xffU);
    }

    out = out.subview(1 + length_octets, out.size() - 1 - length_octets);
    return asn1pp::result<void>::ok();
}

asn1pp::result<std::size_t> decode_length(buffer_view& in) {
    if (in.empty()) {
        return asn1pp::result<std::size_t>::err(error_code::buffer_underflow);
    }

    const auto first = in[0];
    in = in.subview(1, in.size() - 1);
    if ((first & 0x80U) == 0) {
        return asn1pp::result<std::size_t>::ok(first);
    }

    const auto length_octets = static_cast<std::size_t>(first & 0x7fU);
    if (length_octets == 0 || in.size() < length_octets) {
        return asn1pp::result<std::size_t>::err(error_code::invalid_length);
    }

    std::size_t length = 0;
    for (std::size_t i = 0; i < length_octets; ++i) {
        length = (length << 8) | in[i];
    }

    in = in.subview(length_octets, in.size() - length_octets);
    return asn1pp::result<std::size_t>::ok(length);
}

asn1pp::result<void> copy_bytes(std::span<const uint8_t> source, buffer_view& out) {
    if (out.size() < source.size()) {
        return asn1pp::result<void>::err(error_code::buffer_overflow);
    }

    auto* bytes = const_cast<uint8_t*>(out.data());
    for (std::size_t i = 0; i < source.size(); ++i) {
        bytes[i] = source[i];
    }

    out = out.subview(source.size(), out.size() - source.size());
    return asn1pp::result<void>::ok();
}

asn1pp::result<void> encode_protocol_message(const ProtocolMessage& message,
                                             buffer_view& out) {
    std::vector<uint8_t> content(256);
    buffer_view content_out(content);
    asn1pp::ber::ber_encoder encoder;

    auto version = encoder.encode_integer(message.version, content_out);
    if (version.is_err()) {
        return version;
    }

    auto message_id = encoder.encode_integer(message.messageId, content_out);
    if (message_id.is_err()) {
        return message_id;
    }

    auto urgent = encoder.encode_boolean(message.urgent, content_out);
    if (urgent.is_err()) {
        return urgent;
    }

    auto payload = encoder.encode_octet_string(message.payload, content_out);
    if (payload.is_err()) {
        return payload;
    }

    const auto flush_result = encoder.flush_encode();
    if (flush_result != error_code::ok) {
        return asn1pp::result<void>::err(flush_result);
    }

    const auto content_size = content.size() - content_out.size();
    out = out.subview(0, out.size());
    if (out.size() < 1 + encoded_length_size(content_size) + content_size) {
        return asn1pp::result<void>::err(error_code::buffer_overflow);
    }

    auto* bytes = const_cast<uint8_t*>(out.data());
    bytes[0] = 0x30;  // UNIVERSAL CONSTRUCTED SEQUENCE
    out = out.subview(1, out.size() - 1);

    auto length = encode_length(content_size, out);
    if (length.is_err()) {
        return length;
    }

    return copy_bytes(std::span<const uint8_t>(content.data(), content_size), out);
}

asn1pp::result<ProtocolMessage> decode_protocol_message(buffer_view& in) {
    if (in.empty() || in[0] != 0x30) {
        return asn1pp::result<ProtocolMessage>::err(error_code::invalid_tag);
    }
    in = in.subview(1, in.size() - 1);

    auto length = decode_length(in);
    if (length.is_err()) {
        return asn1pp::result<ProtocolMessage>::err(length.error());
    }
    if (in.size() < length.value()) {
        return asn1pp::result<ProtocolMessage>::err(error_code::buffer_underflow);
    }

    buffer_view content = in.subview(0, length.value());
    in = in.subview(length.value(), in.size() - length.value());

    asn1pp::ber::ber_decoder decoder;
    ProtocolMessage message;

    auto version = decoder.decode_integer(content);
    if (version.is_err()) {
        return asn1pp::result<ProtocolMessage>::err(version.error());
    }
    message.version = version.value();

    auto message_id = decoder.decode_integer(content);
    if (message_id.is_err()) {
        return asn1pp::result<ProtocolMessage>::err(message_id.error());
    }
    message.messageId = message_id.value();

    auto urgent = decoder.decode_boolean(content);
    if (urgent.is_err()) {
        return asn1pp::result<ProtocolMessage>::err(urgent.error());
    }
    message.urgent = urgent.value();

    auto payload = decoder.decode_octet_string(content);
    if (payload.is_err()) {
        return asn1pp::result<ProtocolMessage>::err(payload.error());
    }
    message.payload = payload.value();

    if (!content.empty()) {
        return asn1pp::result<ProtocolMessage>::err(error_code::invalid_length);
    }

    const auto flush_result = decoder.flush_decode();
    if (flush_result != error_code::ok) {
        return asn1pp::result<ProtocolMessage>::err(flush_result);
    }

    return asn1pp::result<ProtocolMessage>::ok(message);
}

void hex_dump(std::span<const uint8_t> data) {
    for (uint8_t byte : data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(byte) << ' ';
    }
    std::cout << std::dec << '\n';
}

}  // namespace

int main() {
    ProtocolMessage original{
        .version = 1,
        .messageId = 42,
        .urgent = true,
        .payload = {0xde, 0xad, 0xbe, 0xef},
    };

    std::vector<uint8_t> encoded(256);
    buffer_view out(encoded);
    auto encoded_result = encode_protocol_message(original, out);
    if (encoded_result.is_err()) {
        std::cerr << "encode failed: " << static_cast<int>(encoded_result.error()) << '\n';
        return 1;
    }

    const auto encoded_size = encoded.size() - out.size();
    std::cout << "encoded ProtocolMessage (" << encoded_size << " bytes): ";
    hex_dump(std::span<const uint8_t>(encoded.data(), encoded_size));

    buffer_view in(encoded.data(), encoded_size);
    auto decoded_result = decode_protocol_message(in);
    if (decoded_result.is_err()) {
        std::cerr << "decode failed: " << static_cast<int>(decoded_result.error()) << '\n';
        return 1;
    }

    if (decoded_result.value() == original && in.empty()) {
        std::cout << "PASS: round-trip verified\n";
        return 0;
    }

    std::cerr << "FAIL: decoded message does not match original\n";
    return 1;
}
