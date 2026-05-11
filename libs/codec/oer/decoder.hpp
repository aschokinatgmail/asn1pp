#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <vector>
#include <utility>
#include <cstring>

#include "codec/result.hpp"
#include "codec/arch_codec.hpp"
#include "codec/batch_buffer.hpp"
#include "buffer/buffer_view.hpp"

namespace asn1pp::oer {

/// OER (Octet Encoding Rules) decoder per ITU-T X.696.
///
/// Stateless per-call; the buffer_view is advanced past consumed bytes.
/// Returns decoded values via result<T>. For types returning containers
/// (octet string, bit string data), allocates a std::vector<uint8_t>.
class oer_decoder {
public:
    oer_decoder() = default;

    // ── Primitive type decoders ────────────────────────────────────────

    /// Decode INTEGER per X.696 §7.2.
    template<typename OerMeta>
    result<int64_t> decode_integer(buffer_view& buf);

    /// Decode BOOLEAN per X.696 §7.3 — 1 octet.
    result<bool> decode_boolean(buffer_view& buf);

    /// Decode NULL per X.696 §7.4 — 0 octets.
    result<void> decode_null(buffer_view& buf);

    /// Decode OCTET STRING per X.696 §16.
    /// Returns a vector of the raw content octets.
    template<typename OerMeta>
    result<std::vector<uint8_t>> decode_octet_string(buffer_view& buf);

    /// Decode BIT STRING per X.696 §14.
    /// Returns (data, unused_bits).
    template<typename OerMeta>
    result<std::pair<std::vector<uint8_t>, uint8_t>> decode_bit_string(buffer_view& buf);

    /// Decode ENUMERATED per X.696 §7.7.
    template<typename OerMeta>
    result<int64_t> decode_enumerated(buffer_view& buf);

    // ── Constructed type helpers ────────────────────────────────────────

    /// Decode CHOICE index per X.696 §19.
    template<typename OerMeta>
    result<int64_t> decode_choice_index(buffer_view& buf);

    /// Decode SEQUENCE OF / SET OF length determinant per X.696 §20.
    template<typename OerMeta>
    result<size_t> decode_sequence_of_length(buffer_view& buf);

    /// Decode length determinant per X.696 §4.
    result<size_t> decode_length_determinant(buffer_view& buf);

    // ── Batch drain ────────────────────────────────────────────────────

    /// Drain any pending batch decode operations via SIMD.
    /// Idempotent — calling when no pending ops is a no-op.
    /// Returns error_code::ok on success, or the first error encountered.
    error_code flush_decode() noexcept;

    /// Number of consecutive decode_integer() calls before flushing.
    /// 4 for AVX2, 2 for SSE4.2/NEON, 1 for scalar (pass-through).
    static size_t batch_size() noexcept;

private:
    /// Determine byte width for integer per constraint at compile time.
    template<typename OerMeta>
    static constexpr size_t integer_byte_width();

    /// Determine byte width for index (enumerated, choice).
    static constexpr size_t index_byte_width(size_t count);

    /// Decode a fixed-width big-endian unsigned integer from buffer.
    static uint64_t read_be_bytes(const uint8_t* data, size_t count);

    /// Convert unsigned fixed-width value to signed using two's complement.
    static int64_t unsigned_to_signed(uint64_t val, size_t byte_width);

    /// Decode two's complement big-endian integer from bytes.
    static int64_t decode_integer_value(std::span<const uint8_t> bytes);

    // ── Batch buffer ───────────────────────────────────────────────────

    static constexpr size_t kMaxBatchSize = 4;

    struct pending_integer {
        uint8_t value_buf[8];
        size_t size;
    };

    batch_buffer<pending_integer, kMaxBatchSize> pending_;
};

// ============================================================================
// Buffer helpers
// ============================================================================

namespace {

inline uint8_t read_be_8_from_buf(buffer_view& buf) {
    uint8_t val = buf.data()[0];
    buf = buf.subview(1, buf.size() - 1);
    return val;
}

inline uint16_t read_be_16_from_buf(buffer_view& buf) {
    const uint8_t* d = buf.data();
    uint16_t val = (static_cast<uint16_t>(d[0]) << 8) | d[1];
    buf = buf.subview(2, buf.size() - 2);
    return val;
}

inline uint32_t read_be_32_from_buf(buffer_view& buf) {
    const uint8_t* d = buf.data();
    uint32_t val = (static_cast<uint32_t>(d[0]) << 24)
                 | (static_cast<uint32_t>(d[1]) << 16)
                 | (static_cast<uint32_t>(d[2]) << 8)
                 | d[3];
    buf = buf.subview(4, buf.size() - 4);
    return val;
}

inline uint64_t read_be_64_from_buf(buffer_view& buf) {
    const uint8_t* d = buf.data();
    uint64_t val = (static_cast<uint64_t>(d[0]) << 56)
                 | (static_cast<uint64_t>(d[1]) << 48)
                 | (static_cast<uint64_t>(d[2]) << 40)
                 | (static_cast<uint64_t>(d[3]) << 32)
                 | (static_cast<uint64_t>(d[4]) << 24)
                 | (static_cast<uint64_t>(d[5]) << 16)
                 | (static_cast<uint64_t>(d[6]) << 8)
                 | d[7];
    buf = buf.subview(8, buf.size() - 8);
    return val;
}

} // anonymous namespace

// ============================================================================
// Private helpers
// ============================================================================

template<typename OerMeta>
constexpr size_t oer_decoder::integer_byte_width() {
    if constexpr (OerMeta::has_range_constraint) {
        constexpr int64_t range_min = OerMeta::min_value;
        constexpr int64_t range_max = OerMeta::max_value;

        if constexpr (range_min >= 0) {
            if constexpr (range_max <= 255)       return 1;
            if constexpr (range_max <= 65535)     return 2;
            if constexpr (range_max <= 4294967295) return 4;
            return 8;
        } else {
            if constexpr (range_min >= -128 && range_max <= 127)         return 1;
            if constexpr (range_min >= -32768 && range_max <= 32767)     return 2;
            if constexpr (range_min >= -2147483648LL && range_max <= 2147483647LL) return 4;
            return 8;
        }
    }
    return 0;
}

constexpr size_t oer_decoder::index_byte_width(size_t count) {
    if (count <= 255)   return 1;
    if (count <= 65535) return 2;
    return 4;
}

inline uint64_t oer_decoder::read_be_bytes(const uint8_t* data, size_t count) {
    uint64_t val = 0;
    for (size_t i = 0; i < count; ++i) {
        val = (val << 8) | data[i];
    }
    return val;
}

inline int64_t oer_decoder::unsigned_to_signed(uint64_t val, size_t byte_width) {
    switch (byte_width) {
    case 1: return static_cast<int64_t>(static_cast<int8_t>(static_cast<uint8_t>(val)));
    case 2: return static_cast<int64_t>(static_cast<int16_t>(static_cast<uint16_t>(val)));
    case 4: return static_cast<int64_t>(static_cast<int32_t>(static_cast<uint32_t>(val)));
    case 8: return static_cast<int64_t>(val);
    default: return static_cast<int64_t>(val);
    }
}

inline int64_t oer_decoder::decode_integer_value(std::span<const uint8_t> bytes) {
    const size_t len = bytes.size();
    if (len == 0) return 0;
    const bool negative = (bytes[0] & 0x80) != 0;
    int64_t result = negative ? static_cast<int64_t>(-1) : 0;
    for (size_t i = 0; i < len; ++i) {
        result = (result << 8) | static_cast<int64_t>(bytes[i]);
    }
    return result;
}

// ============================================================================
// Template member functions
// ============================================================================

template<typename OerMeta>
result<int64_t> oer_decoder::decode_integer(buffer_view& buf) {
    if constexpr (OerMeta::has_range_constraint) {
        constexpr size_t byte_width = integer_byte_width<OerMeta>();
        if (buf.size() < byte_width) return result<int64_t>::err(error_code::buffer_underflow);

        uint64_t uval = read_be_bytes(buf.data(), byte_width);
        buf = buf.subview(byte_width, buf.size() - byte_width);

        constexpr int64_t range_min = OerMeta::min_value;
        constexpr int64_t range_max = OerMeta::max_value;

        if constexpr (range_min >= 0) {
            // Unsigned: just cast
            return result<int64_t>::ok(static_cast<int64_t>(uval));
        } else {
            // Signed: convert from two's complement
            int64_t signed_val = unsigned_to_signed(uval, byte_width);
            if (signed_val < range_min || signed_val > range_max)
                return ::asn1pp::result<int64_t>::err(error_code::value_out_of_range);
            return ::asn1pp::result<int64_t>::ok(signed_val);
        }
    } else {
        // Unconstrained: decode length determinant then value
        auto len_r = decode_length_determinant(buf);
        if (len_r.is_err()) return result<int64_t>::err(len_r.error());
        const size_t val_len = len_r.value();

        if (val_len == 0) return result<int64_t>::err(error_code::invalid_length);
        if (val_len > 8) return result<int64_t>::err(error_code::value_out_of_range);
        if (buf.size() < val_len) return result<int64_t>::err(error_code::buffer_underflow);

        // Copy value bytes before advancing
        uint8_t val_buf[8] = {};
        for (size_t i = 0; i < val_len; ++i) val_buf[i] = buf.data()[i];
        buf = buf.subview(val_len, buf.size() - val_len);

        return result<int64_t>::ok(
            decode_integer_value(std::span<const uint8_t>(val_buf, val_len)));
    }
}

template<typename OerMeta>
result<std::vector<uint8_t>> oer_decoder::decode_octet_string(buffer_view& buf) {
    if constexpr (OerMeta::has_size_constraint) {
        constexpr size_t sz_min = OerMeta::min_size;
        constexpr size_t sz_max = OerMeta::max_size;

        if (sz_min == sz_max) {
            // Fixed size — no length prefix
            if (buf.size() < sz_min)
                return result<std::vector<uint8_t>>::err(error_code::buffer_underflow);
            std::vector<uint8_t> data(buf.data(), buf.data() + sz_min);
            buf = buf.subview(sz_min, buf.size() - sz_min);
            return result<std::vector<uint8_t>>::ok(std::move(data));
        } else {
            // Constrained — decode length as constrained whole number
            constexpr size_t len_width = (sz_max <= 255) ? 1 : ((sz_max <= 65535) ? 2 : 4);
            if (buf.size() < len_width)
                return result<std::vector<uint8_t>>::err(error_code::buffer_underflow);
            uint64_t len_val = read_be_bytes(buf.data(), len_width);
            buf = buf.subview(len_width, buf.size() - len_width);
            const size_t data_len = static_cast<size_t>(len_val);

            if (data_len < sz_min || data_len > sz_max)
                return result<std::vector<uint8_t>>::err(error_code::constraint_violation);
            if (buf.size() < data_len)
                return result<std::vector<uint8_t>>::err(error_code::buffer_underflow);

            std::vector<uint8_t> data(buf.data(), buf.data() + data_len);
            buf = buf.subview(data_len, buf.size() - data_len);
            return result<std::vector<uint8_t>>::ok(std::move(data));
        }
    } else {
        // Unconstrained — length determinant + data
        auto len_r = decode_length_determinant(buf);
        if (len_r.is_err()) return result<std::vector<uint8_t>>::err(len_r.error());
        const size_t data_len = len_r.value();

        if (buf.size() < data_len)
            return result<std::vector<uint8_t>>::err(error_code::buffer_underflow);

        std::vector<uint8_t> data(buf.data(), buf.data() + data_len);
        buf = buf.subview(data_len, buf.size() - data_len);
        return result<std::vector<uint8_t>>::ok(std::move(data));
    }
}

template<typename OerMeta>
result<std::pair<std::vector<uint8_t>, uint8_t>> oer_decoder::decode_bit_string(buffer_view& buf) {
    if constexpr (OerMeta::has_size_constraint) {
        constexpr size_t sz_min = OerMeta::min_size;
        constexpr size_t sz_max = OerMeta::max_size;

        if (sz_min == sz_max) {
            // Fixed size — no length, no unused_bits prefix
            if (buf.size() < sz_min)
                return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::buffer_underflow);
            std::vector<uint8_t> data(buf.data(), buf.data() + sz_min);
            buf = buf.subview(sz_min, buf.size() - sz_min);
            return result<std::pair<std::vector<uint8_t>, uint8_t>>::ok({std::move(data), 0});
        }
    }

    // Unconstrained or constrained-non-fixed: length + unused_bits + data
    // We use length determinant for unconstrained
    auto len_r = decode_length_determinant(buf);
    if (len_r.is_err())
        return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(len_r.error());
    const size_t total = len_r.value();

    if (total < 1) // at least unused_bits octet
        return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::invalid_length);
    if (buf.size() < total)
        return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::buffer_underflow);

    uint8_t unused = buf.data()[0];
    std::vector<uint8_t> data(buf.data() + 1, buf.data() + total);
    buf = buf.subview(total, buf.size() - total);
    return result<std::pair<std::vector<uint8_t>, uint8_t>>::ok({std::move(data), unused});
}

template<typename OerMeta>
result<int64_t> oer_decoder::decode_enumerated(buffer_view& buf) {
    constexpr size_t width = index_byte_width(OerMeta::root_count);
    if (buf.size() < width) return result<int64_t>::err(error_code::buffer_underflow);
    uint64_t index = read_be_bytes(buf.data(), width);
    buf = buf.subview(width, buf.size() - width);
    return result<int64_t>::ok(static_cast<int64_t>(index));
}

template<typename OerMeta>
result<int64_t> oer_decoder::decode_choice_index(buffer_view& buf) {
    constexpr size_t width = index_byte_width(OerMeta::alternative_count);
    if (buf.size() < width) return result<int64_t>::err(error_code::buffer_underflow);
    uint64_t index = read_be_bytes(buf.data(), width);
    buf = buf.subview(width, buf.size() - width);
    return result<int64_t>::ok(static_cast<int64_t>(index));
}

template<typename OerMeta>
result<size_t> oer_decoder::decode_sequence_of_length(buffer_view& buf) {
    return decode_length_determinant(buf);
}

// ============================================================================
// Non-template member functions
// ============================================================================

inline result<bool> oer_decoder::decode_boolean(buffer_view& buf) {
    if (buf.size() < 1) return result<bool>::err(error_code::buffer_underflow);
    bool val = (buf.data()[0] != 0x00);
    buf = buf.subview(1, buf.size() - 1);
    return result<bool>::ok(val);
}

inline result<void> oer_decoder::decode_null(buffer_view& buf) {
    (void)buf; // 0 octets
    return result<void>::ok();
}

inline result<size_t> oer_decoder::decode_length_determinant(buffer_view& buf) {
    if (buf.size() < 1) return result<size_t>::err(error_code::buffer_underflow);
    uint8_t first = buf.data()[0];

    if ((first & 0x80) == 0) {
        // Short form: 0xxxxxxx
        size_t val = first;
        buf = buf.subview(1, buf.size() - 1);
        return result<size_t>::ok(val);
    }

    if ((first & 0xC0) == 0x80) {
        // Two-byte form: 10xxxxxx xxxxxxxx
        if (buf.size() < 2) return result<size_t>::err(error_code::buffer_underflow);
        uint16_t val = ((static_cast<uint16_t>(first & 0x3F)) << 8) | buf.data()[1];
        buf = buf.subview(2, buf.size() - 2);
        return result<size_t>::ok(val);
    }

    // Long form: 0xC0 + 16-bit count
    if (buf.size() < 3) return result<size_t>::err(error_code::buffer_underflow);
    uint32_t val = (static_cast<uint32_t>(buf.data()[1]) << 8) | buf.data()[2];
    buf = buf.subview(3, buf.size() - 3);
    return result<size_t>::ok(val);
}

} // namespace asn1pp::oer
