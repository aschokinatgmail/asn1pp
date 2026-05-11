#pragma once

#include <cstdint>
#include <string>
#include <span>
#include <vector>
#include <cassert>

#include "codec/result.hpp"
#include "arch/simd.hpp"

#ifndef ASN1PP_NO_TEXT_CODECS

namespace asn1pp::jer {

class jer_encoder {
public:
    result<void> encode_integer(int64_t value, std::string& out) {
        pre_value(out);
        out += std::to_string(value);
        post_value();
        return result<void>::ok();
    }

    result<void> encode_boolean(bool value, std::string& out) {
        pre_value(out);
        out += value ? "true" : "false";
        post_value();
        return result<void>::ok();
    }

    result<void> encode_null(std::string& out) {
        pre_value(out);
        out += "null";
        post_value();
        return result<void>::ok();
    }

    result<void> encode_octet_string(std::span<const uint8_t> data, std::string& out) {
        pre_value(out);
        out += '"';
        base64_encode(data, out);
        out += '"';
        post_value();
        return result<void>::ok();
    }

    result<void> encode_bit_string(std::span<const uint8_t> data, uint8_t unused_bits, std::string& out) {
        pre_value(out);
        open_object(out);
        add_key("value", out);
        out += '"';
        base64_encode(data, out);
        out += '"';
        post_value();
        add_key("unused", out);
        out += std::to_string(unused_bits);
        close_object(out);
        post_value();
        return result<void>::ok();
    }

    result<void> encode_enumerated(int64_t index, const char* const* names, std::string& out) {
        pre_value(out);
        out += '"';
        out += names[static_cast<size_t>(index)];
        out += '"';
        post_value();
        return result<void>::ok();
    }

    result<void> encode_oid(const char* dotted, std::string& out) {
        pre_value(out);
        out += '"';
        out += dotted;
        out += '"';
        post_value();
        return result<void>::ok();
    }

    void open_object(std::string& out) {
        out += '{';
        need_comma_stack_.push_back(false);
    }

    void close_object(std::string& out) {
        assert(!need_comma_stack_.empty());
        need_comma_stack_.pop_back();
        out += '}';
        if (!need_comma_stack_.empty()) {
            need_comma_stack_.back() = true;
        }
    }

    void open_array(std::string& out) {
        out += '[';
        need_comma_stack_.push_back(false);
    }

    void close_array(std::string& out) {
        assert(!need_comma_stack_.empty());
        need_comma_stack_.pop_back();
        out += ']';
        if (!need_comma_stack_.empty()) {
            need_comma_stack_.back() = true;
        }
    }

    error_code flush_encode() noexcept { return error_code::ok; }

    void add_key(const char* key, std::string& out) {
        pre_value(out);
        out += '"';
        out += key;
        out += '"';
        out += ':';
    }

private:
    static constexpr char base64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::vector<bool> need_comma_stack_;

    void pre_value(std::string& out) {
        if (!need_comma_stack_.empty() && need_comma_stack_.back()) {
            out += ',';
            need_comma_stack_.back() = false;
        }
    }

    void post_value() {
        if (!need_comma_stack_.empty()) {
            need_comma_stack_.back() = true;
        }
    }

    static void base64_encode(std::span<const uint8_t> data, std::string& out) {
        const size_t len = data.size();
        if (len == 0) return;

        size_t i = 0;
        while (i < len) {
            uint32_t a = (i < len) ? data[i] : 0; ++i;
            uint32_t b = (i < len) ? data[i] : 0; ++i;
            uint32_t c = (i < len) ? data[i] : 0; ++i;
            uint32_t triple = (a << 16) | (b << 8) | c;
            out += base64_alphabet[(triple >> 18) & 0x3F];
            out += base64_alphabet[(triple >> 12) & 0x3F];
            out += base64_alphabet[(triple >> 6) & 0x3F];
            out += base64_alphabet[triple & 0x3F];
        }

        const size_t mod = len % 3;
        if (mod == 1) {
            out[out.size() - 1] = '=';
            out[out.size() - 2] = '=';
        } else if (mod == 2) {
            out[out.size() - 1] = '=';
        }
    }
};

} // namespace asn1pp::jer

#endif  // ASN1PP_NO_TEXT_CODECS
