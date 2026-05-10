#include "codec/ber/der_encoder.hpp"
#include "codec/ber/decoder.hpp"
#include "codec/ber/tlv.hpp"

#include <cstdint>
#include <cstddef>
#include <span>
#include <algorithm>
#include <vector>

namespace asn1pp::ber {

bool der_encoder::is_integer_minimal(std::span<const uint8_t> content) {
    if (content.empty()) return false;
    if (content.size() == 1) return true;

    const uint8_t first = content[0];
    const uint8_t second = content[1];

    // DER minimal encoding rule (X.690 §11.5):
    // For positive: leading 0x00 is unnecessary if second byte has MSB set (1xxxxxxx)
    //   -> i.e., 0x00 is necessary only if second byte MSB is set (0x00, 0x80+)
    //   -> 0x00, 0x7F is NOT minimal (0x7F alone suffices)
    // For negative: leading 0xFF is unnecessary if second byte has MSB clear (0xxxxxxx)
    //   -> i.e., 0xFF is necessary only if second byte MSB is clear
    //   -> 0xFF, 0x7F IS minimal (need sign extension)
    //   -> 0xFF, 0x80 is NOT minimal (0x80 alone suffices for -128)

    if (first == 0x00 && (second & 0x80) == 0) {
        // Leading 0x00 is unnecessary because second byte doesn't need sign extension
        return false;
    }
    if (first == 0xFF && (second & 0x80) != 0) {
        // Leading 0xFF is unnecessary because second byte already has MSB set (negative)
        return false;
    }
    return true;
}

size_t der_encoder::count_sign_leading_octets(std::span<const uint8_t> content,
                                               uint8_t sign_byte) {
    size_t count = 0;
    while (count < content.size() && content[count] == sign_byte) {
        ++count;
    }
    return count;
}

result<void> der_encoder::encode_boolean(bool value, buffer_view& buf) {
    return ber_enc_.encode_boolean(value, buf);
}

result<void> der_encoder::encode_integer(int64_t value, buffer_view& buf) {
    return ber_enc_.encode_integer(value, buf);
}

result<void> der_encoder::encode_null(buffer_view& buf) {
    return ber_enc_.encode_null(buf);
}

result<void> der_encoder::encode_octet_string(std::span<const uint8_t> data,
                                              buffer_view& buf) {
    return ber_enc_.encode_octet_string(data, buf);
}

result<void> der_encoder::encode_bit_string(std::span<const uint8_t> data,
                                             uint8_t unused_bits,
                                             buffer_view& buf) {
    return ber_enc_.encode_bit_string(data, unused_bits, buf);
}

result<void> der_encoder::encode_enumerated(int64_t value, buffer_view& buf) {
    return ber_enc_.encode_enumerated(value, buf);
}

result<void> der_encoder::validate(std::span<const uint8_t> encoded) {
    buffer_view bv(const_cast<uint8_t*>(encoded.data()), encoded.size());
    auto tlv = ber_dec_.decode_tlv(bv);
    if (tlv.is_err()) return result<void>::err(tlv.error());

    const auto& t = tlv.value().t;

    if (t.cls == tag_class::universal) {
        if (t.number == static_cast<uint32_t>(universal_tag::octet_string) ||
            t.number == static_cast<uint32_t>(universal_tag::bit_string)) {
            if (t.constructed) {
                return result<void>::err(error_code::constraint_violation);
            }
        }

        if (t.number == static_cast<uint32_t>(universal_tag::boolean)) {
            return validate_boolean_der(tlv.value().value);
        }

        if (t.number == static_cast<uint32_t>(universal_tag::integer) ||
            t.number == static_cast<uint32_t>(universal_tag::enumerated)) {
            return validate_integer_der(tlv.value().value);
        }

        if (t.number == static_cast<uint32_t>(universal_tag::bit_string)) {
            return validate_bit_string_unused_bits(tlv.value().value);
        }
    }

    return validate_length_definite(encoded);
}

result<void> der_encoder::validate_boolean_der(std::span<const uint8_t> content) const {
    if (content.size() != 1) {
        return result<void>::err(error_code::constraint_violation);
    }
    if (content[0] != 0x00 && content[0] != 0xFF) {
        return result<void>::err(error_code::constraint_violation);
    }
    return result<void>::ok();
}

result<void> der_encoder::validate_integer_der(std::span<const uint8_t> content) const {
    if (content.empty()) {
        return result<void>::err(error_code::constraint_violation);
    }

    if (content.size() == 1) {
        return result<void>::ok();
    }

    const uint8_t first = content[0];
    const bool positive = (first & 0x80) == 0;

    if (positive) {
        if (first == 0x00) {
            if (content.size() > 1 && (content[1] & 0x80) == 0) {
                return result<void>::err(error_code::constraint_violation);
            }
        }
    } else {
        if (first == 0xFF) {
            size_t leading_ff = count_sign_leading_octets(content, 0xFF);
            if (leading_ff >= 2) {
                return result<void>::err(error_code::constraint_violation);
            }
        }
    }

    return result<void>::ok();
}

result<void> der_encoder::validate_length_definite(std::span<const uint8_t> tlv) {
    if (tlv.empty()) return result<void>::err(error_code::buffer_underflow);

    buffer_view bv(const_cast<uint8_t*>(tlv.data()), tlv.size());
    auto tag_r = ber::decode_tag(bv);
    if (tag_r.is_err()) return result<void>::err(tag_r.error());

    auto len_r = ber::decode_length(bv);
    if (len_r.is_err()) return result<void>::err(len_r.error());

    if (len_r.value() == INDEFINITE_LENGTH) {
        return result<void>::err(error_code::invalid_length);
    }

    return result<void>::ok();
}

result<void> der_encoder::validate_primitive_string(std::span<const uint8_t> tlv) {
    if (tlv.empty()) return result<void>::err(error_code::buffer_underflow);

    buffer_view bv(const_cast<uint8_t*>(tlv.data()), tlv.size());
    auto tag_r = ber::decode_tag(bv);
    if (tag_r.is_err()) return result<void>::err(tag_r.error());

    if (tag_r.value().constructed) {
        return result<void>::err(error_code::constraint_violation);
    }

    return result<void>::ok();
}

result<void> der_encoder::validate_bit_string_unused_bits(std::span<const uint8_t> content) const {
    if (content.empty()) {
        return result<void>::err(error_code::constraint_violation);
    }

    const uint8_t unused_bits = content[0];
    if (unused_bits > 7) {
        return result<void>::err(error_code::constraint_violation);
    }

    if (content.size() > 1) {
        const uint8_t last_byte = content[content.size() - 1];
        if (unused_bits > 0) {
            const uint8_t mask = (0xFF << (8 - unused_bits));
            if ((last_byte & mask) != 0) {
                return result<void>::err(error_code::constraint_violation);
            }
        }
    }

    return result<void>::ok();
}

} // namespace asn1pp::ber
