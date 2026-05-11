#pragma once

#if defined(__AVX2__)

#include <immintrin.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace asn1pp::arch::avx2 {

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
    // AVX2 byte-reversal shuffle mask.
    // _mm256_shuffle_epi8 operates within 128-bit lanes independently,
    // so the mask pattern is duplicated for both lanes.
    static const __m256i shuffle_mask = _mm256_setr_epi8(
        7, 6, 5, 4, 3, 2, 1, 0,         // reverse bytes 0-7
        15, 14, 13, 12, 11, 10, 9, 8,    // reverse bytes 8-15
        7, 6, 5, 4, 3, 2, 1, 0,          // reverse bytes 16-23
        15, 14, 13, 12, 11, 10, 9, 8);   // reverse bytes 24-31

    size_t i = 0;
    for (; i + 3 < count; i += 4) {
        if (sizes[i] == 8 && sizes[i + 1] == 8 &&
            sizes[i + 2] == 8 && sizes[i + 3] == 8) {
            // Copy 4 x 8 bytes into a temporary buffer, then load as 256-bit.
            uint64_t raw[4];
            std::memcpy(&raw[0], ptrs[i], 8);
            std::memcpy(&raw[1], ptrs[i + 1], 8);
            std::memcpy(&raw[2], ptrs[i + 2], 8);
            std::memcpy(&raw[3], ptrs[i + 3], 8);
            __m256i vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(raw));
            vec = _mm256_shuffle_epi8(vec, shuffle_mask);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out[i]), vec);
        } else {
            out[i]     = load_u64_be(ptrs[i], sizes[i]);
            out[i + 1] = load_u64_be(ptrs[i + 1], sizes[i + 1]);
            out[i + 2] = load_u64_be(ptrs[i + 2], sizes[i + 2]);
            out[i + 3] = load_u64_be(ptrs[i + 3], sizes[i + 3]);
        }
    }
    for (; i < count; ++i) {
        out[i] = load_u64_be(ptrs[i], sizes[i]);
    }
}

inline void batch_store_u64_be(uint8_t** ptrs, const uint64_t* values,
                               const size_t* sizes, size_t count) noexcept {
    // Same shuffle mask as load — byte reversal.
    static const __m256i shuffle_mask = _mm256_setr_epi8(
        7, 6, 5, 4, 3, 2, 1, 0,
        15, 14, 13, 12, 11, 10, 9, 8,
        7, 6, 5, 4, 3, 2, 1, 0,
        15, 14, 13, 12, 11, 10, 9, 8);

    size_t i = 0;
    for (; i + 3 < count; i += 4) {
        if (sizes[i] == 8 && sizes[i + 1] == 8 &&
            sizes[i + 2] == 8 && sizes[i + 3] == 8) {
            // Load 4 values, byte-reverse, store to temporary buffer,
            // then copy each 8 bytes to the target pointer.
            __m256i vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&values[i]));
            vec = _mm256_shuffle_epi8(vec, shuffle_mask);
            uint64_t raw[4];
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(raw), vec);
            std::memcpy(ptrs[i],     &raw[0], 8);
            std::memcpy(ptrs[i + 1], &raw[1], 8);
            std::memcpy(ptrs[i + 2], &raw[2], 8);
            std::memcpy(ptrs[i + 3], &raw[3], 8);
        } else {
            store_u64_be(ptrs[i],     values[i],     sizes[i]);
            store_u64_be(ptrs[i + 1], values[i + 1], sizes[i + 1]);
            store_u64_be(ptrs[i + 2], values[i + 2], sizes[i + 2]);
            store_u64_be(ptrs[i + 3], values[i + 3], sizes[i + 3]);
        }
    }
    for (; i < count; ++i) {
        store_u64_be(ptrs[i], values[i], sizes[i]);
    }
}

inline size_t copy_bytes(uint8_t* dst, const uint8_t* src, size_t n) noexcept {
    if (n == 0) return 0;
    size_t i = 0;
    for (; i + 32 <= n; i += 32) {
        __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + i));
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i), chunk);
    }
    for (; i < n; ++i) {
        dst[i] = src[i];
    }
    return n;
}

inline void compare_tags(const uint8_t* tag_bytes, uint8_t expected,
                         size_t count, bool* results) noexcept {
    __m256i expected_vec = _mm256_set1_epi8(static_cast<char>(expected));
    size_t i = 0;
    for (; i + 32 <= count; i += 32) {
        __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(tag_bytes + i));
        __m256i cmp   = _mm256_cmpeq_epi8(chunk, expected_vec);
        int      bits  = _mm256_movemask_epi8(cmp);
        for (int j = 0; j < 32; ++j) {
            results[i + j] = ((bits >> j) & 1) != 0;
        }
    }
    for (; i < count; ++i) {
        results[i] = (tag_bytes[i] == expected);
    }
}

}  // namespace asn1pp::arch::avx2

#else
#error "x86_avx2.hpp requires AVX2 support.  Compile with -mavx2."
#endif
