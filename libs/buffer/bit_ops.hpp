#pragma once

#include <cstdint>
#include <cstddef>
#include <cassert>

namespace asn1pp {

inline constexpr void write_bits(uint8_t* buf, size_t& bit_offset, uint64_t value, size_t bit_count) noexcept {
    assert(bit_count <= 64);
    for (size_t i = 0; i < bit_count; ++i) {
        const size_t bit_idx = i;
        const size_t byte_idx = (bit_offset + bit_idx) / 8;
        const size_t bit_pos = (bit_offset + bit_idx) % 8;
        const uint64_t bit = (value >> (bit_count - 1 - i)) & 1ULL;
        buf[byte_idx] = (buf[byte_idx] & ~(1U << (7 - bit_pos))) | (static_cast<uint8_t>(bit << (7 - bit_pos)));
    }
    bit_offset += bit_count;
}

inline constexpr uint64_t read_bits(const uint8_t* buf, size_t& bit_offset, size_t bit_count) noexcept {
    assert(bit_count <= 64);
    uint64_t result = 0;
    for (size_t i = 0; i < bit_count; ++i) {
        const size_t byte_idx = (bit_offset + i) / 8;
        const size_t bit_pos = (bit_offset + i) % 8;
        const uint64_t bit = (buf[byte_idx] >> (7 - bit_pos)) & 1U;
        result = (result << 1) | bit;
    }
    bit_offset += bit_count;
    return result;
}

inline constexpr void write_octets(uint8_t* buf, size_t& byte_offset, const uint8_t* src, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        buf[byte_offset + i] = src[i];
    }
    byte_offset += count;
}

inline constexpr void read_octets(const uint8_t* buf, size_t& byte_offset, uint8_t* dst, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        dst[i] = buf[byte_offset + i];
    }
    byte_offset += count;
}

inline constexpr void align_to_octet(size_t& bit_offset) noexcept {
    const size_t remainder = bit_offset % 8;
    if (remainder != 0) {
        bit_offset += 8 - remainder;
    }
}

inline constexpr size_t remaining_bits(const uint8_t* buf, size_t buf_size, size_t bit_offset) noexcept {
    (void)buf;
    assert(bit_offset <= buf_size * 8);
    return buf_size * 8 - bit_offset;
}

inline constexpr size_t remaining_octets(const uint8_t* buf, size_t buf_size, size_t byte_offset) noexcept {
    (void)buf;
    assert(byte_offset <= buf_size);
    return buf_size - byte_offset;
}

}
