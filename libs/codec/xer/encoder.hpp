#pragma once

#include <cstdint>
#include <string>
#include <span>
#include <vector>

#include "codec/result.hpp"
#include "arch/simd.hpp"

#ifndef ASN1PP_NO_TEXT_CODECS

namespace asn1pp::xer {

std::string base64_encode(const uint8_t* data, size_t len);

class xer_encoder {
public:
    static result<void> encode_integer(const char* name, int64_t value, std::string& out) {
        out += '<';
        out += name;
        out += '>';
        out += std::to_string(value);
        out += "</";
        out += name;
        out += '>';
        return result<void>::ok();
    }

    static result<void> encode_boolean(const char* name, bool value, std::string& out) {
        out += '<';
        out += name;
        out += '>';
        if (value) {
            out += "<true/>";
        } else {
            out += "<false/>";
        }
        out += "</";
        out += name;
        out += '>';
        return result<void>::ok();
    }

    static result<void> encode_null(const char* name, std::string& out) {
        out += '<';
        out += name;
        out += "/>";
        return result<void>::ok();
    }

    static result<void> encode_octet_string(const char* name, std::span<const uint8_t> data, std::string& out) {
        out += '<';
        out += name;
        out += '>';
        if (!data.empty()) {
            out += base64_encode(data.data(), data.size());
        }
        out += "</";
        out += name;
        out += '>';
        return result<void>::ok();
    }

    static result<void> encode_bit_string(const char* name, std::span<const uint8_t> data, std::string& out) {
        return encode_octet_string(name, data, out);
    }

    static result<void> encode_enumerated(const char* name, int64_t index, const char* const* names, std::string& out) {
        out += '<';
        out += name;
        out += '>';
        if (names != nullptr) {
            out += names[index];
        } else {
            out += std::to_string(index);
        }
        out += "</";
        out += name;
        out += '>';
        return result<void>::ok();
    }

    static void open_element(const char* name, std::string& out) {
        out += '<';
        out += name;
        out += '>';
    }

    static void close_element(const char* name, std::string& out) {
        out += "</";
        out += name;
        out += '>';
    }

    static void empty_element(const char* name, std::string& out) {
        out += '<';
        out += name;
        out += "/>";
    }

    static void element_with_text(const char* name, const char* text, std::string& out) {
        out += '<';
        out += name;
        out += '>';
        out += text;
        out += "</";
        out += name;
        out += '>';
    }

    static error_code flush_encode() noexcept { return error_code::ok; }

    static void open_sequence(const char* name, std::string& out) {
        open_element(name, out);
    }

    static void close_sequence(const char* name, std::string& out) {
        close_element(name, out);
    }

    static void open_choice(const char* name, std::string& out) {
        open_element(name, out);
    }

    static void close_choice(const char* name, std::string& out) {
        close_element(name, out);
    }

    static void open_sequence_of(const char* name, std::string& out) {
        open_element(name, out);
    }

    static void close_sequence_of(const char* name, std::string& out) {
        close_element(name, out);
    }
};

} // namespace asn1pp::xer

#endif  // ASN1PP_NO_TEXT_CODECS
