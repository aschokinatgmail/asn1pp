#include "codec/ber/encoder.hpp"
#include "codec/ber/tlv.hpp"
#include "codec/arch_codec.hpp"
#include "codec/config.hpp"
#include "arch/simd.hpp"

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <span>
#include <algorithm>

namespace asn1pp::ber {

size_t ber_encoder::batch_size() noexcept {
    const auto level = arch::available_simd_level();
    switch (level) {
    case arch::simd_level::avx2:  return 4;
    case arch::simd_level::sse42: return 2;
    case arch::simd_level::neon:  return 2;
    default:                      return 1;
    }
}

size_t ber_encoder::encode_integer_bytes(int64_t value, uint8_t* out) {
    if (value == 0) {
        out[0] = 0x00;
        return 1;
    }

    uint64_t uval = static_cast<uint64_t>(value);
    const bool negative = (value < 0);

    uint8_t raw[8];
    for (int i = 0; i < 8; ++i) {
        raw[7 - i] = static_cast<uint8_t>(uval & 0xFF);
        uval >>= 8;
    }

    const uint8_t sign_byte = negative ? 0xFF : 0x00;
    size_t start = 0;
    while (start < 7 && raw[start] == sign_byte) {
        ++start;
    }

    bool needs_extra = false;
    if (!negative && (raw[start] & 0x80)) {
        needs_extra = true;
    } else if (negative && !(raw[start] & 0x80)) {
        needs_extra = true;
    }

    size_t out_idx = 0;
    if (needs_extra) {
        out[out_idx++] = negative ? 0xFF : 0x00;
    }

    const size_t meaningful = 8 - start;
    std::copy(raw + start, raw + 8, out + out_idx);
    out_idx += meaningful;

    return out_idx;
}

result<void> ber_encoder::encode_integer(int64_t value, buffer_view& buf) {
    uint8_t int_buf[9];
    const size_t value_len = encode_integer_bytes(value, int_buf);

    const size_t bs = batch_size();

    pending_integer slot = {};
    std::memcpy(slot.value_buf, int_buf, value_len);
    slot.size  = value_len;
    slot.value = value;
    pending_.push(slot);

    if (pending_.size() == bs) {
        auto drained = pending_.drain();
        uint8_t* ptrs[kMaxBatchSize];
        size_t sizes[kMaxBatchSize];
        int64_t values[kMaxBatchSize];
        error_code errors[kMaxBatchSize];

        for (size_t i = 0; i < bs; ++i) {
            ptrs[i]   = drained[i].value_buf;
            sizes[i]   = drained[i].size;
            values[i]  = drained[i].value;
        }

        arch_codec::batch_encode_integers(values, sizes, ptrs, errors, bs);

        if (errors[bs - 1] != error_code::ok)
            return result<void>::err(errors[bs - 1]);
    }

    return ber::encode_tlv(buf, make_universal(universal_tag::integer),
                           std::span<const uint8_t>(int_buf, value_len));
}

result<void> ber_encoder::encode_boolean(bool value, buffer_view& buf) {
    const uint8_t byte_val = value ? 0xFF : 0x00;
    return ber::encode_tlv(buf, make_universal(universal_tag::boolean),
                           std::span<const uint8_t>(&byte_val, 1));
}

result<void> ber_encoder::encode_null(buffer_view& buf) {
    return ber::encode_tlv(buf, make_universal(universal_tag::null),
                           std::span<const uint8_t>{});
}

result<void> ber_encoder::encode_octet_string(std::span<const uint8_t> data, buffer_view& buf) {
    const auto t = make_universal(universal_tag::octet_string);
    const size_t total_needed = encoded_tag_size(t) +
                                encoded_length_size(data.size()) + data.size();
    if (buf.size() < total_needed)
        return result<void>::err(error_code::buffer_overflow);

    uint8_t* out = const_cast<uint8_t*>(buf.data());
    size_t pos = 0;

    {
        buffer_view tag_view(out + pos, buf.size() - pos);
        auto r = encode_tag(tag_view, t);
        if (r.is_err()) return r;
        pos += encoded_tag_size(t);
    }

    {
        buffer_view len_view(out + pos, buf.size() - pos);
        auto r = encode_length(len_view, data.size());
        if (r.is_err()) return r;
        pos += encoded_length_size(data.size());
    }

    if (data.size() > 32) {
        arch::copy_bytes(out + pos, data.data(), data.size());
    } else if (data.size() > 0) {
        std::memcpy(out + pos, data.data(), data.size());
    }
    pos += data.size();

    buf = buf.subview(pos, buf.size() - pos);
    return result<void>::ok();
}

result<void> ber_encoder::encode_bit_string(std::span<const uint8_t> data, uint8_t unused_bits,
                                            buffer_view& buf) {
    const size_t total_value_len = 1 + data.size();
    const size_t total_needed = encoded_tag_size(make_universal(universal_tag::bit_string)) +
                                encoded_length_size(total_value_len) + total_value_len;
    if (buf.size() < total_needed)
        return result<void>::err(error_code::buffer_overflow);

    uint8_t* out = const_cast<uint8_t*>(buf.data());
    size_t pos = 0;

    {
        buffer_view tag_view(out + pos, buf.size() - pos);
        auto r = encode_tag(tag_view, make_universal(universal_tag::bit_string));
        if (r.is_err()) return r;
        pos += encoded_tag_size(make_universal(universal_tag::bit_string));
    }

    {
        buffer_view len_view(out + pos, buf.size() - pos);
        auto r = encode_length(len_view, total_value_len);
        if (r.is_err()) return r;
        pos += encoded_length_size(total_value_len);
    }

    out[pos++] = unused_bits;

    if (!data.empty()) {
        std::memcpy(out + pos, data.data(), data.size());
        pos += data.size();
    }

    buf = buf.subview(pos, buf.size() - pos);
    return result<void>::ok();
}

result<void> ber_encoder::encode_enumerated(int64_t value, buffer_view& buf) {
    uint8_t int_buf[9];
    const size_t value_len = encode_integer_bytes(value, int_buf);
    return ber::encode_tlv(buf, make_universal(universal_tag::enumerated),
                           std::span<const uint8_t>(int_buf, value_len));
}

result<void> ber_encoder::encode_oid(std::span<const uint8_t> encoded_oid, buffer_view& buf) {
    return ber::encode_tlv(buf, make_universal(universal_tag::object_identifier), encoded_oid);
}

result<size_t> ber_encoder::encode_sequence_header(const tag& t, size_t content_length,
                                                   buffer_view& buf) {
    const size_t total_needed = encoded_tag_size(t) + encoded_length_size(content_length);
    if (buf.size() < total_needed)
        return result<size_t>::err(error_code::buffer_overflow);

    uint8_t* out = const_cast<uint8_t*>(buf.data());
    size_t pos = 0;

    {
        buffer_view tag_view(out + pos, buf.size() - pos);
        auto r = encode_tag(tag_view, t);
        if (r.is_err()) return result<size_t>::err(r.error());
        pos += encoded_tag_size(t);
    }

    {
        buffer_view len_view(out + pos, buf.size() - pos);
        auto r = encode_length(len_view, content_length);
        if (r.is_err()) return result<size_t>::err(r.error());
        pos += encoded_length_size(content_length);
    }

    buf = buf.subview(pos, buf.size() - pos);
    return result<size_t>::ok(pos);
}

result<void> ber_encoder::encode_sequence_end(buffer_view& buf) {
    return encode_end_of_content(buf);
}

result<void> ber_encoder::encode_tlv(const tag& t, std::span<const uint8_t> value,
                                     buffer_view& buf) {
    return ber::encode_tlv(buf, t, value);
}

error_code ber_encoder::flush_encode() noexcept {
    if (pending_.empty())
        return error_code::ok;

    uint8_t* ptrs[kMaxBatchSize];
    size_t sizes[kMaxBatchSize];
    int64_t values[kMaxBatchSize];
    error_code errors[kMaxBatchSize];

    auto drained = pending_.flush();
    const size_t count = drained.size();
    for (size_t i = 0; i < count; ++i) {
        ptrs[i]  = drained[i].value_buf;
        sizes[i]  = drained[i].size;
        values[i] = drained[i].value;
    }

    arch_codec::batch_encode_integers(values, sizes, ptrs, errors, count);

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
