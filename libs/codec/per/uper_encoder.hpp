#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <type_traits>

#include "codec/result.hpp"
#include "buffer/buffer_view.hpp"
#include "buffer/bit_ops.hpp"

namespace asn1pp::per {

/// PER UNALIGNED (UPER) encoder per ITU-T X.691.
///
/// Variant of PER ALIGNED that eliminates all inter-field octet alignment.
/// Bits are packed continuously; final PDU is padded to full octet only
/// at the very end via flush().
///
/// Stateless per-call; bit_offset_ tracks current bit position in the output
/// buffer. All PER metadata (constraints, extension markers, optional fields)
/// comes from constexpr PerMeta template parameters emitted by the code
/// generator.
class uper_encoder {
public:
    uper_encoder() = default;

    // ── Primitive type encoders ────────────────────────────────────────

    /// Encode INTEGER using constraint metadata.
    template<typename PerMeta>
    result<void> encode_integer(int64_t value, buffer_view& buf);

    /// Encode BOOLEAN — exactly 1 bit (0=FALSE, 1=TRUE). NO alignment after.
    result<void> encode_boolean(bool value, buffer_view& buf);

    /// Encode NULL — 0 bits (nothing written).
    result<void> encode_null(buffer_view& buf);

    /// Encode OCTET STRING using size constraint metadata.
    template<typename PerMeta>
    result<void> encode_octet_string(std::span<const uint8_t> data, buffer_view& buf);

    /// Encode BIT STRING using size constraint metadata.
    template<typename PerMeta>
    result<void> encode_bit_string(std::span<const uint8_t> data, uint8_t unused_bits, buffer_view& buf);

    /// Encode ENUMERATED — index within root values.
    template<typename PerMeta>
    result<void> encode_enumerated(int64_t index, buffer_view& buf);

    // ── Constructed type helpers ───────────────────────────────────────

    /// Start a SEQUENCE: writes extension bit (if extensible) then
    /// optional-field presence bitmap. NO alignment.
    template<typename PerMeta>
    result<void> encode_sequence_start(buffer_view& buf,
                                       const bool* optional_present = nullptr,
                                       size_t optional_count = 0);

    /// No-op for UPER (alignment is done only at final flush).
    result<void> encode_sequence_end(buffer_view& buf);

    /// Encode CHOICE index. NO alignment.
    template<typename PerMeta>
    result<void> encode_choice_index(int64_t choice_index, buffer_view& buf);

    /// Encode SEQUENCE OF / SET OF length determinant.
    template<typename PerMeta>
    result<void> encode_sequence_of_length(size_t length, buffer_view& buf);

    /// Encode OBJECT IDENTIFIER (pre-encoded subidentifier form).
    result<void> encode_oid(std::span<const uint8_t> encoded_oid, buffer_view& buf);

    // ── Low-level helpers (public for testing) ─────────────────────────

    /// Encode constrained whole number per X.691 §12.2. NO alignment.
    static result<void> encode_constrained_whole_number_unaligned(
        int64_t value, int64_t min, int64_t max,
        buffer_view& buf, size_t& bit_offset);

    /// Encode length determinant per X.691 §21. NO alignment.
    static result<void> encode_length_determinant_unaligned(
        size_t length, buffer_view& buf, size_t& bit_offset);

    /// Flush: pad current bit position to next octet boundary with zero bits.
    /// Call once at end of complete PDU.
    result<void> flush(buffer_view& buf);

    /// Pad current bit position to next octet boundary (pure bit-position advance).
    static void flush_to_octet(size_t& bit_offset);

    /// Calculate ceil(log2(n + 1)) — bits needed to represent 0..n.
    static constexpr size_t bits_needed_unsigned(size_t range);

    /// Calculate ceil(log2(range)) where range = max - min + 1.
    static constexpr size_t bits_needed_for_range(int64_t min, int64_t max);

    /// Current bit offset in the output buffer.
    [[nodiscard]] size_t bit_offset() const noexcept { return bit_offset_; }

    /// Set bit offset (used after encoding into the buffer to track position).
    void set_bit_offset(size_t offset) noexcept { bit_offset_ = offset; }

private:
    /// Write raw bytes at an arbitrary bit position using write_bits.
    static void write_octets_unaligned(uint8_t* buf, size_t& bit_offset,
                                       const uint8_t* src, size_t count);

    size_t bit_offset_ = 0;
};

// ── inline constexpr helpers ───────────────────────────────────────────────

inline constexpr size_t uper_encoder::bits_needed_unsigned(size_t range) {
    if (range == 0) return 0;
    size_t bits = 0;
    while (range > 0) {
        range >>= 1;
        ++bits;
    }
    return bits;
}

inline constexpr size_t uper_encoder::bits_needed_for_range(
    int64_t min, int64_t max) {
    if (min > max) return 0;
    uint64_t range_u = static_cast<uint64_t>(max - min) + 1;
    return bits_needed_unsigned(range_u - 1);
}

// ── Private helper ─────────────────────────────────────────────────────────

inline void uper_encoder::write_octets_unaligned(uint8_t* buf, size_t& bit_offset,
                                                  const uint8_t* src, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        write_bits(buf, bit_offset, src[i], 8);
    }
}

// ── flush_to_octet (static helper) ────────────────────────────────────────

inline void uper_encoder::flush_to_octet(size_t& bit_offset) {
    align_to_octet(bit_offset);
}

// ── Flush (final padding) ──────────────────────────────────────────────────

inline result<void> uper_encoder::flush(buffer_view& buf) {
    (void)buf;
    flush_to_octet(bit_offset_);
    return result<void>::ok();
}

// ── Constrained whole number (X.691 §12.2) UNALIGNED ───────────────────────

inline result<void> uper_encoder::encode_constrained_whole_number_unaligned(
    int64_t value, int64_t min, int64_t max,
    buffer_view& buf, size_t& bit_offset) {
    if (value < min || value > max) {
        return result<void>::err(error_code::value_out_of_range);
    }
    int64_t offset_val = value - min;
    uint64_t range = static_cast<uint64_t>(max - min) + 1;

    if (range <= 1) {
        return result<void>::ok();
    }

    uint8_t* out = const_cast<uint8_t*>(buf.data());

    if (range <= 256) {
        // X.691 §12.2.2: range ≤ 255 → encode in minimum bits. NO alignment.
        size_t bits = bits_needed_for_range(min, max);
        write_bits(out, bit_offset, static_cast<uint64_t>(offset_val), bits);
        return result<void>::ok();
    }

    if (range <= 65536) {
        // X.691 §12.2.3: range ≤ 65536 → 1-2 octets. NO alignment.
        size_t bits = bits_needed_for_range(min, max);
        write_bits(out, bit_offset, static_cast<uint64_t>(offset_val), bits);
        return result<void>::ok();
    }

    // X.691 §12.2.4: Large range → length determinant + unaligned value
    // Encode as unconstrained: length determinant for the value bytes + value in min bytes
    uint64_t uv = static_cast<uint64_t>(offset_val);
    size_t val_bytes = 0;
    {
        uint64_t tmp = uv;
        do { ++val_bytes; tmp >>= 8; } while (tmp > 0);
    }
    // Write length determinant (byte count) — unaligned
    auto ld = encode_length_determinant_unaligned(val_bytes, buf, bit_offset);
    if (ld.is_err()) return ld;
    // Write value bytes big-endian at current bit position
    for (size_t i = val_bytes; i > 0; --i) {
        write_bits(out, bit_offset, static_cast<uint64_t>((uv >> ((i - 1) * 8)) & 0xFF), 8);
    }
    return result<void>::ok();
}

// ── Length determinant (X.691 §21) UNALIGNED ───────────────────────────────

inline result<void> uper_encoder::encode_length_determinant_unaligned(
    size_t length, buffer_view& buf, size_t& bit_offset) {
    uint8_t* out = const_cast<uint8_t*>(buf.data());
    if (length <= 127) {
        // X.691 §21.3: 0xxxxxxx — single octet. NO alignment.
        write_bits(out, bit_offset, static_cast<uint64_t>(length), 8);
        return result<void>::ok();
    }
    if (length <= 16383) {
        // X.691 §21.4: 10xxxxxx xxxxxxxx — two octets. NO alignment.
        uint64_t two_bytes = 0x8000 | static_cast<uint64_t>(length & 0x3FFF);
        write_bits(out, bit_offset, two_bytes, 16);
        return result<void>::ok();
    }
    // X.691 §21.5: Fragmentation for large lengths. NO alignment.
    if (length <= 0x3FFFFFFF) {
        // 4-byte form: 110xxxxx xxxxxxxx xxxxxxxx xxxxxxxx (30 bits)
        uint64_t four_bytes =
            0xC0000000ULL |
            ((static_cast<uint64_t>(length >> 24) & 0x1F) << 24) |
            ((static_cast<uint64_t>(length >> 16) & 0xFF) << 16) |
            ((static_cast<uint64_t>(length >> 8) & 0xFF) << 8) |
            (static_cast<uint64_t>(length) & 0xFF);
        write_bits(out, bit_offset, four_bytes, 32);
    } else {
        // Fragment: encode 16384 chunks
        size_t remaining = length;
        while (remaining > 0) {
            size_t chunk = remaining > 16383 ? 16383 : remaining;
            uint64_t two_chunk =
                0x8000 | static_cast<uint64_t>(chunk & 0x3FFF);
            write_bits(out, bit_offset, two_chunk, 16);
            remaining -= chunk;
        }
    }
    return result<void>::ok();
}

// ── Template member function definitions ───────────────────────────────────

template<typename PerMeta>
inline result<void> uper_encoder::encode_integer(
    int64_t value, buffer_view& buf) {
    if constexpr (PerMeta::has_range_constraint) {
        return encode_constrained_whole_number_unaligned(
            value, PerMeta::min_value, PerMeta::max_value, buf, bit_offset_);
    }
    // Unconstrained: length determinant + two's complement value. NO alignment.
    uint8_t* out = const_cast<uint8_t*>(buf.data());
    uint64_t uv;
    size_t val_bytes;
    if (value >= 0) {
        uv = static_cast<uint64_t>(value);
        val_bytes = 0;
        uint64_t tmp = uv;
        do { ++val_bytes; tmp >>= 8; } while (tmp > 0);
        if (val_bytes > 0 && (uv >> ((val_bytes - 1) * 8)) & 0x80) {
            ++val_bytes;
        }
        if (val_bytes == 0) val_bytes = 1;
    } else {
        uv = static_cast<uint64_t>(value);
        val_bytes = 0;
        uint64_t tmp = ~uv;
        do { ++val_bytes; tmp >>= 8; } while (tmp > 0);
        if (val_bytes == 0) val_bytes = 1;
    }
    auto ld = encode_length_determinant_unaligned(val_bytes, buf, bit_offset_);
    if (ld.is_err()) return ld;
    // Write value bytes big-endian
    for (size_t i = val_bytes; i > 0; --i) {
        write_bits(out, bit_offset_,
                   static_cast<uint64_t>((uv >> ((i - 1) * 8)) & 0xFF), 8);
    }
    return result<void>::ok();
}

inline result<void> uper_encoder::encode_boolean(
    bool value, buffer_view& buf) {
    uint8_t bit = value ? 1 : 0;
    write_bits(const_cast<uint8_t*>(buf.data()), bit_offset_, bit, 1);
    // NO alignment after boolean in UPER
    return result<void>::ok();
}

inline result<void> uper_encoder::encode_null(buffer_view& /* buf */) {
    return result<void>::ok();
}

template<typename PerMeta>
inline result<void> uper_encoder::encode_octet_string(
    std::span<const uint8_t> data, buffer_view& buf) {
    uint8_t* out = const_cast<uint8_t*>(buf.data());

    if constexpr (PerMeta::has_size_constraint) {
        if constexpr (PerMeta::min_size == PerMeta::max_size) {
            if (data.size() != PerMeta::min_size) {
                return result<void>::err(error_code::constraint_violation);
            }
            // Fixed size: write data bytes directly at current bit offset. NO alignment.
            write_octets_unaligned(out, bit_offset_, data.data(), data.size());
            return result<void>::ok();
        }
        // Constrained: length prefix + data. NO alignment.
        uint64_t range = PerMeta::max_size - PerMeta::min_size + 1;
        if (range <= 65536) {
            size_t len_bits = bits_needed_unsigned(range - 1);
            write_bits(out, bit_offset_, data.size(), len_bits);
            // Write data bytes at current bit position
            write_octets_unaligned(out, bit_offset_, data.data(), data.size());
            return result<void>::ok();
        }
    }
    // Unconstrained: length determinant + data. NO alignment.
    auto ld = encode_length_determinant_unaligned(data.size(), buf, bit_offset_);
    if (ld.is_err()) return ld;
    write_octets_unaligned(out, bit_offset_, data.data(), data.size());
    return result<void>::ok();
}

template<typename PerMeta>
inline result<void> uper_encoder::encode_bit_string(
    std::span<const uint8_t> data, uint8_t unused_bits, buffer_view& buf) {
    size_t bit_length = data.size() * 8 - unused_bits;

    if constexpr (PerMeta::has_size_constraint) {
        if constexpr (PerMeta::min_size == PerMeta::max_size) {
            if (bit_length != PerMeta::min_size) {
                return result<void>::err(error_code::constraint_violation);
            }
            // Fixed size: write data bytes directly. NO alignment.
            uint8_t* out = const_cast<uint8_t*>(buf.data());
            write_octets_unaligned(out, bit_offset_, data.data(), data.size());
            return result<void>::ok();
        }
        uint64_t range = PerMeta::max_size - PerMeta::min_size + 1;
        if (range <= 65536) {
            size_t len_bits = bits_needed_unsigned(range - 1);
            uint8_t* out = const_cast<uint8_t*>(buf.data());
            write_bits(out, bit_offset_, bit_length, len_bits);
            write_octets_unaligned(out, bit_offset_, data.data(), data.size());
            return result<void>::ok();
        }
    }
    // Unconstrained: length determinant + unused bits + data. NO alignment.
    auto ld = encode_length_determinant_unaligned(bit_length, buf, bit_offset_);
    if (ld.is_err()) return ld;
    uint8_t* out = const_cast<uint8_t*>(buf.data());
    write_bits(out, bit_offset_, unused_bits, 8);
    write_octets_unaligned(out, bit_offset_, data.data(), data.size());
    return result<void>::ok();
}

template<typename PerMeta>
inline result<void> uper_encoder::encode_enumerated(
    int64_t index, buffer_view& buf) {
    if (index < 0 || static_cast<size_t>(index) >= PerMeta::normal_index_count) {
        return result<void>::err(error_code::value_out_of_range);
    }
    if (PerMeta::normal_index_count <= 1) {
        return result<void>::ok();
    }
    size_t bits = bits_needed_unsigned(PerMeta::normal_index_count - 1);
    uint8_t* out = const_cast<uint8_t*>(buf.data());
    write_bits(out, bit_offset_, static_cast<uint64_t>(index), bits);
    // NO alignment after in UPER
    return result<void>::ok();
}

template<typename PerMeta>
inline result<void> uper_encoder::encode_sequence_start(
    buffer_view& buf, const bool* optional_present, size_t optional_count) {
    uint8_t* out = const_cast<uint8_t*>(buf.data());
    if constexpr (PerMeta::has_extension) {
        write_bits(out, bit_offset_, 0, 1);
    }
    if (optional_count > 0 && optional_present != nullptr) {
        for (size_t i = 0; i < optional_count; ++i) {
            write_bits(out, bit_offset_, optional_present[i] ? 1ULL : 0ULL, 1);
        }
    }
    // NO alignment in UPER
    return result<void>::ok();
}

inline result<void> uper_encoder::encode_sequence_end(
    buffer_view& /* buf */) {
    return result<void>::ok();
}

template<typename PerMeta>
inline result<void> uper_encoder::encode_choice_index(
    int64_t choice_index, buffer_view& buf) {
    if (choice_index < 0 ||
        static_cast<size_t>(choice_index) >= PerMeta::alternative_count) {
        return result<void>::err(error_code::value_out_of_range);
    }
    size_t bits = bits_needed_unsigned(PerMeta::alternative_count - 1);
    uint8_t* out = const_cast<uint8_t*>(buf.data());
    write_bits(out, bit_offset_, static_cast<uint64_t>(choice_index), bits);
    // NO alignment after in UPER
    return result<void>::ok();
}

template<typename PerMeta>
inline result<void> uper_encoder::encode_sequence_of_length(
    size_t length, buffer_view& buf) {
    if constexpr (PerMeta::has_size_constraint) {
        if (length < PerMeta::min_size || length > PerMeta::max_size) {
            return result<void>::err(error_code::value_out_of_range);
        }
        uint64_t range = PerMeta::max_size - PerMeta::min_size + 1;
        if (range <= 65536) {
            size_t len_bits = bits_needed_unsigned(range - 1);
            uint8_t* out = const_cast<uint8_t*>(buf.data());
            write_bits(out, bit_offset_, length, len_bits);
            // NO alignment after in UPER
            return result<void>::ok();
        }
    }
    // Unconstrained. NO alignment.
    return encode_length_determinant_unaligned(length, buf, bit_offset_);
}

inline result<void> uper_encoder::encode_oid(
    std::span<const uint8_t> encoded_oid, buffer_view& buf) {
    // NO alignment before length determinant in UPER
    auto ld = encode_length_determinant_unaligned(encoded_oid.size(), buf, bit_offset_);
    if (ld.is_err()) return ld;
    uint8_t* out = const_cast<uint8_t*>(buf.data());
    write_octets_unaligned(out, bit_offset_, encoded_oid.data(), encoded_oid.size());
    return result<void>::ok();
}

} // namespace asn1pp::per
