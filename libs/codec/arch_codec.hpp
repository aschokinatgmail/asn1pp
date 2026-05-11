#pragma once

///
/// arch_codec.hpp — SIMD-accelerated codec primitives
///
/// Higher-level batch operations that bridge the low-level arch:: SIMD
/// dispatch primitives with ASN.1-specific semantics (sign extension,
/// integer encoding rules, buffer overflow checks).
///
/// All functions are noexcept. Errors are reported through the per-PDU
/// error_code array, not through exceptions or error-return values.
///
/// Uses arch:: dispatch primitives internally for SIMD acceleration:
///   - arch::batch_load_u64_be   (big-endian byte array → uint64_t)
///   - arch::batch_store_u64_be  (uint64_t → big-endian byte array)
///   - arch::copy_bytes          (memcpy with SIMD)
///   - arch::compare_tags        (byte comparison with SIMD)
///

#include <cstdint>
#include <cstddef>

#include "arch/simd.hpp"
#include "codec/result.hpp"

namespace asn1pp::arch_codec {

///
/// Decode N integer PDUs from big-endian wire format to int64_t.
///
/// Uses arch::batch_load_u64_be() for SIMD-accelerated byte loading,
/// then applies two's complement sign extension based on the most
/// significant bit of the original bytes.
///
/// @param bufs    Array of N pointers, each pointing to a big-endian
///                integer value (tag and length already stripped)
/// @param sizes   Number of bytes in each integer PDU (0 = error,
///                > 8 = buffer_overflow)
/// @param out     Output: decoded int64_t values (only valid where
///                errors[i] == error_code::ok)
/// @param errors  Output: per-PDU error status
/// @param count   Number of PDUs to process
///
inline void batch_decode_integers(const uint8_t* const* bufs,
                                  const size_t* sizes,
                                  int64_t* out,
                                  error_code* errors,
                                  size_t count) noexcept {
    // Single call to dispatch — let SIMD handle the valid PDUs
    uint64_t raw_values[256];  // caller guarantees count ≤ reasonable batch size
    arch::batch_load_u64_be(bufs, sizes, raw_values, count);

    for (size_t i = 0; i < count; ++i) {
        const size_t n = sizes[i];

        if (n == 0) {
            errors[i] = error_code::invalid_length;
            out[i]    = 0;
            continue;
        }

        if (n > 8) {
            errors[i] = error_code::buffer_overflow;
            out[i]    = 0;
            continue;
        }

        errors[i] = error_code::ok;

        // Sign-extend: if the most significant bit of the n-byte value
        // is set, the number is negative and we must fill the upper
        // (8-n) bytes with 0xFF.
        const bool negative = (raw_values[i] >> (n * 8 - 1)) & 1;

        if (negative && n < 8) {
            // OR with 0xFF bytes above the valid bits
            out[i] = static_cast<int64_t>(raw_values[i]
                                          | (~0ULL << (n * 8)));
        } else {
            out[i] = static_cast<int64_t>(raw_values[i]);
        }
    }
}

///
/// Encode N int64_t values to big-endian wire format.
///
/// Validates that each value fits in the requested byte count,
/// then uses arch::batch_store_u64_be() for SIMD-accelerated
/// output.
///
/// @param values  Array of N int64_t values to encode
/// @param sizes   Requested byte count for each PDU (1-8).
///                Caller controls encoding width.
/// @param bufs    Array of N output pointers. Each must be at least
///                sizes[i] bytes.
/// @param errors  Output: per-PDU error status
/// @param count   Number of PDUs to process
///
inline void batch_encode_integers(const int64_t* values,
                                  const size_t* sizes,
                                  uint8_t** bufs,
                                  error_code* errors,
                                  size_t count) noexcept {
    uint64_t raw_values[256];

    for (size_t i = 0; i < count; ++i) {
        const size_t n = sizes[i];

        if (n == 0) {
            errors[i]    = error_code::invalid_length;
            raw_values[i] = 0;
            continue;
        }

        if (n > 8) {
            errors[i]    = error_code::buffer_overflow;
            raw_values[i] = 0;
            continue;
        }

        // Check whether the signed value fits in n bytes of two's complement.
        // For n bytes, the range is [−2^(n·8−1), 2^(n·8−1)−1].
        if (n < 8) {
            const int64_t max_val = (1LL << (n * 8 - 1)) - 1;
            const int64_t min_val = -(1LL << (n * 8 - 1));

            if (values[i] < min_val || values[i] > max_val) {
                errors[i]    = error_code::buffer_overflow;
                raw_values[i] = 0;
                continue;
            }
        }
        // n == 8: any int64_t fits — no range check needed.

        errors[i]    = error_code::ok;
        raw_values[i] = static_cast<uint64_t>(values[i]);
    }

    arch::batch_store_u64_be(bufs, raw_values, sizes, count);
}

///
/// Copy N octet strings into a contiguous destination buffer.
///
/// Computes cumulative destination offsets and copies each string
/// using arch::copy_bytes() for SIMD acceleration.
///
/// @param srcs        Array of N source pointers
/// @param src_sizes   Byte counts for each source
/// @param dst         Contiguous destination buffer (must be large enough
///                    to hold the sum of all src_sizes)
/// @param dst_offsets Output: byte offset of each string within dst.
///                    dst_offsets[0] is always 0.
/// @param count       Number of strings to copy
///
inline void batch_copy_octet_strings(const uint8_t* const* srcs,
                                     const size_t* src_sizes,
                                     uint8_t* dst,
                                     size_t* dst_offsets,
                                     size_t count) noexcept {
    if (count == 0) return;

    dst_offsets[0] = 0;

    for (size_t i = 0; i < count; ++i) {
        if (src_sizes[i] > 0) {
            arch::copy_bytes(dst + dst_offsets[i], srcs[i], src_sizes[i]);
        }

        if (i + 1 < count) {
            dst_offsets[i + 1] = dst_offsets[i] + src_sizes[i];
        }
    }
}

///
/// Compare N tag bytes against an expected tag value.
///
/// Thin wrapper over arch::compare_tags() with tag-specific
/// documentation. In BER/DER processing, this is used to
/// batch-verify that all PDUs in a sequence have the expected
/// tag byte.
///
/// @param tag_bytes Array of N tag bytes (first byte of each PDU's
///                  TLV header)
/// @param expected  Expected tag byte value
/// @param count     Number of tag bytes to compare
/// @param results   Output: results[i] = true if tag_bytes[i] == expected
///
inline void batch_compare_tags(const uint8_t* tag_bytes,
                               uint8_t expected,
                               size_t count,
                               bool* results) noexcept {
    arch::compare_tags(tag_bytes, expected, count, results);
}

}  // namespace asn1pp::arch_codec
