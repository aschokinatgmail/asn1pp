#pragma once

#include <cstdint>
#include <cstddef>
#include <span>

#include "codec/result.hpp"
#include "codec/traits.hpp"
#include "codec/tagged_types.hpp"
#include "codec/ber/tlv.hpp"
#include "buffer/buffer_view.hpp"

namespace asn1pp::ber {

template<typename InnerEncoder, typename T>
result<void> encode_implicit_tagged(InnerEncoder& enc, const implicit_tagged<T>& val,
                                    buffer_view& buf) {
    tag outer_t = tag_for_type_v<implicit_tagged<T>>;
    if constexpr (std::is_same_v<T, int64_t>) {
        uint8_t int_buf[9];
        size_t value_len = 0;
        {
            int64_t v = val.value;
            if (v == 0) {
                int_buf[0] = 0x00;
                value_len = 1;
            } else {
                uint64_t uval = static_cast<uint64_t>(v);
                bool negative = (v < 0);
                uint8_t raw[8];
                for (int i = 0; i < 8; ++i) { raw[7 - i] = static_cast<uint8_t>(uval & 0xFF); uval >>= 8; }
                uint8_t sign_byte = negative ? 0xFF : 0x00;
                size_t start = 0;
                while (start < 7 && raw[start] == sign_byte) ++start;
                bool needs_extra = (!negative && (raw[start] & 0x80)) || (negative && !(raw[start] & 0x80));
                if (needs_extra) int_buf[value_len++] = negative ? 0xFF : 0x00;
                size_t meaningful = 8 - start;
                for (size_t i = 0; i < meaningful; ++i) int_buf[value_len++] = raw[start + i];
            }
        }
        return encode_tlv(buf, outer_t, std::span<const uint8_t>(int_buf, value_len));
    }
    if constexpr (std::is_same_v<T, bool>) {
        uint8_t byte_val = val.value ? 0xFF : 0x00;
        return encode_tlv(buf, outer_t, std::span<const uint8_t>(&byte_val, 1));
    }
    if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
        return encode_tlv(buf, outer_t, std::span<const uint8_t>(val.value.data(), val.value.size()));
    }
    if constexpr (std::is_same_v<T, std::string>) {
        return encode_tlv(buf, outer_t, std::span<const uint8_t>(
            reinterpret_cast<const uint8_t*>(val.value.data()), val.value.size()));
    }
    if constexpr (std::is_same_v<T, std::monostate>) {
        return encode_tlv(buf, outer_t, std::span<const uint8_t>{});
    }
    (void)enc;
    (void)val;
    (void)buf;
    return result<void>::err(error_code::parse_error);
}

template<typename InnerEncoder, typename T>
result<void> encode_explicit_tagged(InnerEncoder& enc, const explicit_tagged<T>& val,
                                    buffer_view& buf) {
    tag outer_t = tag_for_type_v<explicit_tagged<T>>;
    std::vector<uint8_t> inner_buf(4096);
    buffer_view inner_view = buffer_view(inner_buf.data(), inner_buf.size());

    auto inner_res = enc.encode(val.value, inner_view);
    if (inner_res.is_err()) return inner_res;

    size_t inner_len = inner_buf.size() - inner_view.size();
    size_t total_needed = encoded_tag_size(outer_t) + encoded_length_size(inner_len) + inner_len;
    if (buf.size() < total_needed) return result<void>::err(error_code::buffer_overflow);

    uint8_t* out = const_cast<uint8_t*>(buf.data());
    size_t pos = 0;
    {
        buffer_view tag_view(out + pos, buf.size() - pos);
        auto r = encode_tag(tag_view, outer_t);
        if (r.is_err()) return r;
        pos += encoded_tag_size(outer_t);
    }
    {
        buffer_view len_view(out + pos, buf.size() - pos);
        auto r = encode_length(len_view, inner_len);
        if (r.is_err()) return r;
        pos += encoded_length_size(inner_len);
    }
    std::memcpy(out + pos, inner_buf.data(), inner_len);
    pos += inner_len;
    buf = buf.subview(pos, buf.size() - pos);
    return result<void>::ok();
}

}  // namespace asn1pp::ber
