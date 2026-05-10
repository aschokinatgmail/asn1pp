#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <type_traits>
#include <vector>
#include <utility>

#include "codec/result.hpp"
#include "buffer/buffer_view.hpp"
#include "buffer/bit_ops.hpp"

namespace asn1pp::per {

/// PER ALIGNED decoder per ITU-T X.691.
///
/// Exact inverse of per_aligned_encoder. Decodes PER-aligned byte sequences
/// back into ASN.1 values. All PER metadata (constraints, extension markers,
/// optional fields) comes from constexpr PerMeta template parameters.
///
/// Stateless per-call; bit_offset_ tracks current bit position in the input
/// buffer.
class per_aligned_decoder {
public:
    per_aligned_decoder() = default;

    // ── Primitive type decoders ────────────────────────────────────────

    /// Decode INTEGER using constraint metadata.
    template<typename PerMeta>
    result<int64_t> decode_integer(const buffer_view& buf);

    /// Decode BOOLEAN — exactly 1 bit, then aligned.
    result<bool> decode_boolean(const buffer_view& buf);

    /// Decode NULL — 0 bits (nothing read).
    result<void> decode_null(const buffer_view& buf);

    /// Decode OCTET STRING using size constraint metadata.
    template<typename PerMeta>
    result<std::vector<uint8_t>> decode_octet_string(const buffer_view& buf);

    /// Decode BIT STRING using size constraint metadata.
    /// Returns data bytes + unused_bits count.
    template<typename PerMeta>
    result<std::pair<std::vector<uint8_t>, uint8_t>> decode_bit_string(const buffer_view& buf);

    /// Decode ENUMERATED — returns index within root values.
    template<typename PerMeta>
    result<int64_t> decode_enumerated(const buffer_view& buf);

    // ── Constructed type helpers ───────────────────────────────────────

    /// Start a SEQUENCE: reads extension bit (if extensible) then
    /// optional-field presence bitmap.
    /// @return decoded extension bit value (true if extension chosen)
    template<typename PerMeta>
    result<bool> decode_sequence_start(const buffer_view& buf,
                                       bool* optional_present = nullptr,
                                       size_t optional_count = 0);

    /// No-op for ALIGNED PER.
    result<void> decode_sequence_end(const buffer_view& buf);

    /// Decode CHOICE index.
    template<typename PerMeta>
    result<int64_t> decode_choice_index(const buffer_view& buf);

    /// Decode SEQUENCE OF / SET OF length determinant.
    template<typename PerMeta>
    result<size_t> decode_sequence_of_length(const buffer_view& buf);

    /// Decode OBJECT IDENTIFIER (returns pre-encoded subidentifier form).
    result<std::vector<uint8_t>> decode_oid(const buffer_view& buf);

    // ── Low-level helpers (public for testing) ─────────────────────────

    /// Decode constrained whole number per X.691 §12.2.
    static result<int64_t> decode_constrained_whole_number(
        int64_t min, int64_t max,
        const buffer_view& buf, size_t& bit_offset);

    /// Decode length determinant per X.691 §21.
    static result<size_t> decode_length_determinant(
        const buffer_view& buf, size_t& bit_offset);

    /// Align current bit position to next octet boundary.
    static void align(size_t& bit_offset);

    /// Calculate ceil(log2(n + 1)) — bits needed to represent 0..n.
    static constexpr size_t bits_needed_unsigned(size_t range);

    /// Calculate ceil(log2(range)) where range = max - min + 1.
    static constexpr size_t bits_needed_for_range(int64_t min, int64_t max);

    /// Current bit offset in the input buffer.
    [[nodiscard]] size_t bit_offset() const noexcept { return bit_offset_; }

    /// Set bit offset.
    void set_bit_offset(size_t offset) noexcept { bit_offset_ = offset; }

private:
    size_t bit_offset_ = 0;
};

// ── inline constexpr helpers ───────────────────────────────────────────────

inline constexpr size_t per_aligned_decoder::bits_needed_unsigned(size_t range) {
    if (range == 0) return 0;
    size_t bits = 0;
    while (range > 0) {
        range >>= 1;
        ++bits;
    }
    return bits;
}

inline constexpr size_t per_aligned_decoder::bits_needed_for_range(
    int64_t min, int64_t max) {
    if (min > max) return 0;
    uint64_t range_u = static_cast<uint64_t>(max - min) + 1;
    return bits_needed_unsigned(range_u - 1);
}

inline void per_aligned_decoder::align(size_t& bit_offset) {
    align_to_octet(bit_offset);
}

// ── Constrained whole number (X.691 §12.2) ─────────────────────────────────

inline result<int64_t> per_aligned_decoder::decode_constrained_whole_number(
    int64_t min, int64_t max,
    const buffer_view& buf, size_t& bit_offset) {
    uint64_t range = static_cast<uint64_t>(max - min) + 1;

    if (range <= 1) {
        return result<int64_t>::ok(min);
    }

    const uint8_t* data = buf.data();

    if (range <= 256) {
        size_t bits = bits_needed_for_range(min, max);
        align(bit_offset);
        if (bit_offset + bits > buf.size() * 8) {
            return result<int64_t>::err(error_code::buffer_underflow);
        }
        uint64_t val = read_bits(data, bit_offset, bits);
        align(bit_offset);
        return result<int64_t>::ok(min + static_cast<int64_t>(val));
    }

    if (range <= 65536) {
        align(bit_offset);
        size_t bits = bits_needed_for_range(min, max);
        if (bit_offset + bits > buf.size() * 8) {
            return result<int64_t>::err(error_code::buffer_underflow);
        }
        uint64_t val = read_bits(data, bit_offset, bits);
        align(bit_offset);
        return result<int64_t>::ok(min + static_cast<int64_t>(val));
    }

    // Large range → length determinant + value bytes, aligned
    align(bit_offset);
    auto ld = decode_length_determinant(buf, bit_offset);
    if (ld.is_err()) return result<int64_t>::err(ld.error());
    size_t val_bytes = ld.value();
    if (bit_offset / 8 + val_bytes > buf.size()) {
        return result<int64_t>::err(error_code::buffer_underflow);
    }
    uint64_t uv = 0;
    for (size_t i = 0; i < val_bytes; ++i) {
        size_t byte_off = bit_offset / 8;
        uv = (uv << 8) | data[byte_off];
        bit_offset += 8;
    }
    return result<int64_t>::ok(min + static_cast<int64_t>(uv));
}

// ── Length determinant (X.691 §21) ─────────────────────────────────────────

inline result<size_t> per_aligned_decoder::decode_length_determinant(
    const buffer_view& buf, size_t& bit_offset) {
    const uint8_t* data = buf.data();
    align(bit_offset);
    if (bit_offset / 8 >= buf.size()) {
        return result<size_t>::err(error_code::buffer_underflow);
    }
    size_t byte_off = bit_offset / 8;
    uint8_t first_byte = data[byte_off];
    bit_offset += 8;

    if ((first_byte & 0x80) == 0) {
        // 0xxxxxxx: single octet, value = first_byte
        return result<size_t>::ok(first_byte);
    }

    if ((first_byte & 0xC0) == 0x80) {
        // 10xxxxxx xxxxxxxx: two octets
        if (byte_off + 1 >= buf.size()) {
            return result<size_t>::err(error_code::buffer_underflow);
        }
        uint8_t second_byte = data[byte_off + 1];
        bit_offset += 8;
        size_t value = ((static_cast<size_t>(first_byte) & 0x3F) << 8) | second_byte;
        return result<size_t>::ok(value);
    }

    // 110xxxxx ...: multi-byte (4-byte form or fragmentation)
    // Check for 4-byte form: 110xxxxx xxxxxxxx xxxxxxxx xxxxxxxx
    if ((first_byte & 0xE0) == 0xC0) {
        size_t remaining = buf.size() - (bit_offset / 8);
        if (remaining < 3) {
            return result<size_t>::err(error_code::buffer_underflow);
        }
        size_t byte_off_after = bit_offset / 8;
        size_t value = ((static_cast<size_t>(first_byte) & 0x1F) << 24) |
                        (static_cast<size_t>(data[byte_off_after]) << 16) |
                        (static_cast<size_t>(data[byte_off_after + 1]) << 8) |
                        static_cast<size_t>(data[byte_off_after + 2]);
        bit_offset += 24;
        return result<size_t>::ok(value);
    }

    return result<size_t>::err(error_code::parse_error);
}

// ── Template member function definitions ───────────────────────────────────

template<typename PerMeta>
inline result<int64_t> per_aligned_decoder::decode_integer(const buffer_view& buf) {
    if constexpr (PerMeta::has_range_constraint) {
        return decode_constrained_whole_number(
            PerMeta::min_value, PerMeta::max_value, buf, bit_offset_);
    }
    // Unconstrained: length determinant + two's complement value bytes
    align(bit_offset_);
    auto ld = decode_length_determinant(buf, bit_offset_);
    if (ld.is_err()) return result<int64_t>::err(ld.error());
    size_t val_bytes = ld.value();
    if (val_bytes == 0) return result<int64_t>::ok(0);
    const uint8_t* data = buf.data();
    if (bit_offset_ / 8 + val_bytes > buf.size()) {
        return result<int64_t>::err(error_code::buffer_underflow);
    }
    // Read value bytes big-endian, sign-extend
    int64_t result_val = 0;
    bool negative = false;
    size_t byte_off = bit_offset_ / 8;
    if (val_bytes > 0 && (data[byte_off] & 0x80)) {
        negative = true;
        result_val = -1;  // sign extension fill
    }
    for (size_t i = 0; i < val_bytes; ++i) {
        result_val = (result_val << 8) | data[byte_off + i];
    }
    bit_offset_ += val_bytes * 8;
    (void)negative;
    return result<int64_t>::ok(result_val);
}

inline result<bool> per_aligned_decoder::decode_boolean(const buffer_view& buf) {
    const uint8_t* data = buf.data();
    if (bit_offset_ + 1 > buf.size() * 8) {
        return result<bool>::err(error_code::buffer_underflow);
    }
    uint64_t bit = read_bits(data, bit_offset_, 1);
    align(bit_offset_);
    return result<bool>::ok(bit != 0);
}

inline result<void> per_aligned_decoder::decode_null(const buffer_view& /* buf */) {
    return result<void>::ok();
}

template<typename PerMeta>
inline result<std::vector<uint8_t>> per_aligned_decoder::decode_octet_string(const buffer_view& buf) {
    const uint8_t* data = buf.data();

    if constexpr (PerMeta::has_size_constraint) {
        if constexpr (PerMeta::min_size == PerMeta::max_size) {
            align(bit_offset_);
            if (bit_offset_ / 8 + PerMeta::min_size > buf.size()) {
                return result<std::vector<uint8_t>>::err(error_code::buffer_underflow);
            }
            size_t byte_off = bit_offset_ / 8;
            std::vector<uint8_t> out(data + byte_off, data + byte_off + PerMeta::min_size);
            bit_offset_ = (byte_off + PerMeta::min_size) * 8;
            return result<std::vector<uint8_t>>::ok(std::move(out));
        }
        // Constrained: length prefix + data
        uint64_t range = PerMeta::max_size - PerMeta::min_size + 1;
        if (range <= 65536) {
            align(bit_offset_);
            size_t len_bits = bits_needed_unsigned(range - 1);
            if (bit_offset_ + len_bits > buf.size() * 8) {
                return result<std::vector<uint8_t>>::err(error_code::buffer_underflow);
            }
            size_t data_len = static_cast<size_t>(read_bits(data, bit_offset_, len_bits));
            align(bit_offset_);
            if (bit_offset_ / 8 + data_len > buf.size()) {
                return result<std::vector<uint8_t>>::err(error_code::buffer_underflow);
            }
            size_t byte_off = bit_offset_ / 8;
            std::vector<uint8_t> out(data + byte_off, data + byte_off + data_len);
            bit_offset_ = (byte_off + data_len) * 8;
            return result<std::vector<uint8_t>>::ok(std::move(out));
        }
    }
    // Unconstrained: length determinant + data
    align(bit_offset_);
    auto ld = decode_length_determinant(buf, bit_offset_);
    if (ld.is_err()) return result<std::vector<uint8_t>>::err(ld.error());
    size_t data_len = ld.value();
    if (bit_offset_ / 8 + data_len > buf.size()) {
        return result<std::vector<uint8_t>>::err(error_code::buffer_underflow);
    }
    size_t byte_off = bit_offset_ / 8;
    std::vector<uint8_t> out(data + byte_off, data + byte_off + data_len);
    bit_offset_ = (byte_off + data_len) * 8;
    return result<std::vector<uint8_t>>::ok(std::move(out));
}

template<typename PerMeta>
inline result<std::pair<std::vector<uint8_t>, uint8_t>> per_aligned_decoder::decode_bit_string(const buffer_view& buf) {
    const uint8_t* data = buf.data();

    if constexpr (PerMeta::has_size_constraint) {
        if constexpr (PerMeta::min_size == PerMeta::max_size) {
            align(bit_offset_);
            size_t byte_len = (PerMeta::min_size + 7) / 8;
            size_t unused = static_cast<uint8_t>(byte_len * 8 - PerMeta::min_size);
            if (bit_offset_ / 8 + byte_len > buf.size()) {
                return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::buffer_underflow);
            }
            size_t byte_off = bit_offset_ / 8;
            std::vector<uint8_t> out(data + byte_off, data + byte_off + byte_len);
            bit_offset_ = (byte_off + byte_len) * 8;
            return result<std::pair<std::vector<uint8_t>, uint8_t>>::ok(
                std::make_pair(std::move(out), unused));
        }
        // Constrained
        uint64_t range = PerMeta::max_size - PerMeta::min_size + 1;
        if (range <= 65536) {
            align(bit_offset_);
            size_t len_bits = bits_needed_unsigned(range - 1);
            if (bit_offset_ + len_bits > buf.size() * 8) {
                return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::buffer_underflow);
            }
            size_t bit_len = static_cast<size_t>(read_bits(data, bit_offset_, len_bits));
            align(bit_offset_);
            size_t byte_len = (bit_len + 7) / 8;
            size_t unused = static_cast<uint8_t>(byte_len * 8 - bit_len);
            if (bit_offset_ / 8 + byte_len > buf.size()) {
                return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::buffer_underflow);
            }
            size_t byte_off = bit_offset_ / 8;
            std::vector<uint8_t> out(data + byte_off, data + byte_off + byte_len);
            bit_offset_ = (byte_off + byte_len) * 8;
            return result<std::pair<std::vector<uint8_t>, uint8_t>>::ok(
                std::make_pair(std::move(out), unused));
        }
    }
    // Unconstrained
    align(bit_offset_);
    auto ld = decode_length_determinant(buf, bit_offset_);
    if (ld.is_err()) return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(ld.error());
    size_t bit_len = ld.value();
    // Read unused bits byte
    if (bit_offset_ / 8 >= buf.size()) {
        return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::buffer_underflow);
    }
    size_t unused_byte_off = bit_offset_ / 8;
    uint8_t unused = data[unused_byte_off];
    bit_offset_ += 8;
    // Read data
    size_t byte_len = (bit_len + 7) / 8;
    if (bit_offset_ / 8 + byte_len > buf.size()) {
        return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::buffer_underflow);
    }
    size_t data_byte_off = bit_offset_ / 8;
    std::vector<uint8_t> out(data + data_byte_off, data + data_byte_off + byte_len);
    bit_offset_ = (data_byte_off + byte_len) * 8;
    return result<std::pair<std::vector<uint8_t>, uint8_t>>::ok(
        std::make_pair(std::move(out), unused));
}

template<typename PerMeta>
inline result<int64_t> per_aligned_decoder::decode_enumerated(const buffer_view& buf) {
    if (PerMeta::normal_index_count <= 1) {
        return result<int64_t>::ok(0);
    }
    size_t bits = bits_needed_unsigned(PerMeta::normal_index_count - 1);
    const uint8_t* data = buf.data();
    if (bit_offset_ + bits > buf.size() * 8) {
        return result<int64_t>::err(error_code::buffer_underflow);
    }
    uint64_t idx = read_bits(data, bit_offset_, bits);
    align(bit_offset_);
    if (idx >= PerMeta::normal_index_count) {
        return result<int64_t>::err(error_code::value_out_of_range);
    }
    return result<int64_t>::ok(static_cast<int64_t>(idx));
}

template<typename PerMeta>
inline result<bool> per_aligned_decoder::decode_sequence_start(
    const buffer_view& buf, bool* optional_present, size_t optional_count) {
    const uint8_t* data = buf.data();
    bool extension_chosen = false;

    if constexpr (PerMeta::has_extension) {
        if (bit_offset_ + 1 > buf.size() * 8) {
            return result<bool>::err(error_code::buffer_underflow);
        }
        uint64_t ext_bit = read_bits(data, bit_offset_, 1);
        extension_chosen = (ext_bit != 0);
    }

    if (optional_count > 0 && optional_present != nullptr) {
        for (size_t i = 0; i < optional_count; ++i) {
            if (bit_offset_ + 1 > buf.size() * 8) {
                return result<bool>::err(error_code::buffer_underflow);
            }
            uint64_t pres = read_bits(data, bit_offset_, 1);
            optional_present[i] = (pres != 0);
        }
    }

    if (bit_offset_ > 0) {
        align(bit_offset_);
    }
    return result<bool>::ok(extension_chosen);
}

inline result<void> per_aligned_decoder::decode_sequence_end(const buffer_view& /* buf */) {
    return result<void>::ok();
}

template<typename PerMeta>
inline result<int64_t> per_aligned_decoder::decode_choice_index(const buffer_view& buf) {
    size_t bits = bits_needed_unsigned(PerMeta::alternative_count - 1);
    const uint8_t* data = buf.data();
    if (bit_offset_ + bits > buf.size() * 8) {
        return result<int64_t>::err(error_code::buffer_underflow);
    }
    uint64_t idx = read_bits(data, bit_offset_, bits);
    align(bit_offset_);
    if (idx >= PerMeta::alternative_count) {
        return result<int64_t>::err(error_code::value_out_of_range);
    }
    return result<int64_t>::ok(static_cast<int64_t>(idx));
}

template<typename PerMeta>
inline result<size_t> per_aligned_decoder::decode_sequence_of_length(const buffer_view& buf) {
    if constexpr (PerMeta::has_size_constraint) {
        uint64_t range = PerMeta::max_size - PerMeta::min_size + 1;
        if (range <= 65536) {
            size_t len_bits = bits_needed_unsigned(range - 1);
            align(bit_offset_);
            const uint8_t* data = buf.data();
            if (bit_offset_ + len_bits > buf.size() * 8) {
                return result<size_t>::err(error_code::buffer_underflow);
            }
            size_t len = static_cast<size_t>(read_bits(data, bit_offset_, len_bits));
            align(bit_offset_);
            if (len < PerMeta::min_size || len > PerMeta::max_size) {
                return result<size_t>::err(error_code::value_out_of_range);
            }
            return result<size_t>::ok(len);
        }
    }
    // Unconstrained
    align(bit_offset_);
    return decode_length_determinant(buf, bit_offset_);
}

inline result<std::vector<uint8_t>> per_aligned_decoder::decode_oid(const buffer_view& buf) {
    align(bit_offset_);
    auto ld = decode_length_determinant(buf, bit_offset_);
    if (ld.is_err()) return result<std::vector<uint8_t>>::err(ld.error());
    size_t data_len = ld.value();
    const uint8_t* data = buf.data();
    if (bit_offset_ / 8 + data_len > buf.size()) {
        return result<std::vector<uint8_t>>::err(error_code::buffer_underflow);
    }
    size_t byte_off = bit_offset_ / 8;
    std::vector<uint8_t> out(data + byte_off, data + byte_off + data_len);
    bit_offset_ = (byte_off + data_len) * 8;
    return result<std::vector<uint8_t>>::ok(std::move(out));
}

} // namespace asn1pp::per
