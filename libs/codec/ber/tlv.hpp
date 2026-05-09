#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <cstring>

#include "codec/result.hpp"
#include "codec/traits.hpp"
#include "buffer/buffer_view.hpp"

namespace asn1pp::ber {

// ============================================================================
// Named constants per ITU-T X.690
// ============================================================================

constexpr uint8_t CONSTRUCTED_BIT   = 0x20;
constexpr uint8_t CLASS_MASK        = 0xC0;
constexpr uint8_t TAG_NUMBER_MASK   = 0x1F;
constexpr uint8_t LONG_TAG_SENTINEL = 0x1F;   // signals long-form tag
constexpr uint8_t INDEFINITE_LENGTH = 0x80;
constexpr uint8_t SHORT_LENGTH_MASK = 0x7F;
constexpr uint8_t LONG_LENGTH_FLAG  = 0x80;
constexpr uint8_t END_OF_CONTENT_TAG    = 0x00;
constexpr uint8_t END_OF_CONTENT_LENGTH = 0x00;
constexpr uint8_t CONT_BYTE_FLAG    = 0x80;   // continuation byte: bit 8 set
constexpr uint8_t CONT_BYTE_VALUE   = 0x7F;   // value portion of continuation byte
constexpr int TAG_CLASS_SHIFT       = 6;

// ============================================================================
// Forward declarations
// ============================================================================

constexpr size_t encoded_tag_size(const tag& t) noexcept;
constexpr size_t encoded_length_size(size_t length) noexcept;

// ============================================================================
// Tag encoding (X.690 Clause 8.1)
// ============================================================================

// encode_tag: writes identifier octets for the given tag into the buffer,
// advancing buf past the written bytes.
// The caller must ensure buf was created over a mutable backing buffer.
inline result<void> encode_tag(buffer_view& buf, const tag& t) {
    const size_t needed = encoded_tag_size(t);
    if (buf.size() < needed) return result<void>::err(error_code::buffer_overflow);

    // buf.data() is const*; for encoding the caller provides a mutable buffer
    uint8_t* out = const_cast<uint8_t*>(buf.data());

    if (t.number <= 30) {
        // Short form (X.690 8.1.2.3): single octet
        const uint8_t class_bits = static_cast<uint8_t>(
            static_cast<uint8_t>(t.cls) << TAG_CLASS_SHIFT);
        const uint8_t construct_bit = t.constructed ? CONSTRUCTED_BIT : 0;
        out[0] = class_bits | construct_bit | static_cast<uint8_t>(t.number);
        buf = buf.subview(1, buf.size() - 1);
    } else {
        // Long form (X.690 8.1.2.4): first octet + continuation bytes
        const uint8_t class_bits = static_cast<uint8_t>(
            static_cast<uint8_t>(t.cls) << TAG_CLASS_SHIFT);
        const uint8_t construct_bit = t.constructed ? CONSTRUCTED_BIT : 0;
        out[0] = class_bits | construct_bit | LONG_TAG_SENTINEL;

        // Encode tag number in base-128, big-endian
        // Count continuation bytes
        size_t cont_count = 1;
        {
            uint32_t tmp = t.number;
            while (tmp > 0x7F) { tmp >>= 7; ++cont_count; }
        }

        // Write continuation bytes in wire order (big-endian, MSB group first)
        for (size_t i = 0; i < cont_count; ++i) {
            const int shift = static_cast<int>((cont_count - 1 - i) * 7);
            const uint8_t byte_val = static_cast<uint8_t>((t.number >> shift) & CONT_BYTE_VALUE);
            out[1 + i] = (i < cont_count - 1) ? (byte_val | CONT_BYTE_FLAG) : byte_val;
        }

        buf = buf.subview(1 + cont_count, buf.size() - 1 - cont_count);
    }

    return result<void>::ok();
}

// decode_tag: reads identifier octets from buf, returns the decoded tag.
// Advances buf past the consumed bytes.
inline result<tag> decode_tag(buffer_view& buf) {
    if (buf.empty()) return result<tag>::err(error_code::buffer_underflow);

    const uint8_t first = buf[0];

    const auto cls = static_cast<tag_class>((first & CLASS_MASK) >> TAG_CLASS_SHIFT);
    const bool constructed = (first & CONSTRUCTED_BIT) != 0;
    const uint8_t low5 = first & TAG_NUMBER_MASK;

    uint32_t number = 0;

    if (low5 != LONG_TAG_SENTINEL) {
        // Short form
        number = low5;
        buf = buf.subview(1, buf.size() - 1);
    } else {
        // Long form (X.690 8.1.2.4.2)
        size_t consumed = 1;
        number = 0;

        // Read continuation bytes
        while (consumed < buf.size()) {
            const uint8_t byte_val = buf[consumed];
            ++consumed;
            number = (number << 7) | (byte_val & CONT_BYTE_VALUE);
            if ((byte_val & CONT_BYTE_FLAG) == 0) break;  // last byte
        }

        // Check we actually terminated properly
        if (consumed == buf.size() && consumed == 1) {
            // We started long form but had no continuation bytes
            return result<tag>::err(error_code::invalid_tag);
        }
        // If we ran out of bytes without finding the terminator:
        if (consumed == buf.size() && buf.size() > 1) {
            const uint8_t last_byte = buf[consumed - 1];
            if ((last_byte & CONT_BYTE_FLAG) != 0) {
                return result<tag>::err(error_code::buffer_underflow);
            }
        }

        buf = buf.subview(consumed, buf.size() - consumed);
    }

    return result<tag>::ok(tag{cls, constructed, number});
}

// ============================================================================
// Utility: encoded_tag_size
// ============================================================================

constexpr size_t encoded_tag_size(const tag& t) noexcept {
    if (t.number <= 30) return 1;
    // Long form: 1 initial octet + continuation bytes
    size_t cont = 1;
    uint32_t num = t.number;
    while (num > 0x7F) { num >>= 7; ++cont; }
    return 1 + cont;
}

// ============================================================================
// Length encoding (X.690 Clause 8.2)
// ============================================================================

// encode_length: writes length octets per X.690 8.1.3.
// indefinite = true emits the indefinite-length marker 0x80.
// Caller must provide mutable backing buffer for buf.
inline result<void> encode_length(buffer_view& buf, size_t length, bool indefinite = false) {
    if (indefinite) {
        if (buf.empty()) return result<void>::err(error_code::buffer_overflow);
        uint8_t* out = const_cast<uint8_t*>(buf.data());
        out[0] = INDEFINITE_LENGTH;
        buf = buf.subview(1, buf.size() - 1);
        return result<void>::ok();
    }

    uint8_t* out = const_cast<uint8_t*>(buf.data());

    if (length <= 127) {
        // Short form (X.690 8.1.3.4): single octet, bit 8 = 0
        if (buf.size() < 1) return result<void>::err(error_code::buffer_overflow);
        out[0] = static_cast<uint8_t>(length);
        buf = buf.subview(1, buf.size() - 1);
    } else {
        // Long form (X.690 8.1.3.5): 1+ len octets, bit 8 of first = 1
        const size_t cont = encoded_length_size(length) - 1;  // minus first octet
        if (buf.size() < 1 + cont) return result<void>::err(error_code::buffer_overflow);

        // First octet: 0x80 | number of length octets
        out[0] = static_cast<uint8_t>(LONG_LENGTH_FLAG | cont);

        // Write big-endian length
        size_t val = length;
        for (size_t i = 0; i < cont; ++i) {
            out[1 + cont - 1 - i] = static_cast<uint8_t>(val & 0xFF);
            val >>= 8;
        }
        buf = buf.subview(1 + cont, buf.size() - 1 - cont);
    }

    return result<void>::ok();
}

// decode_length: reads length octets from buf, advances past consumed bytes.
// Returns the length value, or INDEFINITE_LENGTH sentinel for indefinite form.
inline result<size_t> decode_length(buffer_view& buf) {
    if (buf.empty()) return result<size_t>::err(error_code::buffer_underflow);

    const uint8_t first = buf[0];

    if (first == INDEFINITE_LENGTH) {
        // Indefinite form
        buf = buf.subview(1, buf.size() - 1);
        // Return INDEFINITE_LENGTH as sentinel value
        return result<size_t>::ok(static_cast<size_t>(INDEFINITE_LENGTH));
    }

    if ((first & LONG_LENGTH_FLAG) == 0) {
        // Short form
        const size_t len = first;
        buf = buf.subview(1, buf.size() - 1);
        return result<size_t>::ok(len);
    }

    // Long form
    const size_t num_octets = first & SHORT_LENGTH_MASK;
    if (num_octets == 0) {
        // Reserved: 0x80 is indefinite, already handled; long form with 0 len bytes is invalid
        return result<size_t>::err(error_code::invalid_length);
    }
    // 0xFF is reserved for future use (X.690 8.1.3.5.1)
    // Actually the spec says: 0xFF is reserved. Let's reject it.
    if (num_octets == 0x7F) {
        return result<size_t>::err(error_code::invalid_length);
    }
    if (buf.size() < 1 + num_octets) {
        return result<size_t>::err(error_code::buffer_underflow);
    }
    if (num_octets > sizeof(size_t)) {
        // Length doesn't fit in our size_t
        return result<size_t>::err(error_code::invalid_length);
    }

    size_t length = 0;
    for (size_t i = 0; i < num_octets; ++i) {
        length = (length << 8) | buf[1 + i];
    }

    buf = buf.subview(1 + num_octets, buf.size() - 1 - num_octets);
    return result<size_t>::ok(length);
}

// ============================================================================
// Utility: encoded_length_size
// ============================================================================

constexpr size_t encoded_length_size(size_t length) noexcept {
    if (length <= 127) return 1;
    size_t cont = 0;
    size_t tmp = length;
    while (tmp > 0) { tmp >>= 8; ++cont; }
    return 1 + cont;
}

// ============================================================================
// End-of-content marker (X.690 8.1.5)
// ============================================================================

inline result<void> encode_end_of_content(buffer_view& buf) {
    if (buf.size() < 2) return result<void>::err(error_code::buffer_overflow);
    uint8_t* out = const_cast<uint8_t*>(buf.data());
    out[0] = END_OF_CONTENT_TAG;
    out[1] = END_OF_CONTENT_LENGTH;
    buf = buf.subview(2, buf.size() - 2);
    return result<void>::ok();
}

// ============================================================================
// Full TLV encoding convenience
// ============================================================================

// encode_tlv: writes tag + length + value as a complete TLV.
inline result<void> encode_tlv(buffer_view& buf, const tag& t, std::span<const uint8_t> value) {
    const size_t total_needed = encoded_tag_size(t) + encoded_length_size(value.size()) + value.size();
    if (buf.size() < total_needed) return result<void>::err(error_code::buffer_overflow);

    uint8_t* out = const_cast<uint8_t*>(buf.data());
    size_t pos = 0;

    // Encode tag
    {
        buffer_view tag_view(out + pos, buf.size() - pos);
        auto r = encode_tag(tag_view, t);
        if (r.is_err()) return r;
        pos += encoded_tag_size(t);
    }

    // Encode length
    {
        buffer_view len_view(out + pos, buf.size() - pos);
        auto r = encode_length(len_view, value.size());
        if (r.is_err()) return r;
        pos += encoded_length_size(value.size());
    }

    // Copy value
    if (!value.empty()) {
        std::memcpy(out + pos, value.data(), value.size());
        pos += value.size();
    }

    buf = buf.subview(pos, buf.size() - pos);
    return result<void>::ok();
}

// decode_tlv_header: reads tag and length from buf, advances past the TLV header.
// The caller then reads `out_length` bytes of value from the remaining buf.
inline result<size_t> decode_tlv_header(buffer_view& buf, tag& out_tag, size_t& out_length) {
    auto tag_res = decode_tag(buf);
    if (tag_res.is_err()) return result<size_t>::err(tag_res.error());
    out_tag = tag_res.value();

    auto len_res = decode_length(buf);
    if (len_res.is_err()) return result<size_t>::err(len_res.error());
    out_length = len_res.value();

    // Check that value fits in remaining buffer
    if (out_length != INDEFINITE_LENGTH && buf.size() < out_length) {
        return result<size_t>::err(error_code::buffer_underflow);
    }

    return result<size_t>::ok(out_length);
}

} // namespace asn1pp::ber
