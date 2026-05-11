#include "codec/per/decoder.hpp"
#include "codec/per/uper_decoder.hpp"
#include "codec/arch_codec.hpp"
#include "codec/config.hpp"
#include "arch/simd.hpp"

#include <cstdint>
#include <cstddef>

namespace asn1pp::per {

size_t per_aligned_decoder::batch_size() noexcept {
    const auto level = arch::available_simd_level();
    switch (level) {
    case arch::simd_level::avx2:  return 4;
    case arch::simd_level::sse42: return 2;
    case arch::simd_level::neon:  return 2;
    default:                      return 1;
    }
}

error_code per_aligned_decoder::flush_decode() noexcept {
    if (pending_.empty()) {
        return error_code::ok;
    }

    const uint8_t* ptrs[kMaxBatchSize];
    size_t sizes[kMaxBatchSize];
    int64_t results[kMaxBatchSize];
    error_code errors[kMaxBatchSize];

    auto drained = pending_.drain();
    const size_t count = drained.size();
    for (size_t i = 0; i < count; ++i) {
        ptrs[i] = drained[i].value_buf;
        sizes[i] = drained[i].size;
    }

    arch_codec::batch_decode_integers(ptrs, sizes, results, errors, count);

    error_code first_err = error_code::ok;
    for (size_t i = 0; i < count; ++i) {
        if (errors[i] != error_code::ok) {
            first_err = errors[i];
            break;
        }
    }

    return first_err;
}

size_t uper_decoder::batch_size() noexcept {
    const auto level = arch::available_simd_level();
    switch (level) {
    case arch::simd_level::avx2:  return 4;
    case arch::simd_level::sse42: return 2;
    case arch::simd_level::neon:  return 2;
    default:                      return 1;
    }
}

error_code uper_decoder::flush_decode() noexcept {
    if (pending_.empty()) {
        return error_code::ok;
    }

    const uint8_t* ptrs[kMaxBatchSize];
    size_t sizes[kMaxBatchSize];
    int64_t results[kMaxBatchSize];
    error_code errors[kMaxBatchSize];

    auto drained = pending_.drain();
    const size_t count = drained.size();
    for (size_t i = 0; i < count; ++i) {
        ptrs[i] = drained[i].value_buf;
        sizes[i] = drained[i].size;
    }

    arch_codec::batch_decode_integers(ptrs, sizes, results, errors, count);

    error_code first_err = error_code::ok;
    for (size_t i = 0; i < count; ++i) {
        if (errors[i] != error_code::ok) {
            first_err = errors[i];
            break;
        }
    }

    return first_err;
}

}  // namespace asn1pp::per
