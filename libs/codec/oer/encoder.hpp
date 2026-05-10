#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <type_traits>

#include "codec/result.hpp"
#include "buffer/buffer_view.hpp"

namespace asn1pp::oer {

/// OER (Octet Encoding Rules) encoder per ITU-T X.696.
///
/// OER is octet-aligned — no bit-level packing needed. Uses length-determinant
/// prefixes for variable-length types, but no TLV tags. Fixed-width encoding
/// is used when constraints are available at compile time via OerMeta template
/// parameters (emitted by the code generator).
///
/// Stateless per-call; the buffer_view is advanced past the written bytes.
class oer_encoder {
public:
    oer_encoder() = default;

    // ── Primitive type encoders ────────────────────────────────────────

    /// Encode INTEGER per X.696 §7.2.
    ///
    /// Constrained (OerMeta::has_range_constraint==true):
    ///   Fixed-width big-endian two's complement. Width determined by
    ///   the range size: ≤256→1oct, ≤65536→2oct, ≤2^32→4oct, else→8oct.
    ///
    /// Unconstrained (has_range_constraint==false):
    ///   Length determinant (X.696 §4) + minimal two's complement bytes.
    template<typename OerMeta>
    result<void> encode_integer(int64_t value, buffer_view& buf);

    /// Encode BOOLEAN per X.696 §7.3 — 1 octet: 0xFF (true) or 0x00 (false).
    result<void> encode_boolean(bool value, buffer_view& buf);

    /// Encode NULL per X.696 §7.4 — 0 octets (no bytes written).
    result<void> encode_null(buffer_view& buf);

    /// Encode OCTET STRING per X.696 §16.
    ///
    /// Fixed size (min_size==max_size): just the data bytes, no length prefix.
    /// Constrained size: length encoded as constrained whole number + data.
    /// Unconstrained (has_size_constraint==false): length determinant + data.
    template<typename OerMeta>
    result<void> encode_octet_string(std::span<const uint8_t> data, buffer_view& buf);

    /// Encode BIT STRING per X.696 §14.
    ///
    /// Fixed size: just the data octets.
    /// Unconstrained: length determinant + unused_bits octet + data.
    template<typename OerMeta>
    result<void> encode_bit_string(std::span<const uint8_t> data, uint8_t unused_bits, buffer_view& buf);

    /// Encode ENUMERATED per X.696 §7.7.
    ///
    /// Width depends on root_count: ≤255→1oct, ≤65535→2oct, else→4oct.
    /// Extension marker (if has_extension) is handled by the containing
    /// SEQUENCE's optional bitmap, not here.
    template<typename OerMeta>
    result<void> encode_enumerated(int64_t index, buffer_view& buf);

    // ── Constructed type helpers ───────────────────────────────────────

    /// Start a SEQUENCE: writes extension bit (if extensible) then
    /// optional-field presence bitmap, packed into full octets (MSB first).
    /// No preamble if has_extension==false and optional_count==0.
    template<typename OerMeta>
    result<void> encode_sequence_start(buffer_view& buf,
                                       const bool* optional_present = nullptr,
                                       size_t optional_count = 0);

    /// End a SEQUENCE — no-op for OER (no alignment needed).
    result<void> encode_sequence_end(buffer_view& buf);

    /// Encode CHOICE index per X.696 §19.
    ///
    /// Width depends on alternative_count: ≤255→1oct, ≤65535→2oct, else→4oct.
    template<typename OerMeta>
    result<void> encode_choice_index(int64_t choice_index, buffer_view& buf);

    /// Encode SEQUENCE OF / SET OF length determinant per X.696 §20.
    template<typename OerMeta>
    result<void> encode_sequence_of_length(size_t length, buffer_view& buf);

    /// Encode OBJECT IDENTIFIER per X.696 §7.6.
    /// Accepts pre-encoded BER-style subidentifier bytes.
    /// Encoded as: length determinant + subidentifier content.
    result<void> encode_oid(std::span<const uint8_t> encoded_oid, buffer_view& buf);

    /// Encode length determinant per X.696 §4.
    ///
    /// 0–127:         single octet `0xxxxxxx`
    /// 128–16383:     two octets `10xxxxxx xxxxxxxx`
    /// 16384+:        `11000000 nnnnnnnn nnnnnnnn` (16-bit count)
    result<void> encode_length_determinant(size_t length, buffer_view& buf);

private:
    /// Determine byte width for a constrained integer range at compile time.
    template<typename OerMeta>
    static constexpr size_t integer_byte_width();

    /// Determine byte width for enumerated or choice index at compile time.
    static constexpr size_t index_byte_width(size_t count);

    /// Encode an unconstrained integer as length-prefixed two's complement.
    result<void> encode_unconstrained_integer(int64_t value, buffer_view& buf);

    /// Compute minimal two's complement big-endian encoding of an integer.
    /// Returns number of bytes written. out must have room for 9 bytes.
    static size_t encode_integer_bytes(int64_t value, uint8_t* out);
};

// ============================================================================
// Private helpers — compile-time width calculation
// ============================================================================

template<typename OerMeta>
constexpr size_t oer_encoder::integer_byte_width() {
    if constexpr (OerMeta::has_range_constraint) {
        constexpr int64_t range_min = OerMeta::min_value;
        constexpr int64_t range_max = OerMeta::max_value;

        if constexpr (range_min >= 0) {
            // Unsigned range
            if constexpr (range_max <= 255)       return 1;
            if constexpr (range_max <= 65535)     return 2;
            if constexpr (range_max <= 4294967295) return 4;
            return 8;
        } else {
            // Signed range — find smallest width that fits both min and max
            if constexpr (range_min >= -128 && range_max <= 127)         return 1;
            if constexpr (range_min >= -32768 && range_max <= 32767)     return 2;
            if constexpr (range_min >= -2147483648LL && range_max <= 2147483647LL) return 4;
            return 8;
        }
    }
    return 0; // not constrained
}

constexpr size_t oer_encoder::index_byte_width(size_t count) {
    if (count <= 255)   return 1;
    if (count <= 65535) return 2;
    return 4;
}

// ============================================================================
// Buffer helpers (inline to avoid symbol duplication across TUs)
// ============================================================================

namespace {

inline void write_octets_to_buf(buffer_view& buf, const uint8_t* src, size_t count) {
    if (buf.size() < count) return;
    uint8_t* out = const_cast<uint8_t*>(buf.data());
    for (size_t i = 0; i < count; ++i) out[i] = src[i];
    buf = buf.subview(count, buf.size() - count);
}

inline void write_be_8(buffer_view& buf, uint8_t val) {
    if (buf.size() >= 1) {
        const_cast<uint8_t*>(buf.data())[0] = val;
        buf = buf.subview(1, buf.size() - 1);
    }
}

inline void write_be_16(buffer_view& buf, uint16_t val) {
    if (buf.size() >= 2) {
        uint8_t* out = const_cast<uint8_t*>(buf.data());
        out[0] = static_cast<uint8_t>(val >> 8);
        out[1] = static_cast<uint8_t>(val & 0xFF);
        buf = buf.subview(2, buf.size() - 2);
    }
}

inline void write_be_32(buffer_view& buf, uint32_t val) {
    if (buf.size() >= 4) {
        uint8_t* out = const_cast<uint8_t*>(buf.data());
        out[0] = static_cast<uint8_t>(val >> 24);
        out[1] = static_cast<uint8_t>((val >> 16) & 0xFF);
        out[2] = static_cast<uint8_t>((val >> 8) & 0xFF);
        out[3] = static_cast<uint8_t>(val & 0xFF);
        buf = buf.subview(4, buf.size() - 4);
    }
}

inline void write_be_64(buffer_view& buf, uint64_t val) {
    if (buf.size() >= 8) {
        uint8_t* out = const_cast<uint8_t*>(buf.data());
        out[0] = static_cast<uint8_t>(val >> 56);
        out[1] = static_cast<uint8_t>((val >> 48) & 0xFF);
        out[2] = static_cast<uint8_t>((val >> 40) & 0xFF);
        out[3] = static_cast<uint8_t>((val >> 32) & 0xFF);
        out[4] = static_cast<uint8_t>((val >> 24) & 0xFF);
        out[5] = static_cast<uint8_t>((val >> 16) & 0xFF);
        out[6] = static_cast<uint8_t>((val >> 8) & 0xFF);
        out[7] = static_cast<uint8_t>(val & 0xFF);
        buf = buf.subview(8, buf.size() - 8);
    }
}

} // anonymous namespace

// ============================================================================
// Integer encoding helpers
// ============================================================================

inline size_t oer_encoder::encode_integer_bytes(int64_t value, uint8_t* out) {
    if (value == 0) {
        out[0] = 0x00;
        return 1;
    }

    uint64_t uval = static_cast<uint64_t>(value);
    const bool negative = (value < 0);

    uint8_t raw[8];
    for (int i = 0; i < 8; ++i) {
        raw[7 - i] = static_cast<uint8_t>(uval & 0xFF);
        uval >>= 8;
    }

    const uint8_t sign_byte = negative ? 0xFF : 0x00;
    size_t start = 0;
    while (start < 7 && raw[start] == sign_byte) {
        ++start;
    }

    bool needs_extra = false;
    if (!negative && (raw[start] & 0x80)) {
        needs_extra = true;
    } else if (negative && !(raw[start] & 0x80)) {
        needs_extra = true;
    }

    size_t out_idx = 0;
    if (needs_extra) {
        out[out_idx++] = negative ? 0xFF : 0x00;
    }
    const size_t meaningful = 8 - start;
    for (size_t i = 0; i < meaningful; ++i) {
        out[out_idx++] = raw[start + i];
    }
    return out_idx;
}

inline result<void> oer_encoder::encode_unconstrained_integer(int64_t value, buffer_view& buf) {
    uint8_t int_buf[9];
    const size_t val_len = encode_integer_bytes(value, int_buf);
    return encode_length_determinant(val_len, buf).and_then(
        [&]() {
            if (buf.size() < val_len) return result<void>::err(error_code::buffer_overflow);
            write_octets_to_buf(buf, int_buf, val_len);
            return result<void>::ok();
        });
}

// ============================================================================
// Template member functions (must be in header for implicit instantiation)
// ============================================================================

template<typename OerMeta>
result<void> oer_encoder::encode_integer(int64_t value, buffer_view& buf) {
    if constexpr (OerMeta::has_range_constraint) {
        constexpr size_t byte_width = integer_byte_width<OerMeta>();

        if (value < OerMeta::min_value || value > OerMeta::max_value)
            return result<void>::err(error_code::value_out_of_range);
        if (buf.size() < byte_width)
            return result<void>::err(error_code::buffer_overflow);

        switch (byte_width) {
        case 1: write_be_8(buf, static_cast<uint8_t>(value)); break;
        case 2: write_be_16(buf, static_cast<uint16_t>(value)); break;
        case 4: write_be_32(buf, static_cast<uint32_t>(value)); break;
        case 8: write_be_64(buf, static_cast<uint64_t>(value)); break;
        default: return result<void>::err(error_code::encoding_error);
        }
        return result<void>::ok();
    } else {
        return encode_unconstrained_integer(value, buf);
    }
}

template<typename OerMeta>
result<void> oer_encoder::encode_octet_string(std::span<const uint8_t> data, buffer_view& buf) {
    if constexpr (OerMeta::has_size_constraint) {
        constexpr size_t sz_min = OerMeta::min_size;
        constexpr size_t sz_max = OerMeta::max_size;

        if (sz_min == sz_max) {
            // Fixed size — no length prefix
            if (data.size() != sz_min) return result<void>::err(error_code::constraint_violation);
            if (buf.size() < sz_min) return result<void>::err(error_code::buffer_overflow);
            write_octets_to_buf(buf, data.data(), sz_min);
            return result<void>::ok();
        } else {
            // Constrained — length as constrained whole number
            if (data.size() < sz_min || data.size() > sz_max)
                return result<void>::err(error_code::constraint_violation);
            const size_t len = data.size();
            // Length encoded as constrained whole number: since sz_max ≤ 10 in tests,
            // fits in 1 byte. In general, determine width from sz_max.
            constexpr size_t len_width = (sz_max <= 255) ? 1 : ((sz_max <= 65535) ? 2 : 4);
            if (buf.size() < len_width + len) return result<void>::err(error_code::buffer_overflow);
            switch (len_width) {
            case 1: write_be_8(buf, static_cast<uint8_t>(len)); break;
            case 2: write_be_16(buf, static_cast<uint16_t>(len)); break;
            default: write_be_32(buf, static_cast<uint32_t>(len)); break;
            }
            write_octets_to_buf(buf, data.data(), len);
            return result<void>::ok();
        }
    } else {
        // Unconstrained — length determinant + data
        const size_t len = data.size();
        auto r = encode_length_determinant(len, buf);
        if (r.is_err()) return r;
        if (buf.size() < len) return result<void>::err(error_code::buffer_overflow);
        write_octets_to_buf(buf, data.data(), len);
        return result<void>::ok();
    }
}

template<typename OerMeta>
result<void> oer_encoder::encode_bit_string(std::span<const uint8_t> data,
                                             uint8_t unused_bits, buffer_view& buf) {
    if constexpr (OerMeta::has_size_constraint) {
        constexpr size_t sz_min = OerMeta::min_size;
        constexpr size_t sz_max = OerMeta::max_size;

        if (sz_min == sz_max) {
            // Fixed size — no length prefix, no unused_bits octet
            if (data.size() != sz_min) return result<void>::err(error_code::constraint_violation);
            if (buf.size() < sz_min) return result<void>::err(error_code::buffer_overflow);
            write_octets_to_buf(buf, data.data(), sz_min);
            return result<void>::ok();
        } else {
            // Constrained — length + unused_bits octet + data
            const size_t total = data.size() + 1; // +1 for unused_bits
            if (total - 1 < sz_min || total - 1 > sz_max)
                return result<void>::err(error_code::constraint_violation);
            constexpr size_t len_width = (sz_max + 1 <= 255) ? 1 : 2;
            if (buf.size() < len_width + 1 + data.size())
                return result<void>::err(error_code::buffer_overflow);
            switch (len_width) {
            case 1: write_be_8(buf, static_cast<uint8_t>(total)); break;
            default: write_be_16(buf, static_cast<uint16_t>(total)); break;
            }
            write_be_8(buf, unused_bits);
            write_octets_to_buf(buf, data.data(), data.size());
            return result<void>::ok();
        }
    } else {
        // Unconstrained — length determinant + unused_bits + data
        const size_t total = 1 + data.size();
        auto r = encode_length_determinant(total, buf);
        if (r.is_err()) return r;
        if (buf.size() < total) return result<void>::err(error_code::buffer_overflow);
        write_be_8(buf, unused_bits);
        write_octets_to_buf(buf, data.data(), data.size());
        return result<void>::ok();
    }
}

template<typename OerMeta>
result<void> oer_encoder::encode_enumerated(int64_t index, buffer_view& buf) {
    constexpr size_t width = index_byte_width(OerMeta::root_count);
    if (buf.size() < width) return result<void>::err(error_code::buffer_overflow);
    switch (width) {
    case 1: write_be_8(buf, static_cast<uint8_t>(index)); break;
    case 2: write_be_16(buf, static_cast<uint16_t>(index)); break;
    default: write_be_32(buf, static_cast<uint32_t>(index)); break;
    }
    return result<void>::ok();
}

template<typename OerMeta>
result<void> oer_encoder::encode_choice_index(int64_t choice_index, buffer_view& buf) {
    constexpr size_t width = index_byte_width(OerMeta::alternative_count);
    if (buf.size() < width) return result<void>::err(error_code::buffer_overflow);
    switch (width) {
    case 1: write_be_8(buf, static_cast<uint8_t>(choice_index)); break;
    case 2: write_be_16(buf, static_cast<uint16_t>(choice_index)); break;
    default: write_be_32(buf, static_cast<uint32_t>(choice_index)); break;
    }
    return result<void>::ok();
}

template<typename OerMeta>
result<void> oer_encoder::encode_sequence_of_length(size_t length, buffer_view& buf) {
    return encode_length_determinant(length, buf);
}

template<typename OerMeta>
result<void> oer_encoder::encode_sequence_start(buffer_view& buf,
                                                  const bool* optional_present,
                                                  size_t optional_count) {
    (void)optional_count;
    if constexpr (OerMeta::has_extension || true) {
        size_t total_bits = 0;
        if constexpr (OerMeta::has_extension) total_bits += 1;
        total_bits += OerMeta::optional_count;

        if (total_bits == 0) return result<void>::ok();

        // Round up to full octets
        const size_t preamble_bytes = (total_bits + 7) / 8;
        if (buf.size() < preamble_bytes) return result<void>::err(error_code::buffer_overflow);

        uint8_t* out = const_cast<uint8_t*>(buf.data());
        for (size_t i = 0; i < preamble_bytes; ++i) {
            uint8_t byte_val = 0;
            for (size_t b = 0; b < 8 && (i * 8 + b) < total_bits; ++b) {
                const size_t bit_idx = i * 8 + b;
                bool bit_val = false;

                if constexpr (OerMeta::has_extension) {
                    if (bit_idx == 0) {
                        bit_val = false; // not using extension
                    } else {
                        // optional bits follow the extension bit
                        const size_t opt_idx = bit_idx - 1;
                        if (opt_idx < OerMeta::optional_count && optional_present)
                            bit_val = optional_present[opt_idx];
                    }
                } else {
                    // No extension: all bits are optional bits
                    const size_t opt_idx = bit_idx;
                    if (opt_idx < OerMeta::optional_count && optional_present)
                        bit_val = optional_present[opt_idx];
                }

                if (bit_val) {
                    byte_val |= (1U << (7 - b)); // MSB first
                }
            }
            out[i] = byte_val;
        }
        buf = buf.subview(preamble_bytes, buf.size() - preamble_bytes);
        return result<void>::ok();
    }
    return result<void>::ok();
}

// ============================================================================
// Non-template member functions (inline to avoid multiple definition)
// ============================================================================

inline result<void> oer_encoder::encode_boolean(bool value, buffer_view& buf) {
    if (buf.size() < 1) return result<void>::err(error_code::buffer_overflow);
    write_be_8(buf, value ? 0xFF : 0x00);
    return result<void>::ok();
}

inline result<void> oer_encoder::encode_null(buffer_view& buf) {
    (void)buf; // 0 octets — nothing written
    return result<void>::ok();
}

inline result<void> oer_encoder::encode_sequence_end(buffer_view& buf) {
    (void)buf;
    return result<void>::ok();
}

inline result<void> oer_encoder::encode_oid(std::span<const uint8_t> encoded_oid, buffer_view& buf) {
    auto r = encode_length_determinant(encoded_oid.size(), buf);
    if (r.is_err()) return r;
    if (buf.size() < encoded_oid.size()) return result<void>::err(error_code::buffer_overflow);
    write_octets_to_buf(buf, encoded_oid.data(), encoded_oid.size());
    return result<void>::ok();
}

inline result<void> oer_encoder::encode_length_determinant(size_t length, buffer_view& buf) {
    if (length <= 127) {
        // Short form: single octet
        if (buf.size() < 1) return result<void>::err(error_code::buffer_overflow);
        write_be_8(buf, static_cast<uint8_t>(length));
    } else if (length <= 16383) {
        // Two-byte form: 10xxxxxx xxxxxxxx
        if (buf.size() < 2) return result<void>::err(error_code::buffer_overflow);
        uint8_t* out = const_cast<uint8_t*>(buf.data());
        out[0] = 0x80 | static_cast<uint8_t>((length >> 8) & 0x3F);
        out[1] = static_cast<uint8_t>(length & 0xFF);
        buf = buf.subview(2, buf.size() - 2);
    } else {
        // Long form: 0xC0 + 16-bit count
        if (buf.size() < 3) return result<void>::err(error_code::buffer_overflow);
        if (length > 65535) return result<void>::err(error_code::value_out_of_range);
        uint8_t* out = const_cast<uint8_t*>(buf.data());
        out[0] = 0xC0;
        out[1] = static_cast<uint8_t>((length >> 8) & 0xFF);
        out[2] = static_cast<uint8_t>(length & 0xFF);
        buf = buf.subview(3, buf.size() - 3);
    }
    return result<void>::ok();
}

} // namespace asn1pp::oer
