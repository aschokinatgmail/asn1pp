#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <type_traits>

#include "codec/result.hpp"
#include "buffer/buffer_view.hpp"
#include "buffer/bit_ops.hpp"
#include "arch/simd.hpp"

namespace asn1pp::per {

/// PER ALIGNED encoder per ITU-T X.691.
///
/// Stateless per-call; bit_offset_ tracks current bit position in the output
/// buffer. All PER metadata (constraints, extension markers, optional fields)
/// comes from constexpr PerMeta template parameters emitted by the code
/// generator (Task 27).
///
/// After each complete type encoding, output is aligned to an octet boundary.
/// Uses SIMD-accelerated arch::copy_bytes() for large OCTET STRING payloads.
class per_aligned_encoder {
public:
    per_aligned_encoder() = default;

    // ── Primitive type encoders ────────────────────────────────────────

    /// Encode INTEGER using constraint metadata.
    /// Value is encoded as constrained whole number (X.691 §12.2):
    ///   value - PerMeta::min_value in ceil(log2(range)) bits, then aligned.
    template<typename PerMeta>
    result<void> encode_integer(int64_t value, buffer_view& buf);

    /// Encode BOOLEAN — exactly 1 bit (0=FALSE, 1=TRUE).
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
    /// optional-field presence bitmap.
    template<typename PerMeta>
    result<void> encode_sequence_start(buffer_view& buf,
                                       const bool* optional_present = nullptr,
                                       size_t optional_count = 0);

    /// No-op for ALIGNED PER (alignment is done after each field).
    result<void> encode_sequence_end(buffer_view& buf);

    /// Encode CHOICE index.
    template<typename PerMeta>
    result<void> encode_choice_index(int64_t choice_index, buffer_view& buf);

    /// Encode SEQUENCE OF / SET OF length determinant.
    template<typename PerMeta>
    result<void> encode_sequence_of_length(size_t length, buffer_view& buf);

    /// Encode OBJECT IDENTIFIER (pre-encoded subidentifier form).
    result<void> encode_oid(std::span<const uint8_t> encoded_oid, buffer_view& buf);

    // ── Batch drain ────────────────────────────────────────────────────

    /// Drain any pending batch encode operations.
    /// No-op for PER — bit-level encoding doesn't use byte-level SIMD batching.
    error_code flush_encode() noexcept { return error_code::ok; }

    // ── Low-level helpers (public for testing) ─────────────────────────

    /// Encode constrained whole number per X.691 §12.2.
    static result<void> encode_constrained_whole_number(
        int64_t value, int64_t min, int64_t max,
        buffer_view& buf, size_t& bit_offset);

    /// Encode length determinant per X.691 §21.
    static result<void> encode_length_determinant(
        size_t length, buffer_view& buf, size_t& bit_offset);

    /// Align current bit position to next octet boundary.
    static void align(size_t& bit_offset);

    /// Calculate ceil(log2(n + 1)) — bits needed to represent 0..n.
    static constexpr size_t bits_needed_unsigned(size_t range);

    /// Calculate ceil(log2(range)) where range = max - min + 1.
    static constexpr size_t bits_needed_for_range(int64_t min, int64_t max);

    /// Current bit offset in the output buffer.
    [[nodiscard]] size_t bit_offset() const noexcept { return bit_offset_; }

    /// Set bit offset (used after encoding into the buffer to track position).
    void set_bit_offset(size_t offset) noexcept { bit_offset_ = offset; }

    /// SIMD-accelerated octet copy — uses arch::copy_bytes() for > 32 bytes.
    static void write_octets_simd(uint8_t* dst, size_t& byte_offset,
                                  const uint8_t* src, size_t count) noexcept;

private:
    size_t bit_offset_ = 0;
};

// ── inline constexpr helpers ───────────────────────────────────────────────

inline constexpr size_t per_aligned_encoder::bits_needed_unsigned(size_t range) {
    if (range == 0) return 0;
    size_t bits = 0;
    while (range > 0) {
        range >>= 1;
        ++bits;
    }
    return bits;
}

inline constexpr size_t per_aligned_encoder::bits_needed_for_range(
    int64_t min, int64_t max) {
    if (min > max) return 0;
    uint64_t range_u = static_cast<uint64_t>(max - min) + 1;
    return bits_needed_unsigned(range_u - 1);
}

inline void per_aligned_encoder::align(size_t& bit_offset) {
    align_to_octet(bit_offset);
}

inline void per_aligned_encoder::write_octets_simd(uint8_t* dst, size_t& byte_offset,
                                                    const uint8_t* src, size_t count) noexcept {
    if (count > 32) {
        arch::copy_bytes(dst + byte_offset, src, count);
    } else {
        for (size_t i = 0; i < count; ++i) {
            dst[byte_offset + i] = src[i];
        }
    }
    byte_offset += count;
}

// ── Constrained whole number (X.691 §12.2) ─────────────────────────────────

inline result<void> per_aligned_encoder::encode_constrained_whole_number(
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
        // X.691 §12.2.2: range ≤ 255 → encode in minimum bits
        // range = 256 → exactly 8 bits (edge case, fits 1 byte)
        size_t bits = bits_needed_for_range(min, max);
        align(bit_offset);
        write_bits(out, bit_offset, static_cast<uint64_t>(offset_val), bits);
        align(bit_offset);
        return result<void>::ok();
    }

    if (range <= 65536) {
        // X.691 §12.2.3: range ≤ 65536 → 1-2 octets, aligned
        align(bit_offset);
        size_t bits = bits_needed_for_range(min, max);
        write_bits(out, bit_offset, static_cast<uint64_t>(offset_val), bits);
        align(bit_offset);
        return result<void>::ok();
    }

    // X.691 §12.2.4: Large range → length determinant + unaligned value
    // Encode as unconstrained: length determinant for the value bytes + value in min bytes
    align(bit_offset);
    // Determine minimum bytes needed for the offset value
    uint64_t uv = static_cast<uint64_t>(offset_val);
    size_t val_bytes = 0;
    {
        uint64_t tmp = uv;
        do { ++val_bytes; tmp >>= 8; } while (tmp > 0);
    }
    // Write length determinant (byte count)
    auto ld = encode_length_determinant(val_bytes, buf, bit_offset);
    if (ld.is_err()) return ld;
    // Write value bytes big-endian
    for (size_t i = val_bytes; i > 0; --i) {
        size_t byte_off = bit_offset / 8;
        out[byte_off] = static_cast<uint8_t>((uv >> ((i - 1) * 8)) & 0xFF);
        bit_offset += 8;
    }
    return result<void>::ok();
}

// ── Length determinant (X.691 §21) ─────────────────────────────────────────

inline result<void> per_aligned_encoder::encode_length_determinant(
    size_t length, buffer_view& buf, size_t& bit_offset) {
    uint8_t* out = const_cast<uint8_t*>(buf.data());
    if (length <= 127) {
        // X.691 §21.3: 0xxxxxxx — single octet
        align(bit_offset);
        size_t byte_off = bit_offset / 8;
        out[byte_off] = static_cast<uint8_t>(length);
        bit_offset += 8;
        return result<void>::ok();
    }
    if (length <= 16383) {
        // X.691 §21.4: 10xxxxxx xxxxxxxx — two octets
        align(bit_offset);
        size_t byte_off = bit_offset / 8;
        out[byte_off] = static_cast<uint8_t>(0x80 | ((length >> 8) & 0x3F));
        out[byte_off + 1] = static_cast<uint8_t>(length & 0xFF);
        bit_offset += 16;
        return result<void>::ok();
    }
    // X.691 §21.5: Fragmentation for large lengths
    // For now, encode in a simple multi-byte form
    // First octet: 11000000 + high bits of length
    align(bit_offset);
    size_t byte_off = bit_offset / 8;
    if (length <= 0x3FFFFFFF) {
        // 4-byte form: 110xxxxx xxxxxxxx xxxxxxxx xxxxxxxx (30 bits)
        out[byte_off] = static_cast<uint8_t>(0xC0 | ((length >> 24) & 0x1F));
        out[byte_off + 1] = static_cast<uint8_t>((length >> 16) & 0xFF);
        out[byte_off + 2] = static_cast<uint8_t>((length >> 8) & 0xFF);
        out[byte_off + 3] = static_cast<uint8_t>(length & 0xFF);
        bit_offset += 32;
    } else {
        // Fragment: encode 16384 chunks
        size_t remaining = length;
        size_t written = 0;
        while (remaining > 0) {
            size_t chunk = remaining > 16383 ? 16383 : remaining;
            out[byte_off + written] = static_cast<uint8_t>(0x80 | ((chunk >> 8) & 0x3F));
            out[byte_off + written + 1] = static_cast<uint8_t>(chunk & 0xFF);
            written += 2;
            remaining -= chunk;
        }
        bit_offset += written * 8;
    }
    return result<void>::ok();
}

// ── Template member function definitions ───────────────────────────────────

template<typename PerMeta>
inline result<void> per_aligned_encoder::encode_integer(
    int64_t value, buffer_view& buf) {
    if constexpr (PerMeta::has_range_constraint) {
        return encode_constrained_whole_number(
            value, PerMeta::min_value, PerMeta::max_value, buf, bit_offset_);
    }
    // Unconstrained: length determinant + two's complement value
    align(bit_offset_);
    uint8_t* out = const_cast<uint8_t*>(buf.data());
    // Determine minimum bytes for value (two's complement)
    uint64_t uv;
    size_t val_bytes;
    if (value >= 0) {
        uv = static_cast<uint64_t>(value);
        val_bytes = 0;
        uint64_t tmp = uv;
        // Positive values: need high bit = 0, so if MSB is set, add a 0x00 byte
        do { ++val_bytes; tmp >>= 8; } while (tmp > 0);
        if (val_bytes > 0 && (uv >> ((val_bytes - 1) * 8)) & 0x80) {
            ++val_bytes; // need leading zero byte
        }
        if (val_bytes == 0) val_bytes = 1;
    } else {
        uv = static_cast<uint64_t>(value);
        val_bytes = 0;
        uint64_t tmp = ~uv; // complement for counting
        do { ++val_bytes; tmp >>= 8; } while (tmp > 0);
        if (val_bytes == 0) val_bytes = 1;
    }
    auto ld = encode_length_determinant(val_bytes, buf, bit_offset_);
    if (ld.is_err()) return ld;
    // Write value bytes big-endian
    for (size_t i = val_bytes; i > 0; --i) {
        size_t byte_off = bit_offset_ / 8;
        out[byte_off] = static_cast<uint8_t>((uv >> ((i - 1) * 8)) & 0xFF);
        bit_offset_ += 8;
    }
    return result<void>::ok();
}

inline result<void> per_aligned_encoder::encode_boolean(
    bool value, buffer_view& buf) {
    uint8_t bit = value ? 1 : 0;
    write_bits(const_cast<uint8_t*>(buf.data()), bit_offset_, bit, 1);
    align(bit_offset_);
    return result<void>::ok();
}

inline result<void> per_aligned_encoder::encode_null(buffer_view& /* buf */) {
    return result<void>::ok();
}

template<typename PerMeta>
inline result<void> per_aligned_encoder::encode_octet_string(
    std::span<const uint8_t> data, buffer_view& buf) {
    if constexpr (PerMeta::has_size_constraint) {
        if constexpr (PerMeta::min_size == PerMeta::max_size) {
            if (data.size() != PerMeta::min_size) {
                return result<void>::err(error_code::constraint_violation);
            }
            align(bit_offset_);
            size_t byte_off = bit_offset_ / 8;
            write_octets_simd(const_cast<uint8_t*>(buf.data()), byte_off, data.data(), data.size());
            bit_offset_ = byte_off * 8;
            return result<void>::ok();
        }
        // Constrained: length prefix + data
        uint64_t range = PerMeta::max_size - PerMeta::min_size + 1;
        if (range <= 65536) {
            align(bit_offset_);
            size_t len_bits = bits_needed_unsigned(range - 1);
            uint8_t* out = const_cast<uint8_t*>(buf.data());
            size_t tmp_bit = bit_offset_;
            write_bits(out, tmp_bit, data.size(), len_bits);
            bit_offset_ = tmp_bit;
            align(bit_offset_);
            size_t byte_off = bit_offset_ / 8;
            write_octets_simd(out, byte_off, data.data(), data.size());
            bit_offset_ = byte_off * 8;
            return result<void>::ok();
        }
    }
    // Unconstrained: length determinant + data
    align(bit_offset_);
    auto ld = encode_length_determinant(data.size(), buf, bit_offset_);
    if (ld.is_err()) return ld;
    size_t byte_off = bit_offset_ / 8;
    write_octets_simd(const_cast<uint8_t*>(buf.data()), byte_off, data.data(), data.size());
    bit_offset_ = byte_off * 8;
    return result<void>::ok();
}

template<typename PerMeta>
inline result<void> per_aligned_encoder::encode_bit_string(
    std::span<const uint8_t> data, uint8_t unused_bits, buffer_view& buf) {
    (void)unused_bits;
    size_t bit_length = data.size() * 8 - unused_bits;
    if constexpr (PerMeta::has_size_constraint) {
        if constexpr (PerMeta::min_size == PerMeta::max_size) {
            if (bit_length != PerMeta::min_size) {
                return result<void>::err(error_code::constraint_violation);
            }
            align(bit_offset_);
            size_t byte_off = bit_offset_ / 8;
            write_octets_simd(const_cast<uint8_t*>(buf.data()), byte_off, data.data(), data.size());
            bit_offset_ = byte_off * 8;
            return result<void>::ok();
        }
        uint64_t range = PerMeta::max_size - PerMeta::min_size + 1;
        if (range <= 65536) {
            align(bit_offset_);
            size_t len_bits = bits_needed_unsigned(range - 1);
            uint8_t* out = const_cast<uint8_t*>(buf.data());
            size_t tmp_bit = bit_offset_;
            write_bits(out, tmp_bit, bit_length, len_bits);
            bit_offset_ = tmp_bit;
            align(bit_offset_);
            size_t byte_off = bit_offset_ / 8;
            write_octets_simd(out, byte_off, data.data(), data.size());
            bit_offset_ = byte_off * 8;
            return result<void>::ok();
        }
    }
    align(bit_offset_);
    auto ld = encode_length_determinant(bit_length, buf, bit_offset_);
    if (ld.is_err()) return ld;
    size_t byte_off = bit_offset_ / 8;
    uint8_t* out = const_cast<uint8_t*>(buf.data());
    out[byte_off] = unused_bits;
    bit_offset_ += 8;
    byte_off = bit_offset_ / 8;
    write_octets_simd(out, byte_off, data.data(), data.size());
    bit_offset_ = byte_off * 8;
    return result<void>::ok();
}

template<typename PerMeta>
inline result<void> per_aligned_encoder::encode_enumerated(
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
    align(bit_offset_);
    return result<void>::ok();
}

template<typename PerMeta>
inline result<void> per_aligned_encoder::encode_sequence_start(
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
    if (bit_offset_ > 0) {
        align(bit_offset_);
    }
    return result<void>::ok();
}

inline result<void> per_aligned_encoder::encode_sequence_end(
    buffer_view& /* buf */) {
    return result<void>::ok();
}

template<typename PerMeta>
inline result<void> per_aligned_encoder::encode_choice_index(
    int64_t choice_index, buffer_view& buf) {
    if (choice_index < 0 ||
        static_cast<size_t>(choice_index) >= PerMeta::alternative_count) {
        return result<void>::err(error_code::value_out_of_range);
    }
    size_t bits = bits_needed_unsigned(PerMeta::alternative_count - 1);
    uint8_t* out = const_cast<uint8_t*>(buf.data());
    write_bits(out, bit_offset_, static_cast<uint64_t>(choice_index), bits);
    align(bit_offset_);
    return result<void>::ok();
}

template<typename PerMeta>
inline result<void> per_aligned_encoder::encode_sequence_of_length(
    size_t length, buffer_view& buf) {
    if constexpr (PerMeta::has_size_constraint) {
        if (length < PerMeta::min_size || length > PerMeta::max_size) {
            return result<void>::err(error_code::value_out_of_range);
        }
        uint64_t range = PerMeta::max_size - PerMeta::min_size + 1;
        if (range <= 65536) {
            size_t len_bits = bits_needed_unsigned(range - 1);
            align(bit_offset_);
            uint8_t* out = const_cast<uint8_t*>(buf.data());
            write_bits(out, bit_offset_, length, len_bits);
            align(bit_offset_);
            return result<void>::ok();
        }
    }
    // Unconstrained
    align(bit_offset_);
    return encode_length_determinant(length, buf, bit_offset_);
}

inline result<void> per_aligned_encoder::encode_oid(
    std::span<const uint8_t> encoded_oid, buffer_view& buf) {
    align(bit_offset_);
    auto ld = encode_length_determinant(encoded_oid.size(), buf, bit_offset_);
    if (ld.is_err()) return ld;
    size_t byte_off = bit_offset_ / 8;
    write_octets(const_cast<uint8_t*>(buf.data()), byte_off,
                 encoded_oid.data(), encoded_oid.size());
    bit_offset_ = byte_off * 8;
    return result<void>::ok();
}

} // namespace asn1pp::per
