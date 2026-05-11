#pragma once

#include <cstdint>
#include <cstddef>
#include <span>

#include "codec/result.hpp"
#include "codec/traits.hpp"
#include "codec/batch_buffer.hpp"
#include "buffer/buffer_view.hpp"

namespace asn1pp::ber {

/// BER encoder — encodes ASN.1 types to BER wire format per ITU-T X.690.
///
/// All encoding functions write into the caller-provided mutable buffer
/// (accessed via buffer_view with const_cast, matching the pattern used
/// throughout the codec library). Each function advances `buf` past the
/// bytes it wrote.
///
/// No heap allocation in hot paths; integer encoding uses a stack-allocated
/// buffer of at most 9 bytes.
class ber_encoder {
public:
    // ── Primitive type encoders ────────────────────────────────────────

    /// Encode INTEGER (universal tag 2) in two's complement, big-endian.
    /// Uses SIMD-accelerated batch encoding transparently.
    result<void> encode_integer(int64_t value, buffer_view& buf);

    /// Encode BOOLEAN (universal tag 1). true = 0xFF, false = 0x00.
    result<void> encode_boolean(bool value, buffer_view& buf);

    /// Encode NULL (universal tag 5). Always writes 05 00.
    result<void> encode_null(buffer_view& buf);

    /// Encode OCTET STRING (universal tag 4). Raw bytes, no transformation.
    /// Uses SIMD-accelerated copy for payloads > 32 bytes.
    result<void> encode_octet_string(std::span<const uint8_t> data, buffer_view& buf);

    /// Encode BIT STRING (universal tag 3).
    /// First content octet = unused_bits (0-7), followed by data bytes.
    result<void> encode_bit_string(std::span<const uint8_t> data, uint8_t unused_bits, buffer_view& buf);

    /// Encode ENUMERATED (universal tag 10). Same format as INTEGER.
    result<void> encode_enumerated(int64_t value, buffer_view& buf);

    /// Encode OBJECT IDENTIFIER (universal tag 6).
    /// Accepts pre-encoded subidentifier form; no OID decoding is performed.
    result<void> encode_oid(std::span<const uint8_t> encoded_oid, buffer_view& buf);

    // ── Constructed type helpers ───────────────────────────────────────

    /// Write a constructed SEQUENCE/SET header (tag + definite length).
    /// Returns the number of header bytes written, so the caller can
    /// track content boundaries.
    /// Use this when the content length is known ahead of time.
    result<size_t> encode_sequence_header(const tag& t, size_t content_length, buffer_view& buf);

    /// Write the end-of-content marker (00 00) for indefinite-length encoding.
    result<void> encode_sequence_end(buffer_view& buf);

    // ── Generic TLV encoder ────────────────────────────────────────────

    /// Write a complete TLV (tag + length + value). Delegates to tlv.hpp.
    result<void> encode_tlv(const tag& t, std::span<const uint8_t> value, buffer_view& buf);

    // ── Batch drain ────────────────────────────────────────────────────

    /// Drain any pending batch encode operations via SIMD.
    /// Idempotent — calling when no pending ops is a no-op.
    /// Returns error_code::ok on success, or the first error encountered.
    error_code flush_encode() noexcept;

private:
    /// Compute the minimal two's complement big-endian encoding of an integer.
    /// Writes bytes to `out` and returns the number of bytes written.
    /// `out` must have room for at least 9 bytes.
    static size_t encode_integer_bytes(int64_t value, uint8_t* out);

    // ── Batch buffer ───────────────────────────────────────────────────

    static constexpr size_t kMaxBatchSize = 4;

    struct pending_integer {
        uint8_t value_buf[8];
        size_t size;
        int64_t value;
    };

    batch_buffer<pending_integer, kMaxBatchSize> pending_;

    /// Number of consecutive encode_integer() calls before flushing.
    static size_t batch_size() noexcept;
};

} // namespace asn1pp::ber
