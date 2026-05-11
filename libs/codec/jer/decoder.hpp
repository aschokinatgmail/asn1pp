#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cassert>
#include <cctype>
#include <charconv>

#include "codec/result.hpp"
#include "arch/simd.hpp"

#ifndef ASN1PP_NO_TEXT_CODECS

namespace asn1pp::jer {

enum class json_value_type : uint8_t {
    nil,
    boolean,
    number,
    string_val,
    object,
    array,
};

struct json_value {
    json_value_type type = json_value_type::nil;
    bool bool_val = false;
    int64_t number_val = 0;
    std::string string_val;
    std::shared_ptr<std::unordered_map<std::string, json_value>> object_val;
    std::shared_ptr<std::vector<json_value>> array_val;
};

class jer_json_parser {
public:
    explicit jer_json_parser(std::string_view input) : input_(input), pos_(0) {}

    result<json_value> parse_value() {
        skip_whitespace();
        if (pos_ >= input_.size()) {
            return result<json_value>::err(error_code::parse_error);
        }
        switch (input_[pos_]) {
            case '{': return parse_object();
            case '[': return parse_array();
            case '"': return parse_string();
            case 't': case 'f': return parse_bool_or_null();
            case 'n': return parse_bool_or_null();
            case '-': case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                return parse_number();
            default:
                return result<json_value>::err(error_code::parse_error);
        }
    }

private:
    std::string_view input_;
    size_t pos_;

    void skip_whitespace() {
        while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
            ++pos_;
        }
    }

    result<json_value> parse_object() {
        assert(input_[pos_] == '{');
        ++pos_;
        json_value val;
        val.type = json_value_type::object;
        val.object_val = std::make_shared<std::unordered_map<std::string, json_value>>();
        skip_whitespace();
        if (pos_ < input_.size() && input_[pos_] == '}') {
            ++pos_;
            return result<json_value>::ok(std::move(val));
        }
        for (;;) {
            skip_whitespace();
            if (pos_ >= input_.size() || input_[pos_] != '"') {
                return result<json_value>::err(error_code::parse_error);
            }
            ++pos_;
            std::string key;
            if (!parse_string_content(key)) {
                return result<json_value>::err(error_code::parse_error);
            }
            skip_whitespace();
            if (pos_ >= input_.size() || input_[pos_] != ':') {
                return result<json_value>::err(error_code::parse_error);
            }
            ++pos_;
            skip_whitespace();
            auto field_val = parse_value();
            if (field_val.is_err()) {
                return field_val;
            }
            (*val.object_val)[std::move(key)] = std::move(field_val.value());
            skip_whitespace();
            if (pos_ >= input_.size()) {
                return result<json_value>::err(error_code::parse_error);
            }
            if (input_[pos_] == '}') {
                ++pos_;
                break;
            }
            if (input_[pos_] != ',') {
                return result<json_value>::err(error_code::parse_error);
            }
            ++pos_;
        }
        return result<json_value>::ok(std::move(val));
    }

    result<json_value> parse_array() {
        assert(input_[pos_] == '[');
        ++pos_;
        json_value val;
        val.type = json_value_type::array;
        val.array_val = std::make_shared<std::vector<json_value>>();
        skip_whitespace();
        if (pos_ < input_.size() && input_[pos_] == ']') {
            ++pos_;
            return result<json_value>::ok(std::move(val));
        }
        for (;;) {
            skip_whitespace();
            auto elem = parse_value();
            if (elem.is_err()) {
                return elem;
            }
            val.array_val->push_back(std::move(elem.value()));
            skip_whitespace();
            if (pos_ >= input_.size()) {
                return result<json_value>::err(error_code::parse_error);
            }
            if (input_[pos_] == ']') {
                ++pos_;
                break;
            }
            if (input_[pos_] != ',') {
                return result<json_value>::err(error_code::parse_error);
            }
            ++pos_;
        }
        return result<json_value>::ok(std::move(val));
    }

    result<json_value> parse_string() {
        assert(input_[pos_] == '"');
        ++pos_;
        json_value val;
        val.type = json_value_type::string_val;
        if (!parse_string_content(val.string_val)) {
            return result<json_value>::err(error_code::parse_error);
        }
        return result<json_value>::ok(std::move(val));
    }

    bool parse_string_content(std::string& out) {
        while (pos_ < input_.size()) {
            if (input_[pos_] == '"') {
                ++pos_;
                return true;
            }
            if (input_[pos_] == '\\') {
                ++pos_;
                if (pos_ >= input_.size()) return false;
                switch (input_[pos_]) {
                    case '"':  out += '"';  break;
                    case '\\': out += '\\'; break;
                    case '/':  out += '/';  break;
                    case 'n':  out += '\n'; break;
                    case 'r':  out += '\r'; break;
                    case 't':  out += '\t'; break;
                    default:   return false;
                }
                ++pos_;
            } else {
                out += input_[pos_++];
            }
        }
        return false;
    }

    result<json_value> parse_number() {
        size_t end = pos_;
        if (end < input_.size() && input_[end] == '-') ++end;
        while (end < input_.size() && std::isdigit(static_cast<unsigned char>(input_[end]))) {
            ++end;
        }
        if (end == pos_ || (end == pos_ + 1 && input_[pos_] == '-')) {
            return result<json_value>::err(error_code::parse_error);
        }
        std::string_view num_str = input_.substr(pos_, end - pos_);
        pos_ = end;
        int64_t val = 0;
        auto [ptr, ec] = std::from_chars(num_str.data(), num_str.data() + num_str.size(), val);
        if (ec != std::errc{}) {
            return result<json_value>::err(error_code::parse_error);
        }
        json_value jv;
        jv.type = json_value_type::number;
        jv.number_val = val;
        return result<json_value>::ok(std::move(jv));
    }

    result<json_value> parse_bool_or_null() {
        if (pos_ + 4 <= input_.size() && input_.substr(pos_, 4) == "true") {
            pos_ += 4;
            json_value jv;
            jv.type = json_value_type::boolean;
            jv.bool_val = true;
            return result<json_value>::ok(std::move(jv));
        }
        if (pos_ + 5 <= input_.size() && input_.substr(pos_, 5) == "false") {
            pos_ += 5;
            json_value jv;
            jv.type = json_value_type::boolean;
            jv.bool_val = false;
            return result<json_value>::ok(std::move(jv));
        }
        if (pos_ + 4 <= input_.size() && input_.substr(pos_, 4) == "null") {
            pos_ += 4;
            json_value jv;
            jv.type = json_value_type::nil;
            return result<json_value>::ok(std::move(jv));
        }
        return result<json_value>::err(error_code::parse_error);
    }
};

class jer_decoder {
public:
    result<int64_t> decode_integer(std::string_view json) {
        jer_json_parser parser(json);
        auto val = parser.parse_value();
        if (val.is_err()) return result<int64_t>::err(val.error());
        if (val.value().type != json_value_type::number) {
            return result<int64_t>::err(error_code::parse_error);
        }
        return result<int64_t>::ok(val.value().number_val);
    }

    result<bool> decode_boolean(std::string_view json) {
        jer_json_parser parser(json);
        auto val = parser.parse_value();
        if (val.is_err()) return result<bool>::err(val.error());
        if (val.value().type != json_value_type::boolean) {
            return result<bool>::err(error_code::parse_error);
        }
        return result<bool>::ok(val.value().bool_val);
    }

    result<void> decode_null(std::string_view json) {
        jer_json_parser parser(json);
        auto val = parser.parse_value();
        if (val.is_err()) return result<void>::err(val.error());
        if (val.value().type != json_value_type::nil) {
            return result<void>::err(error_code::parse_error);
        }
        return result<void>::ok();
    }

    result<std::vector<uint8_t>> decode_octet_string(std::string_view json) {
        jer_json_parser parser(json);
        auto val = parser.parse_value();
        if (val.is_err()) return result<std::vector<uint8_t>>::err(val.error());
        if (val.value().type != json_value_type::string_val) {
            return result<std::vector<uint8_t>>::err(error_code::parse_error);
        }
        return base64_decode(val.value().string_val);
    }

    result<int64_t> decode_enumerated(std::string_view json, const char* const* names, size_t count) {
        jer_json_parser parser(json);
        auto val = parser.parse_value();
        if (val.is_err()) return result<int64_t>::err(val.error());
        if (val.value().type != json_value_type::string_val) {
            return result<int64_t>::err(error_code::parse_error);
        }
        for (size_t i = 0; i < count; ++i) {
            if (val.value().string_val == names[i]) {
                return result<int64_t>::ok(static_cast<int64_t>(i));
            }
        }
        return result<int64_t>::err(error_code::parse_error);
    }

    result<std::string> decode_oid(std::string_view json) {
        jer_json_parser parser(json);
        auto val = parser.parse_value();
        if (val.is_err()) return result<std::string>::err(val.error());
        if (val.value().type != json_value_type::string_val) {
            return result<std::string>::err(error_code::parse_error);
        }
        return result<std::string>::ok(val.value().string_val);
    }

    result<json_value> parse_value(std::string_view json) {
        jer_json_parser parser(json);
        return parser.parse_value();
    }

    result<json_value> get_field(const json_value& obj, const char* key) {
        if (obj.type != json_value_type::object || !obj.object_val) {
            return result<json_value>::err(error_code::parse_error);
        }
        auto it = obj.object_val->find(key);
        if (it == obj.object_val->end()) {
            return result<json_value>::err(error_code::parse_error);
        }
        return result<json_value>::ok(it->second);
    }

    error_code flush_decode() noexcept {
        return error_code::ok;
    }

private:
    static constexpr char base64_decode_chars[256] = {
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
        52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-1,-1,-1,
        -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
        15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
        -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
        41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    };

    static result<std::vector<uint8_t>> base64_decode(std::string_view encoded) {
        if (encoded.empty()) {
            return result<std::vector<uint8_t>>::ok(std::vector<uint8_t>{});
        }
        std::vector<uint8_t> output;
        output.reserve((encoded.size() / 4) * 3);
        size_t i = 0;
        while (i < encoded.size()) {
            char c0 = encoded[i]; if (c0 == '=') break;
            char c1 = (i + 1 < encoded.size()) ? encoded[i + 1] : '=';
            char c2 = (i + 2 < encoded.size()) ? encoded[i + 2] : '=';
            char c3 = (i + 3 < encoded.size()) ? encoded[i + 3] : '=';
            i += 4;
            int v0 = base64_decode_chars[static_cast<unsigned char>(c0)];
            int v1 = base64_decode_chars[static_cast<unsigned char>(c1)];
            int v2 = base64_decode_chars[static_cast<unsigned char>(c2)];
            int v3 = base64_decode_chars[static_cast<unsigned char>(c3)];
            if (v0 < 0 || v1 < 0) {
                return result<std::vector<uint8_t>>::err(error_code::parse_error);
            }
            output.push_back(static_cast<uint8_t>((v0 << 2) | (v1 >> 4)));
            if (v2 < 0) {
                if (v3 >= 0) return result<std::vector<uint8_t>>::err(error_code::parse_error);
                break;
            }
            output.push_back(static_cast<uint8_t>((v1 << 4) | (v2 >> 2)));
            if (v3 < 0) break;
            output.push_back(static_cast<uint8_t>((v2 << 6) | v3));
        }
        return result<std::vector<uint8_t>>::ok(std::move(output));
    }
};

} // namespace asn1pp::jer

#endif  // ASN1PP_NO_TEXT_CODECS
