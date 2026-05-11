#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <vector>
#include <utility>

#include "codec/result.hpp"
#include "codec/traits.hpp"
#include "codec/batch_buffer.hpp"
#include "codec/config.hpp"
#include "buffer/buffer_view.hpp"

namespace asn1pp::ber {

/// BER decoder — decodes BER wire format back to ASN.1 types per ITU-T X.690.
///
/// All decoding functions read from the caller-provided buffer_view and
/// advance `buf` past the bytes consumed. No heap allocation in hot paths;
/// INTEGER/BOOLEAN/ENUMERATED values are decoded on the stack.
///
/// Error handling via result<T>: truncated input returns buffer_underflow,
/// wrong tag returns invalid_tag, values exceeding int64_t range return
/// value_out_of_range.
///
/// SIMD batch decoding: decode_integer() internally uses SIMD-accelerated
/// batch_decode_integers() when the pending batch buffer is full. The
/// batching is transparent — the public API is unchanged. Callers can
/// explicitly drain pending batches via flush_decode().
class ber_decoder {
public:
    /// Decoded TLV triple: tag, length, and a span over the value bytes.
    /// The value span points into the caller's buffer; it is valid only
    /// until the underlying buffer is destroyed.
    struct tlv_data {
        tag t;
        size_t length;
        std::span<const uint8_t> value;
    };

    // ── Primitive type decoders ────────────────────────────────────────

    /// Decode INTEGER (universal tag 2) from two's complement big-endian.
    /// When SIMD is available, value decoding may be batched internally.
    result<int64_t> decode_integer(buffer_view& buf);

    /// Decode BOOLEAN (universal tag 1).
    /// BER: any non-zero content byte = true, 0x00 = false.
    result<bool> decode_boolean(buffer_view& buf);

    /// Decode NULL (universal tag 5). Expects 05 00.
    result<void> decode_null(buffer_view& buf);

#ifndef ASN1PP_EMBEDDED
    /// Decode OCTET STRING (universal tag 4). Returns raw content bytes.
    /// Uses SIMD-accelerated copy for payloads > 32 bytes.
    result<std::vector<uint8_t>> decode_octet_string(buffer_view& buf);
#else
    struct octet_string_result {
        uint8_t data[ASN1PP_MAX_PDU];
        size_t length;
    };
    result<octet_string_result> decode_octet_string(buffer_view& buf);
#endif

#ifndef ASN1PP_EMBEDDED
    /// Decode BIT STRING (universal tag 3).
    /// Returns {data, unused_bits} where unused_bits is the number of
    /// unused bits (0-7) in the last data byte.
    result<std::pair<std::vector<uint8_t>, uint8_t>> decode_bit_string(buffer_view& buf);
#else
    struct bit_string_result {
        uint8_t data[ASN1PP_MAX_PDU];
        size_t length;
        uint8_t unused_bits;
    };
    result<bit_string_result> decode_bit_string(buffer_view& buf);
#endif

    /// Decode ENUMERATED (universal tag 10). Same wire format as INTEGER.
    result<int64_t> decode_enumerated(buffer_view& buf);

#ifndef ASN1PP_EMBEDDED
    /// Decode OBJECT IDENTIFIER (universal tag 6). Returns raw subidentifier bytes.
    result<std::vector<uint8_t>> decode_oid(buffer_view& buf);
#else
    result<octet_string_result> decode_oid(buffer_view& buf);
#endif

    // ── Batch drain ────────────────────────────────────────────────────

    /// Drain any pending batch decode operations via SIMD.
    /// Idempotent — calling when no pending ops is a no-op.
    /// Returns error_code::ok on success, or the first error encountered.
    error_code flush_decode() noexcept;

    // ── Constructed type helpers ───────────────────────────────────────

    /// Decode a constructed SEQUENCE/SET header (tag + definite length).
    /// Returns the tag and sets content_length to the number of content bytes
    /// that follow. The caller must then decode content_length bytes of
    /// inner elements from buf.
    result<tag> decode_sequence_header(buffer_view& buf, size_t& content_length);

    /// Verify and consume the end-of-content marker (00 00).
    result<void> decode_sequence_end(buffer_view& buf);

    // ── Generic TLV decoder ────────────────────────────────────────────

    /// Decode a full TLV (tag + length + value). The returned value span
    /// points into the caller's buffer.
    result<tlv_data> decode_tlv(buffer_view& buf);

    /// Skip past the current TLV, positioning buf at the next element.
    /// Works with definite-length encoding only.
    result<void> skip_tlv(buffer_view& buf);

    /// Peek at the next tag without advancing buf.
    result<tag> peek_tag(buffer_view& buf);

private:
    /// Decode two's complement big-endian bytes into int64_t.
    /// `bytes` must have length 1..8.
    static int64_t decode_integer_value(std::span<const uint8_t> bytes);

    // ── Batch buffer ───────────────────────────────────────────────────

    static constexpr size_t kMaxBatchSize = 4;  // AVX2 max, also works for SSE4.2/NEON=2

    struct pending_integer {
        uint8_t value_buf[8];
        size_t size;
    };

    batch_buffer<pending_integer, kMaxBatchSize> pending_;

    /// Number of consecutive decode_integer() calls before flushing.
    /// 4 for AVX2, 2 for SSE4.2/NEON, 1 for scalar (pass-through).
    static size_t batch_size() noexcept;
};

} // namespace asn1pp::ber
