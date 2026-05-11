#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace asn1pp::arch::scalar {

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
    for (size_t i = 0; i < count; ++i) {
        out[i] = load_u64_be(ptrs[i], sizes[i]);
    }
}

inline void batch_store_u64_be(uint8_t** ptrs, const uint64_t* values,
                               const size_t* sizes, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        store_u64_be(ptrs[i], values[i], sizes[i]);
    }
}

inline size_t copy_bytes(uint8_t* dst, const uint8_t* src, size_t n) noexcept {
    if (n == 0) return 0;
    std::memcpy(dst, src, n);
    return n;
}

inline void compare_tags(const uint8_t* tag_bytes, uint8_t expected,
                         size_t count, bool* results) noexcept {
    for (size_t i = 0; i < count; ++i) {
        results[i] = (tag_bytes[i] == expected);
    }
}

}  // namespace asn1pp::arch::scalar
