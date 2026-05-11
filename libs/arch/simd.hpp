#pragma once

//
// simd.hpp — unified SIMD dispatch header
//
// Include this single header to get compile-time + runtime dispatch
// to the best available SIMD backend for the current platform.
//

#include "arch/detect.hpp"
#include "arch/scalar.hpp"

// Conditional includes — only include backends compiled in
#if defined(__SSE4_2__) || defined(__AVX2__)
#include "arch/x86_sse42.hpp"
#endif
#if defined(__AVX2__)
#include "arch/x86_avx2.hpp"
#endif
#if defined(__ARM_NEON) || defined(__aarch64__)
#include "arch/arm_neon.hpp"
#endif

namespace asn1pp::arch {

// -- Compile-time SIMD level ------------------------------------------------

#ifdef __AVX2__
inline constexpr simd_level compile_level = simd_level::avx2;
#elif defined(__SSE4_2__)
inline constexpr simd_level compile_level = simd_level::sse42;
#elif defined(__ARM_NEON) || defined(__aarch64__)
inline constexpr simd_level compile_level = simd_level::neon;
#else
inline constexpr simd_level compile_level = simd_level::scalar;
#endif

// -- Single-value operations (always scalar — no SIMD benefit) --------------

inline uint64_t load_u64_be(const uint8_t* p, size_t n) noexcept {
    return scalar::load_u64_be(p, n);
}

inline void store_u64_be(uint8_t* p, uint64_t v, size_t n) noexcept {
    scalar::store_u64_be(p, v, n);
}

// -- Batch load --------------------------------------------------------------

inline void batch_load_u64_be(const uint8_t* const* ptrs, const size_t* sizes,
                              uint64_t* out, size_t count) noexcept {
    const auto level = available_simd_level();
    if (level == simd_level::avx2) {
#if defined(__AVX2__)
        avx2::batch_load_u64_be(ptrs, sizes, out, count);
#elif defined(__SSE4_2__)
        sse42::batch_load_u64_be(ptrs, sizes, out, count);
#else
        scalar::batch_load_u64_be(ptrs, sizes, out, count);
#endif
    } else if (level == simd_level::sse42) {
#if defined(__SSE4_2__)
        sse42::batch_load_u64_be(ptrs, sizes, out, count);
#else
        scalar::batch_load_u64_be(ptrs, sizes, out, count);
#endif
    } else if (level == simd_level::neon) {
#if defined(__ARM_NEON) || defined(__aarch64__)
        neon::batch_load_u64_be(ptrs, sizes, out, count);
#else
        scalar::batch_load_u64_be(ptrs, sizes, out, count);
#endif
    } else {
        scalar::batch_load_u64_be(ptrs, sizes, out, count);
    }
}

// -- Batch store -------------------------------------------------------------

inline void batch_store_u64_be(uint8_t** ptrs, const uint64_t* values,
                               const size_t* sizes, size_t count) noexcept {
    const auto level = available_simd_level();
    if (level == simd_level::avx2) {
#if defined(__AVX2__)
        avx2::batch_store_u64_be(ptrs, values, sizes, count);
#elif defined(__SSE4_2__)
        sse42::batch_store_u64_be(ptrs, values, sizes, count);
#else
        scalar::batch_store_u64_be(ptrs, values, sizes, count);
#endif
    } else if (level == simd_level::sse42) {
#if defined(__SSE4_2__)
        sse42::batch_store_u64_be(ptrs, values, sizes, count);
#else
        scalar::batch_store_u64_be(ptrs, values, sizes, count);
#endif
    } else if (level == simd_level::neon) {
#if defined(__ARM_NEON) || defined(__aarch64__)
        neon::batch_store_u64_be(ptrs, values, sizes, count);
#else
        scalar::batch_store_u64_be(ptrs, values, sizes, count);
#endif
    } else {
        scalar::batch_store_u64_be(ptrs, values, sizes, count);
    }
}

// -- Copy bytes --------------------------------------------------------------

inline size_t copy_bytes(uint8_t* dst, const uint8_t* src, size_t n) noexcept {
    const auto level = available_simd_level();
    if (level == simd_level::avx2) {
#if defined(__AVX2__)
        return avx2::copy_bytes(dst, src, n);
#elif defined(__SSE4_2__)
        return sse42::copy_bytes(dst, src, n);
#else
        return scalar::copy_bytes(dst, src, n);
#endif
    } else if (level == simd_level::sse42) {
#if defined(__SSE4_2__)
        return sse42::copy_bytes(dst, src, n);
#else
        return scalar::copy_bytes(dst, src, n);
#endif
    } else if (level == simd_level::neon) {
#if defined(__ARM_NEON) || defined(__aarch64__)
        return neon::copy_bytes(dst, src, n);
#else
        return scalar::copy_bytes(dst, src, n);
#endif
    } else {
        return scalar::copy_bytes(dst, src, n);
    }
}

// -- Compare tags ------------------------------------------------------------

inline void compare_tags(const uint8_t* tag_bytes, uint8_t expected,
                         size_t count, bool* results) noexcept {
    const auto level = available_simd_level();
    if (level == simd_level::avx2) {
#if defined(__AVX2__)
        avx2::compare_tags(tag_bytes, expected, count, results);
#elif defined(__SSE4_2__)
        sse42::compare_tags(tag_bytes, expected, count, results);
#else
        scalar::compare_tags(tag_bytes, expected, count, results);
#endif
    } else if (level == simd_level::sse42) {
#if defined(__SSE4_2__)
        sse42::compare_tags(tag_bytes, expected, count, results);
#else
        scalar::compare_tags(tag_bytes, expected, count, results);
#endif
    } else if (level == simd_level::neon) {
#if defined(__ARM_NEON) || defined(__aarch64__)
        neon::compare_tags(tag_bytes, expected, count, results);
#else
        scalar::compare_tags(tag_bytes, expected, count, results);
#endif
    } else {
        scalar::compare_tags(tag_bytes, expected, count, results);
    }
}

}  // namespace asn1pp::arch
