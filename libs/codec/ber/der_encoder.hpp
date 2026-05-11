#pragma once

#include <cstdint>
#include <cstddef>
#include <span>

#include "codec/result.hpp"
#include "codec/traits.hpp"
#include "buffer/buffer_view.hpp"
#include "codec/ber/encoder.hpp"
#include "codec/ber/decoder.hpp"

namespace asn1pp::ber {

class der_encoder {
public:
    der_encoder() = default;

    result<void> encode_integer(int64_t value, buffer_view& buf);
    result<void> encode_boolean(bool value, buffer_view& buf);
    result<void> encode_null(buffer_view& buf);
    result<void> encode_octet_string(std::span<const uint8_t> data, buffer_view& buf);
    result<void> encode_bit_string(std::span<const uint8_t> data, uint8_t unused_bits,
                                   buffer_view& buf);
    result<void> encode_enumerated(int64_t value, buffer_view& buf);

    result<void> validate(std::span<const uint8_t> encoded);
    result<void> validate_boolean_der(std::span<const uint8_t> content) const;
    result<void> validate_integer_der(std::span<const uint8_t> content) const;
    result<void> validate_length_definite(std::span<const uint8_t> tlv);
    result<void> validate_primitive_string(std::span<const uint8_t> tlv);
    result<void> validate_bit_string_unused_bits(std::span<const uint8_t> content) const;

    static bool is_integer_minimal(std::span<const uint8_t> content);
    static size_t count_sign_leading_octets(std::span<const uint8_t> content,
                                            uint8_t sign_byte);

    // ── Batch drain ────────────────────────────────────────────────────

    /// Drain any pending batch encode operations via SIMD.
    /// Delegates to the internal ber_encoder's flush_encode().
    /// Idempotent — calling when no pending ops is a no-op.
    error_code flush_encode() noexcept { return ber_enc_.flush_encode(); }

private:
    ber_encoder ber_enc_;
    ber_decoder ber_dec_;
};

} // namespace asn1pp::ber
