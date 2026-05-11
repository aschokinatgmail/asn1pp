#pragma once

#if defined(__ARM_NEON) || defined(__aarch64__)

#include <arm_neon.h>

#include <cstddef>
#include <cstdint>

namespace asn1pp::arch::neon {

inline uint64_t load_u64_be(const uint8_t* p, size_t n) noexcept {
    if (n == 0) return 0;
    uint64_t result = 0;
    for (size_t i = 0; i < n; ++i) {
        result = (result << 8) | p[i];
    }
    return result;
}

inline void store_u64_be(uint8_t* p, uint64_t v, size_t n) noexcept {
    for (size_t i = n; i > 0; --i) {
        p[i - 1] = static_cast<uint8_t>(v & 0xFF);
        v >>= 8;
    }
}

inline void batch_load_u64_be(const uint8_t* const* ptrs, const size_t* sizes,
                              uint64_t* out, size_t count) noexcept {
    size_t i = 0;
    for (; i + 1 < count; i += 2) {
        if (sizes[i] == 8 && sizes[i + 1] == 8) {
            uint8x8_t lo = vld1_u8(ptrs[i]);
            uint8x8_t hi = vld1_u8(ptrs[i + 1]);
            uint8x16_t combined = vcombine_u8(lo, hi);
            uint8x16_t reversed = vrev64q_u8(combined);
            out[i] = vgetq_lane_u64(vreinterpretq_u64_u8(reversed), 0);
            out[i + 1] = vgetq_lane_u64(vreinterpretq_u64_u8(reversed), 1);
        } else {
            out[i] = load_u64_be(ptrs[i], sizes[i]);
            out[i + 1] = load_u64_be(ptrs[i + 1], sizes[i + 1]);
        }
    }
    for (; i < count; ++i) {
        out[i] = load_u64_be(ptrs[i], sizes[i]);
    }
}

inline void batch_store_u64_be(uint8_t** ptrs, const uint64_t* values,
                               const size_t* sizes, size_t count) noexcept {
    size_t i = 0;
    for (; i + 1 < count; i += 2) {
        if (sizes[i] == 8 && sizes[i + 1] == 8) {
            uint64_t temp[2] = {values[i], values[i + 1]};
            uint64x2_t vec = vld1q_u64(temp);
            uint8x16_t bytes = vrev64q_u8(vreinterpretq_u8_u64(vec));
            vst1_u8(ptrs[i], vget_low_u8(bytes));
            vst1_u8(ptrs[i + 1], vget_high_u8(bytes));
        } else {
            store_u64_be(ptrs[i], values[i], sizes[i]);
            store_u64_be(ptrs[i + 1], values[i + 1], sizes[i + 1]);
        }
    }
    for (; i < count; ++i) {
        store_u64_be(ptrs[i], values[i], sizes[i]);
    }
}

inline size_t copy_bytes(uint8_t* dst, const uint8_t* src, size_t n) noexcept {
    if (n == 0) return 0;
    size_t i = 0;
    for (; i + 16 <= n; i += 16) {
        uint8x16_t chunk = vld1q_u8(src + i);
        vst1q_u8(dst + i, chunk);
    }
    for (; i < n; ++i) {
        dst[i] = src[i];
    }
    return n;
}

inline void compare_tags(const uint8_t* tag_bytes, uint8_t expected,
                         size_t count, bool* results) noexcept {
    uint8x16_t expected_vec = vdupq_n_u8(expected);
    size_t i = 0;
    for (; i + 16 <= count; i += 16) {
        uint8x16_t chunk = vld1q_u8(tag_bytes + i);
        uint8x16_t cmp = vceqq_u8(chunk, expected_vec);
        for (int j = 0; j < 16; ++j) {
            results[i + j] = (vgetq_lane_u8(cmp, j) != 0);
        }
    }
    for (; i < count; ++i) {
        results[i] = (tag_bytes[i] == expected);
    }
}

}  // namespace asn1pp::arch::neon

#else
#error "arm_neon.hpp requires ARM NEON support."
#endif
