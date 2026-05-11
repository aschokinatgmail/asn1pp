#include "codec/ber/decoder.hpp"
#include "codec/ber/tlv.hpp"
#include "codec/arch_codec.hpp"
#include "codec/config.hpp"
#include "arch/simd.hpp"

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <span>
#include <vector>
#include <utility>

namespace asn1pp::ber {

// ── Batch size dispatch ─────────────────────────────────────────────────────

size_t ber_decoder::batch_size() noexcept {
    const auto level = arch::available_simd_level();
    switch (level) {
    case arch::simd_level::avx2:  return 4;
    case arch::simd_level::sse42: return 2;
    case arch::simd_level::neon:  return 2;
    default:                      return 1;  // scalar pass-through
    }
}

// ============================================================================
// Helper: decode two's complement big-endian integer value
// ============================================================================

int64_t ber_decoder::decode_integer_value(std::span<const uint8_t> bytes) {
    const size_t len = bytes.size();
    const bool negative = (bytes[0] & 0x80) != 0;

    int64_t result = negative ? static_cast<int64_t>(-1) : 0;

    for (size_t i = 0; i < len; ++i) {
        result = (result << 8) | static_cast<int64_t>(bytes[i]);
    }

    return result;
}

// ============================================================================
// Primitive type decoders
// ============================================================================

result<int64_t> ber_decoder::decode_integer(buffer_view& buf) {
    auto tag_res = decode_tag(buf);
    if (tag_res.is_err()) return result<int64_t>::err(tag_res.error());
    const tag& t = tag_res.value();

    if (t != make_universal(universal_tag::integer)) {
        return result<int64_t>::err(error_code::invalid_tag);
    }

    auto len_res = decode_length(buf);
    if (len_res.is_err()) return result<int64_t>::err(len_res.error());
    const size_t val_len = len_res.value();

    if (val_len == 0) {
        return result<int64_t>::err(error_code::invalid_length);
    }
    if (buf.size() < val_len) {
        return result<int64_t>::err(error_code::buffer_underflow);
    }
    if (val_len > 8) {
        return result<int64_t>::err(error_code::value_out_of_range);
    }

    const size_t bs = batch_size();

    // Accumulate value bytes into ring buffer
    pending_integer slot = {};
    std::memcpy(slot.value_buf, buf.data(), val_len);
    slot.size = val_len;
    pending_.push(slot);

    buf = buf.subview(val_len, buf.size() - val_len);

    if (pending_.size() == bs) {
        // Batch full: SIMD batch-decode all entries
        auto drained = pending_.drain();
        const uint8_t* ptrs[4];
        size_t sizes[4];
        int64_t results[4];
        error_code errors[4];

        for (size_t i = 0; i < bs; ++i) {
            ptrs[i] = drained[i].value_buf;
            sizes[i] = drained[i].size;
        }

        arch_codec::batch_decode_integers(ptrs, sizes, results, errors, bs);

        if (errors[bs - 1] != error_code::ok)
            return result<int64_t>::err(errors[bs - 1]);
        return result<int64_t>::ok(results[bs - 1]);
    }

    // Not full yet: scalar decode for immediate return
    return result<int64_t>::ok(
        decode_integer_value(std::span<const uint8_t>(slot.value_buf, val_len)));
}

result<bool> ber_decoder::decode_boolean(buffer_view& buf) {
    auto tag_res = decode_tag(buf);
    if (tag_res.is_err()) return result<bool>::err(tag_res.error());

    if (tag_res.value() != make_universal(universal_tag::boolean)) {
        return result<bool>::err(error_code::invalid_tag);
    }

    auto len_res = decode_length(buf);
    if (len_res.is_err()) return result<bool>::err(len_res.error());

    if (len_res.value() != 1) {
        return result<bool>::err(error_code::invalid_length);
    }
    if (buf.empty()) {
        return result<bool>::err(error_code::buffer_underflow);
    }

    const bool value = (buf[0] != 0x00);
    buf = buf.subview(1, buf.size() - 1);

    return result<bool>::ok(value);
}

result<void> ber_decoder::decode_null(buffer_view& buf) {
    auto tag_res = decode_tag(buf);
    if (tag_res.is_err()) return result<void>::err(tag_res.error());

    if (tag_res.value() != make_universal(universal_tag::null)) {
        return result<void>::err(error_code::invalid_tag);
    }

    auto len_res = decode_length(buf);
    if (len_res.is_err()) return result<void>::err(len_res.error());

    if (len_res.value() != 0) {
        return result<void>::err(error_code::invalid_length);
    }

    return result<void>::ok();
}

#ifndef ASN1PP_EMBEDDED
result<std::vector<uint8_t>> ber_decoder::decode_octet_string(buffer_view& buf) {
    auto tag_res = decode_tag(buf);
    if (tag_res.is_err())
        return result<std::vector<uint8_t>>::err(tag_res.error());

    if (tag_res.value() != make_universal(universal_tag::octet_string)) {
        return result<std::vector<uint8_t>>::err(error_code::invalid_tag);
    }

    auto len_res = decode_length(buf);
    if (len_res.is_err())
        return result<std::vector<uint8_t>>::err(len_res.error());

    const size_t val_len = len_res.value();
    if (buf.size() < val_len) {
        return result<std::vector<uint8_t>>::err(error_code::buffer_underflow);
    }

    std::vector<uint8_t> octets(val_len);
    if (val_len > 32) {
        arch::copy_bytes(octets.data(), buf.data(), val_len);
    } else if (val_len > 0) {
        std::memcpy(octets.data(), buf.data(), val_len);
    }
    buf = buf.subview(val_len, buf.size() - val_len);

    return result<std::vector<uint8_t>>::ok(std::move(octets));
}
#else
result<ber_decoder::octet_string_result> ber_decoder::decode_octet_string(buffer_view& buf) {
    static_assert(ASN1PP_MAX_PDU > 0, "ASN1PP_MAX_PDU must be positive in embedded mode");
    auto tag_res = decode_tag(buf);
    if (tag_res.is_err())
        return result<ber_decoder::octet_string_result>::err(tag_res.error());

    if (tag_res.value() != make_universal(universal_tag::octet_string)) {
        return result<ber_decoder::octet_string_result>::err(error_code::invalid_tag);
    }

    auto len_res = decode_length(buf);
    if (len_res.is_err())
        return result<ber_decoder::octet_string_result>::err(len_res.error());

    const size_t val_len = len_res.value();
    if (buf.size() < val_len) {
        return result<ber_decoder::octet_string_result>::err(error_code::buffer_underflow);
    }
    if (val_len > ASN1PP_MAX_PDU) {
        return result<ber_decoder::octet_string_result>::err(error_code::value_out_of_range);
    }

    ber_decoder::octet_string_result out;
    out.length = val_len;
    if (val_len > 32) {
        arch::copy_bytes(out.data, buf.data(), val_len);
    } else if (val_len > 0) {
        std::memcpy(out.data, buf.data(), val_len);
    }
    buf = buf.subview(val_len, buf.size() - val_len);

    return result<ber_decoder::octet_string_result>::ok(out);
}
#endif

#ifndef ASN1PP_EMBEDDED
result<std::pair<std::vector<uint8_t>, uint8_t>> ber_decoder::decode_bit_string(buffer_view& buf) {
    auto tag_res = decode_tag(buf);
    if (tag_res.is_err())
        return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(tag_res.error());

    if (tag_res.value() != make_universal(universal_tag::bit_string)) {
        return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::invalid_tag);
    }

    auto len_res = decode_length(buf);
    if (len_res.is_err())
        return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(len_res.error());

    const size_t val_len = len_res.value();
    if (buf.size() < val_len) {
        return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::buffer_underflow);
    }

    if (val_len < 1) {
        return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::invalid_length);
    }

    const uint8_t unused_bits = buf[0];
    if (unused_bits > 7) {
        return result<std::pair<std::vector<uint8_t>, uint8_t>>::err(error_code::invalid_length);
    }

    const size_t data_len = val_len - 1;
    std::vector<uint8_t> data(data_len);
    if (data_len > 0) {
        std::memcpy(data.data(), buf.data() + 1, data_len);
    }

    buf = buf.subview(val_len, buf.size() - val_len);

    return result<std::pair<std::vector<uint8_t>, uint8_t>>::ok(
        std::make_pair(std::move(data), unused_bits));
}
#else
result<ber_decoder::bit_string_result> ber_decoder::decode_bit_string(buffer_view& buf) {
    static_assert(ASN1PP_MAX_PDU > 0, "ASN1PP_MAX_PDU must be positive in embedded mode");
    auto tag_res = decode_tag(buf);
    if (tag_res.is_err())
        return result<bit_string_result>::err(tag_res.error());

    if (tag_res.value() != make_universal(universal_tag::bit_string)) {
        return result<bit_string_result>::err(error_code::invalid_tag);
    }

    auto len_res = decode_length(buf);
    if (len_res.is_err())
        return result<bit_string_result>::err(len_res.error());

    const size_t val_len = len_res.value();
    if (buf.size() < val_len) {
        return result<bit_string_result>::err(error_code::buffer_underflow);
    }

    if (val_len < 1) {
        return result<bit_string_result>::err(error_code::invalid_length);
    }

    const uint8_t unused_bits = buf[0];
    if (unused_bits > 7) {
        return result<bit_string_result>::err(error_code::invalid_length);
    }

    const size_t data_len = val_len - 1;
    if (data_len > ASN1PP_MAX_PDU) {
        return result<bit_string_result>::err(error_code::value_out_of_range);
    }

    ber_decoder::bit_string_result out;
    out.length = data_len;
    out.unused_bits = unused_bits;
    if (data_len > 0) {
        std::memcpy(out.data, buf.data() + 1, data_len);
    }

    buf = buf.subview(val_len, buf.size() - val_len);

    return result<bit_string_result>::ok(out);
}
#endif

result<int64_t> ber_decoder::decode_enumerated(buffer_view& buf) {
    auto tag_res = decode_tag(buf);
    if (tag_res.is_err()) return result<int64_t>::err(tag_res.error());

    if (tag_res.value() != make_universal(universal_tag::enumerated)) {
        return result<int64_t>::err(error_code::invalid_tag);
    }

    auto len_res = decode_length(buf);
    if (len_res.is_err()) return result<int64_t>::err(len_res.error());

    const size_t val_len = len_res.value();
    if (val_len == 0) {
        return result<int64_t>::err(error_code::invalid_length);
    }
    if (buf.size() < val_len) {
        return result<int64_t>::err(error_code::buffer_underflow);
    }
    if (val_len > 8) {
        return result<int64_t>::err(error_code::value_out_of_range);
    }

    uint8_t val_buf[8];
    std::memcpy(val_buf, buf.data(), val_len);
    buf = buf.subview(val_len, buf.size() - val_len);

    return result<int64_t>::ok(
        decode_integer_value(std::span<const uint8_t>(val_buf, val_len)));
}

#ifndef ASN1PP_EMBEDDED
result<std::vector<uint8_t>> ber_decoder::decode_oid(buffer_view& buf) {
    auto tag_res = decode_tag(buf);
    if (tag_res.is_err())
        return result<std::vector<uint8_t>>::err(tag_res.error());

    if (tag_res.value() != make_universal(universal_tag::object_identifier)) {
        return result<std::vector<uint8_t>>::err(error_code::invalid_tag);
    }

    auto len_res = decode_length(buf);
    if (len_res.is_err())
        return result<std::vector<uint8_t>>::err(len_res.error());

    const size_t val_len = len_res.value();
    if (buf.size() < val_len) {
        return result<std::vector<uint8_t>>::err(error_code::buffer_underflow);
    }

    std::vector<uint8_t> oid_bytes(val_len);
    if (val_len > 0) {
        std::memcpy(oid_bytes.data(), buf.data(), val_len);
    }
    buf = buf.subview(val_len, buf.size() - val_len);

    return result<std::vector<uint8_t>>::ok(std::move(oid_bytes));
}
#else
result<ber_decoder::octet_string_result> ber_decoder::decode_oid(buffer_view& buf) {
    auto tag_res = decode_tag(buf);
    if (tag_res.is_err())
        return result<octet_string_result>::err(tag_res.error());

    if (tag_res.value() != make_universal(universal_tag::object_identifier)) {
        return result<octet_string_result>::err(error_code::invalid_tag);
    }

    auto len_res = decode_length(buf);
    if (len_res.is_err())
        return result<octet_string_result>::err(len_res.error());

    const size_t val_len = len_res.value();
    if (buf.size() < val_len) {
        return result<octet_string_result>::err(error_code::buffer_underflow);
    }
    if (val_len > ASN1PP_MAX_PDU) {
        return result<octet_string_result>::err(error_code::value_out_of_range);
    }

ber_decoder::octet_string_result out;
    out.length = val_len;
    if (val_len > 0) {
        std::memcpy(out.data, buf.data(), val_len);
    }
    buf = buf.subview(val_len, buf.size() - val_len);

    return result<octet_string_result>::ok(out);
}
#endif

// ============================================================================
// Constructed type helpers
// ============================================================================

result<tag> ber_decoder::decode_sequence_header(buffer_view& buf, size_t& content_length) {
    auto tag_res = decode_tag(buf);
    if (tag_res.is_err()) return result<tag>::err(tag_res.error());

    const tag& t = tag_res.value();
    if (!t.constructed) {
        return result<tag>::err(error_code::invalid_tag);
    }

    auto len_res = decode_length(buf);
    if (len_res.is_err()) return result<tag>::err(len_res.error());

    content_length = len_res.value();

    if (buf.size() < content_length) {
        return result<tag>::err(error_code::buffer_underflow);
    }

    return result<tag>::ok(t);
}

result<void> ber_decoder::decode_sequence_end(buffer_view& buf) {
    if (buf.size() < 2) {
        return result<void>::err(error_code::buffer_underflow);
    }

    if (buf[0] != 0x00 || buf[1] != 0x00) {
        return result<void>::err(error_code::invalid_tag);
    }

    buf = buf.subview(2, buf.size() - 2);
    return result<void>::ok();
}

// ============================================================================
// Generic TLV and utility
// ============================================================================

result<ber_decoder::tlv_data> ber_decoder::decode_tlv(buffer_view& buf) {
    auto tag_res = decode_tag(buf);
    if (tag_res.is_err())
        return result<tlv_data>::err(tag_res.error());

    tag t = tag_res.value();

    auto len_res = decode_length(buf);
    if (len_res.is_err())
        return result<tlv_data>::err(len_res.error());

    const size_t val_len = len_res.value();
    if (buf.size() < val_len) {
        return result<tlv_data>::err(error_code::buffer_underflow);
    }

    tlv_data result_data;
    result_data.t = t;
    result_data.length = val_len;
    result_data.value = std::span<const uint8_t>(buf.data(), val_len);

    buf = buf.subview(val_len, buf.size() - val_len);

    return result<tlv_data>::ok(result_data);
}

result<void> ber_decoder::skip_tlv(buffer_view& buf) {
    auto tag_res = decode_tag(buf);
    if (tag_res.is_err()) return result<void>::err(tag_res.error());

    auto len_res = decode_length(buf);
    if (len_res.is_err()) return result<void>::err(len_res.error());

    const size_t val_len = len_res.value();
    if (buf.size() < val_len) {
        return result<void>::err(error_code::buffer_underflow);
    }

    buf = buf.subview(val_len, buf.size() - val_len);
    return result<void>::ok();
}

result<tag> ber_decoder::peek_tag(buffer_view& buf) {
    buffer_view save = buf;
    auto tag_res = decode_tag(save);
    if (tag_res.is_err()) return result<tag>::err(tag_res.error());
    return tag_res;
}

// ============================================================================
// Batch drain
// ============================================================================

error_code ber_decoder::flush_decode() noexcept {
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

} // namespace asn1pp::ber
