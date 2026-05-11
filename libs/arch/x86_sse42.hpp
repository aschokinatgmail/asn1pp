#pragma once

#if defined(__SSE4_2__) || defined(__AVX2__)

#include <smmintrin.h>

#include <cstddef>
#include <cstdint>

namespace asn1pp::arch::sse42 {

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
    static const __m128i shuffle_mask = _mm_setr_epi8(
        7, 6, 5, 4, 3, 2, 1, 0,
        15, 14, 13, 12, 11, 10, 9, 8);

    size_t i = 0;
    for (; i + 1 < count; i += 2) {
        if (sizes[i] == 8 && sizes[i + 1] == 8) {
            __m128i lo = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ptrs[i]));
            __m128i hi = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ptrs[i + 1]));
            __m128i combined = _mm_unpacklo_epi64(lo, hi);
            combined = _mm_shuffle_epi8(combined, shuffle_mask);
            out[i]     = static_cast<uint64_t>(_mm_cvtsi128_si64(combined));
            out[i + 1] = static_cast<uint64_t>(_mm_extract_epi64(combined, 1));
        } else {
            out[i]     = load_u64_be(ptrs[i], sizes[i]);
            out[i + 1] = load_u64_be(ptrs[i + 1], sizes[i + 1]);
        }
    }
    for (; i < count; ++i) {
        out[i] = load_u64_be(ptrs[i], sizes[i]);
    }
}

inline void batch_store_u64_be(uint8_t** ptrs, const uint64_t* values,
                               const size_t* sizes, size_t count) noexcept {
    static const __m128i shuffle_mask = _mm_setr_epi8(
        7, 6, 5, 4, 3, 2, 1, 0,
        15, 14, 13, 12, 11, 10, 9, 8);

    size_t i = 0;
    for (; i + 1 < count; i += 2) {
        if (sizes[i] == 8 && sizes[i + 1] == 8) {
            __m128i v = _mm_set_epi64x(static_cast<int64_t>(values[i + 1]),
                                       static_cast<int64_t>(values[i]));
            v = _mm_shuffle_epi8(v, shuffle_mask);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(ptrs[i]), v);
            __m128i shifted = _mm_srli_si128(v, 8);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(ptrs[i + 1]), shifted);
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
        __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i), chunk);
    }
    for (; i < n; ++i) {
        dst[i] = src[i];
    }
    return n;
}

inline void compare_tags(const uint8_t* tag_bytes, uint8_t expected,
                         size_t count, bool* results) noexcept {
    __m128i expected_vec = _mm_set1_epi8(static_cast<char>(expected));
    size_t i = 0;
    for (; i + 16 <= count; i += 16) {
        __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(tag_bytes + i));
        __m128i cmp   = _mm_cmpeq_epi8(chunk, expected_vec);
        int      bits  = _mm_movemask_epi8(cmp);
        for (int j = 0; j < 16; ++j) {
            results[i + j] = ((bits >> j) & 1) != 0;
        }
    }
    for (; i < count; ++i) {
        results[i] = (tag_bytes[i] == expected);
    }
}

}  // namespace asn1pp::arch::sse42

#else
#error "x86_sse42.hpp requires SSE4.2 support.  Compile with -msse4.2."
#endif
