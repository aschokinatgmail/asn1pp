#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <vector>
#include <utility>

#include "codec/result.hpp"
#include "codec/arch_codec.hpp"
#include "codec/batch_buffer.hpp"
#include "buffer/buffer_view.hpp"
#include "buffer/bit_ops.hpp"

namespace asn1pp::per {

/// PER UNALIGNED (UPER) decoder per ITU-T X.691.
///
/// Exact inverse of uper_encoder. Reads bits continuously WITHOUT
/// inter-field octet alignment. At the end of a complete PDU, flush()
/// verifies remaining padding bits are zeros.
///
/// Stateless per-call; bit_offset_ tracks current bit position in the input
/// buffer. All PER metadata (constraints, extension markers, optional fields)
/// comes from constexpr PerMeta template parameters emitted by the code
/// generator.
class uper_decoder {
public:
    uper_decoder() = default;

    // ── Primitive type decoders ────────────────────────────────────────

    /// Decode INTEGER using constraint metadata.
    template<typename PerMeta>
    result<int64_t> decode_integer(const buffer_view& buf);

    /// Decode BOOLEAN — exactly 1 bit. NO alignment after.
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
    /// optional-field presence bitmap. NO alignment.
    /// @return decoded extension bit value (true if extension chosen)
    template<typename PerMeta>
    result<bool> decode_sequence_start(const buffer_view& buf,
                                       bool* optional_present = nullptr,
                                       size_t optional_count = 0);

    /// No-op for UPER (alignment is done only at final flush).
    result<void> decode_sequence_end(const buffer_view& buf);

    /// Decode CHOICE index. NO alignment.
    template<typename PerMeta>
    result<int64_t> decode_choice_index(const buffer_view& buf);

    /// Decode SEQUENCE OF / SET OF length determinant.
    template<typename PerMeta>
    result<size_t> decode_sequence_of_length(const buffer_view& buf);

    /// Decode OBJECT IDENTIFIER (returns pre-encoded subidentifier form).
    result<std::vector<uint8_t>> decode_oid(const buffer_view& buf);

    // ── Batch drain ────────────────────────────────────────────────────

    error_code flush_decode() noexcept;
    static size_t batch_size() noexcept;

    // ── Low-level helpers (public for testing) ─────────────────────────

    /// Decode constrained whole number per X.691 §12.2. NO alignment.
    static result<int64_t> decode_constrained_whole_number_unaligned(
        int64_t min, int64_t max, const buffer_view& buf, size_t& bit_offset);

    /// Decode length determinant per X.691 §21. NO alignment.
    static result<size_t> decode_length_determinant_unaligned(
        const buffer_view& buf, size_t& bit_offset);

    /// Flush at end: verify remaining bits are 0 padding.
    result<void> flush(const buffer_view& buf);

    /// Calculate ceil(log2(n + 1)) — bits needed to represent 0..n.
    static constexpr size_t bits_needed_unsigned(size_t range);

    /// Calculate ceil(log2(range)) where range = max - min + 1.
    static constexpr size_t bits_needed_for_range(int64_t min, int64_t max);

    /// Current bit offset in the input buffer.
    [[nodiscard]] size_t bit_offset() const noexcept { return bit_offset_; }

    /// Set bit offset.
    void set_bit_offset(size_t offset) noexcept { bit_offset_ = offset; }

private:
    /// Read raw bytes at an arbitrary bit position using read_bits.
    static void read_octets_unaligned(const uint8_t* buf, size_t& bit_offset,
                                      uint8_t* dst, size_t count);

    size_t bit_offset_ = 0;

    static constexpr size_t kMaxBatchSize = 4;

    struct pending_integer {
        uint8_t value_buf[8];
        size_t size;
    };

    batch_buffer<pending_integer, kMaxBatchSize> pending_;
};

// ── inline constexpr helpers ───────────────────────────────────────────────

inline constexpr size_t uper_decoder::bits_needed_unsigned(size_t range) {
    if (range == 0) return 0;
    size_t bits = 0;
    while (range > 0) {
        range >>= 1;
        ++bits;
    }
    return bits;
}

inline constexpr size_t uper_decoder::bits_needed_for_range(
    int64_t min, int64_t max) {
    if (min > max) return 0;
    uint64_t range_u = static_cast<uint64_t>(max - min) + 1;
    return bits_needed_unsigned(range_u - 1);
}

// ── Private helper: read octets at non-byte-aligned positions ──────────────

inline void uper_decoder::read_octets_unaligned(const uint8_t* buf, size_t& bit_offset,
                                                 uint8_t* dst, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        dst[i] = static_cast<uint8_t>(read_bits(buf, bit_offset, 8));
    }
}

// ── Constrained whole number (X.691 §12.2) UNALIGNED ───────────────────────

inline result<int64_t> uper_decoder::decode_constrained_whole_number_unaligned(
    int64_t min, int64_t max, const buffer_view& buf, size_t& bit_offset) {
    uint64_t range = static_cast<uint64_t>(max - min) + 1;

    if (range <= 1) {
        return result<int64_t>::ok(min);
    }

    const uint8_t* data = buf.data();

    if (range <= 256) {
        size_t bits = bits_needed_for_range(min, max);
        if (bit_offset + bits > buf.size() * 8) {
            return result<int64_t>::err(error_code::buffer_underflow);
        }
        uint64_t val = read_bits(data, bit_offset, bits);
        return result<int64_t>::ok(min + static_cast<int64_t>(val));
    }

    if (range <= 65536) {
        size_t bits = bits_needed_for_range(min, max);
        if (bit_offset + bits > buf.size() * 8) {
            return result<int64_t>::err(error_code::buffer_underflow);
        }
        uint64_t val = read_bits(data, bit_offset, bits);
        return result<int64_t>::ok(min + static_cast<int64_t>(val));
    }

    // Large range → length determinant + value bytes, NO alignment
    auto ld = decode_length_determinant_unaligned(buf, bit_offset);
    if (ld.is_err()) return result<int64_t>::err(ld.error());
    size_t val_bytes = ld.value();
    if (val_bytes == 0) return result<int64_t>::ok(min);
    if (bit_offset + val_bytes * 8 > buf.size() * 8) {
        return result<int64_t>::err(error_code::buffer_underflow);
    }
    uint64_t uv = 0;
    for (size_t i = 0; i < val_bytes; ++i) {
        uv = (uv << 8) | read_bits(data, bit_offset, 8);
    }
    return result<int64_t>::ok(min + static_cast<int64_t>(uv));
}

// ── Length determinant (X.691 §21) UNALIGNED ───────────────────────────────

inline result<size_t> uper_decoder::decode_length_determinant_unaligned(
    const buffer_view& buf, size_t& bit_offset) {
    const uint8_t* data = buf.data();

    // Read first 8 bits at current bit position (NOT byte-aligned)
    if (bit_offset + 8 > buf.size() * 8) {
        return result<size_t>::err(error_code::buffer_underflow);
    }
    uint8_t first_byte = static_cast<uint8_t>(read_bits(data, bit_offset, 8));

    if ((first_byte & 0x80) == 0) {
        // 0xxxxxxx: single octet, value = first_byte
        return result<size_t>::ok(first_byte);
    }

    if ((first_byte & 0xC0) == 0x80) {
        // 10xxxxxx xxxxxxxx: two octets
        if (bit_offset + 8 > buf.size() * 8) {
            return result<size_t>::err(error_code::buffer_underflow);
        }
        uint8_t second_byte = static_cast<uint8_t>(read_bits(data, bit_offset, 8));
        size_t value = ((static_cast<size_t>(first_byte) & 0x3F) << 8) | second_byte;
        return result<size_t>::ok(value);
    }

    // 110xxxxx xxxxxxxx xxxxxxxx xxxxxxxx: 4-byte form
    if ((first_byte & 0xE0) == 0xC0) {
        if (bit_offset + 24 > buf.size() * 8) {
            return result<size_t>::err(error_code::buffer_underflow);
        }
        uint8_t b2 = static_cast<uint8_t>(read_bits(data, bit_offset, 8));
        uint8_t b3 = static_cast<uint8_t>(read_bits(data, bit_offset, 8));
        uint8_t b4 = static_cast<uint8_t>(read_bits(data, bit_offset, 8));
        size_t value = ((static_cast<size_t>(first_byte) & 0x1F) << 24) |
                        (static_cast<size_t>(b2) << 16) |
                        (static_cast<size_t>(b3) << 8) |
                        static_cast<size_t>(b4);
        return result<size_t>::ok(value);
    }

    return result<size_t>::err(error_code::parse_error);
}

// ── Flush (verify padding zeros) ───────────────────────────────────────────

inline result<void> uper_decoder::flush(const buffer_view& buf) {
    const uint8_t* data = buf.data();
    size_t total_bits = buf.size() * 8;
    if (bit_offset_ >= total_bits) return result<void>::ok();
    size_t remaining = total_bits - bit_offset_;
    if (remaining < 8) {
        uint64_t val = read_bits(data, bit_offset_, remaining);
        if (val != 0) return result<void>::err(error_code::parse_error);
    } else {
        // Check each remaining byte
        while (bit_offset_ + 8 <= total_bits) {
            uint64_t byte_val = read_bits(data, bit_offset_, 8);
            if (byte_val != 0) return result<void>::err(error_code::parse_error);
        }
        if (bit_offset_ < total_bits) {
            size_t tail = total_bits - bit_offset_;
            uint64_t val = read_bits(data, bit_offset_, tail);
            if (val != 0) return result<void>::err(error_code::parse_error);
        }
    }
    bit_offset_ = total_bits;
    return result<void>::ok();
}

// ── Template member function definitions ───────────────────────────────────

template<typename PerMeta>
inline result<int64_t> uper_decoder::decode_integer(const buffer_view& buf) {
    if constexpr (PerMeta::has_range_constraint) {
        return decode_constrained_whole_number_unaligned(
            PerMeta::min_value, PerMeta::max_value, buf, bit_offset_);
    }
    // Unconstrained: length determinant + two's complement value bytes. NO alignment.
    auto ld = decode_length_determinant_unaligned(buf, bit_offset_);
    if (ld.is_err()) return result<int64_t>::err(ld.error());
    size_t val_bytes = ld.value();
    if (val_bytes == 0) return result<int64_t>::ok(0);
    const uint8_t* data = buf.data();
    if (bit_offset_ + val_bytes * 8 > buf.size() * 8) {
        return result<int64_t>::err(error_code::buffer_underflow);
    }
    if (val_bytes > 8) {
        return result<int64_t>::err(error_code::value_out_of_range);
    }

    const size_t bs = batch_size();

    pending_integer slot = {};
    for (size_t i = 0; i < val_bytes; ++i) {
        slot.value_buf[i] = static_cast<uint8_t>(read_bits(data, bit_offset_, 8));
    }
    slot.size = val_bytes;
    pending_.push(slot);

    if (pending_.size() == bs) {
        auto drained = pending_.drain();
        const uint8_t* ptrs[kMaxBatchSize];
        size_t sizes[kMaxBatchSize];
        int64_t results[kMaxBatchSize];
        error_code errors[kMaxBatchSize];

        for (size_t i = 0; i < bs; ++i) {
            ptrs[i] = drained[i].value_buf;
            sizes[i] = drained[i].size;
        }

        arch_codec::batch_decode_integers(ptrs, sizes, results, errors, bs);

        if (errors[bs - 1] != error_code::ok)
            return result<int64_t>::err(errors[bs - 1]);
        return result<int64_t>::ok(results[bs - 1]);
    }

    int64_t result_val = 0;
    const bool negative = (slot.value_buf[0] & 0x80) != 0;
    if (negative) result_val = -1;
    for (size_t i = 0; i < val_bytes; ++i) {
        result_val = (result_val << 8) | static_cast<int64_t>(slot.value_buf[i]);
    }
    return result<int64_t>::ok(result_val);
}

inline result<bool> uper_decoder::decode_boolean(const buffer_view& buf) {
    const uint8_t* data = buf.data();
    if (bit_offset_ + 1 > buf.size() * 8) {
        return result<bool>::err(error_code::buffer_underflow);
    }
    uint64_t bit = read_bits(data, bit_offset_, 1);
    // NO alignment after boolean in UPER
    return result<bool>::ok(bit != 0);
}

inline result<void> uper_decoder::decode_null(const buffer_view& /* buf */) {
    return result<void>::ok();
}

template<typename PerMeta>
inline result<std::vector<uint8_t>> uper_decoder::decode_octet_string(const buffer_view& buf) {
    const uint8_t* data = buf.data();

    if constexpr (PerMeta::has_size_constraint) {
        if constexpr (PerMeta::min_size == PerMeta::max_size) {
            // Fixed size: read data bytes directly at current bit offset. NO alignment.
            if (bit_offset_ + PerMeta::min_size * 8 > buf.size() * 8) {
                return result<std::vector<uint8_t>>::err(error_code::buffer_underflow);
            }
            std::vector<uint8_t> out(PerMeta::min_size);
            read_octets_unaligned(data, bit_offset_, out.data(), out.size());
            return result<std::vector<uint8_t>>::ok(std::move(out));
        }
        // Constrained: length prefix + data. NO alignment.
        uint64_t range = PerMeta::max_size - PerMeta::min_size + 1;
        if (range <= 65536) {
            size_t len_bits = bits_needed_unsigned(range - 1);
            if (bit_offset_ + len_bits > buf.size() * 8) {
                return result<std::vector<uint8_t>>::err(error_code::buffer_underflow);
            }
            size_t data_len = static_cast<size_t>(read_bits(data, bit_offset_, len_bits));
            if (bit_offset_ + data_len * 8 > buf.size() * 8) {
                return result<std::vector<uint8_t>>::err(error_code::buffer_underflow);
            }
            std::vector<uint8_t> out(data_len);
            read_octets_unaligned(data, bit_offset_, out.data(), out.size());
            return result<std::vector<uint8_t>>::ok(std::move(out));
        }
    }
    // Unconstrained: length determinant + data. NO alignment.
    auto ld = decode_length_determinant_unaligned(buf, bit_offset_);
    if (ld.is_err()) return result<std::vector<uint8_t>>::err(ld.error());
    size_t data_len = ld.value();
    if (bit_offset_ + data_len * 8 > buf.size() * 8) {
        return result<std::vector<uint8_t>>::err(error_code::buffer_underflow);
    }
    std::vector<uint8_t> out(data_len);
    read_octets_unaligned(data, bit_offset_, out.data(), out.size());
    return result<std::vector<uint8_t>>::ok(std::move(out));
}

template<typename PerMeta>
inline result<std::pair<std::vector<uint8_t>, uint8_t>> uper_decoder::decode_bit_string(const buffer_view& buf) {
    const uint8_t* data = buf.data();

    if constexpr (PerMeta::has_size_constraint) {
        if constexpr (PerMeta::min_size == PerMeta::max_size) {
            // Fixed size: read data bytes directly. NO alignment.
            size_t byte_len = (PerMeta::min_size + 7) / 8;
            size_t unused = static_cast<uint8_t>(byte_len * 8 - PerMeta::min_size);
            if (bit_offset_ + byte_len * 8 > buf.size() * 8) {
                return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::buffer_underflow);
            }
            std::vector<uint8_t> out(byte_len);
            read_octets_unaligned(data, bit_offset_, out.data(), out.size());
            return result<std::pair<std::vector<uint8_t>, uint8_t>>::ok(
                std::make_pair(std::move(out), unused));
        }
        // Constrained
        uint64_t range = PerMeta::max_size - PerMeta::min_size + 1;
        if (range <= 65536) {
            size_t len_bits = bits_needed_unsigned(range - 1);
            if (bit_offset_ + len_bits > buf.size() * 8) {
                return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::buffer_underflow);
            }
            size_t bit_len = static_cast<size_t>(read_bits(data, bit_offset_, len_bits));
            size_t byte_len = (bit_len + 7) / 8;
            size_t unused = static_cast<uint8_t>(byte_len * 8 - bit_len);
            if (bit_offset_ + byte_len * 8 > buf.size() * 8) {
                return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::buffer_underflow);
            }
            std::vector<uint8_t> out(byte_len);
            read_octets_unaligned(data, bit_offset_, out.data(), out.size());
            return result<std::pair<std::vector<uint8_t>, uint8_t>>::ok(
                std::make_pair(std::move(out), unused));
        }
    }
    // Unconstrained: length determinant + unused bits byte + data. NO alignment.
    auto ld = decode_length_determinant_unaligned(buf, bit_offset_);
    if (ld.is_err()) return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(ld.error());
    size_t bit_len = ld.value();
    // Read unused bits byte
    if (bit_offset_ + 8 > buf.size() * 8) {
        return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::buffer_underflow);
    }
    uint8_t unused = static_cast<uint8_t>(read_bits(data, bit_offset_, 8));
    // Read data
    size_t byte_len = (bit_len + 7) / 8;
    if (bit_offset_ + byte_len * 8 > buf.size() * 8) {
        return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::buffer_underflow);
    }
    std::vector<uint8_t> out(byte_len);
    read_octets_unaligned(data, bit_offset_, out.data(), out.size());
    return result<std::pair<std::vector<uint8_t>, uint8_t>>::ok(
        std::make_pair(std::move(out), unused));
}

template<typename PerMeta>
inline result<int64_t> uper_decoder::decode_enumerated(const buffer_view& buf) {
    if (PerMeta::normal_index_count <= 1) {
        return result<int64_t>::ok(0);
    }
    size_t bits = bits_needed_unsigned(PerMeta::normal_index_count - 1);
    const uint8_t* data = buf.data();
    if (bit_offset_ + bits > buf.size() * 8) {
        return result<int64_t>::err(error_code::buffer_underflow);
    }
    uint64_t idx = read_bits(data, bit_offset_, bits);
    // NO alignment after in UPER
    if (idx >= PerMeta::normal_index_count) {
        return result<int64_t>::err(error_code::value_out_of_range);
    }
    return result<int64_t>::ok(static_cast<int64_t>(idx));
}

template<typename PerMeta>
inline result<bool> uper_decoder::decode_sequence_start(
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

    // NO alignment in UPER
    return result<bool>::ok(extension_chosen);
}

inline result<void> uper_decoder::decode_sequence_end(const buffer_view& /* buf */) {
    return result<void>::ok();
}

template<typename PerMeta>
inline result<int64_t> uper_decoder::decode_choice_index(const buffer_view& buf) {
    size_t bits = bits_needed_unsigned(PerMeta::alternative_count - 1);
    const uint8_t* data = buf.data();
    if (bit_offset_ + bits > buf.size() * 8) {
        return result<int64_t>::err(error_code::buffer_underflow);
    }
    uint64_t idx = read_bits(data, bit_offset_, bits);
    // NO alignment after in UPER
    if (idx >= PerMeta::alternative_count) {
        return result<int64_t>::err(error_code::value_out_of_range);
    }
    return result<int64_t>::ok(static_cast<int64_t>(idx));
}

template<typename PerMeta>
inline result<size_t> uper_decoder::decode_sequence_of_length(const buffer_view& buf) {
    if constexpr (PerMeta::has_size_constraint) {
        uint64_t range = PerMeta::max_size - PerMeta::min_size + 1;
        if (range <= 65536) {
            size_t len_bits = bits_needed_unsigned(range - 1);
            const uint8_t* data = buf.data();
            if (bit_offset_ + len_bits > buf.size() * 8) {
                return result<size_t>::err(error_code::buffer_underflow);
            }
            size_t len = static_cast<size_t>(read_bits(data, bit_offset_, len_bits));
            // NO alignment after in UPER
            if (len < PerMeta::min_size || len > PerMeta::max_size) {
                return result<size_t>::err(error_code::value_out_of_range);
            }
            return result<size_t>::ok(len);
        }
    }
    // Unconstrained. NO alignment.
    return decode_length_determinant_unaligned(buf, bit_offset_);
}

inline result<std::vector<uint8_t>> uper_decoder::decode_oid(const buffer_view& buf) {
    // NO alignment before length determinant in UPER
    auto ld = decode_length_determinant_unaligned(buf, bit_offset_);
    if (ld.is_err()) return result<std::vector<uint8_t>>::err(ld.error());
    size_t data_len = ld.value();
    const uint8_t* data = buf.data();
    if (bit_offset_ + data_len * 8 > buf.size() * 8) {
        return result<std::vector<uint8_t>>::err(error_code::buffer_underflow);
    }
    std::vector<uint8_t> out(data_len);
    read_octets_unaligned(data, bit_offset_, out.data(), out.size());
    return result<std::vector<uint8_t>>::ok(std::move(out));
}

} // namespace asn1pp::per
