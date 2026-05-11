#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <utility>

#include "codec/result.hpp"
#include "arch/simd.hpp"

#ifndef ASN1PP_NO_TEXT_CODECS

namespace asn1pp::xer {

std::vector<uint8_t> base64_decode(const std::string& encoded);

struct xml_element {
    std::string name;
    std::string content;
    bool is_empty = false;
    std::vector<xml_element> children;
};

class xml_parser {
public:
    result<xml_element> parse(std::string_view input) {
        pos_ = 0;
        input_ = input;
        skip_whitespace();
        if (pos_ >= input_.size()) {
            return result<xml_element>::err(error_code::parse_error);
        }
        return parse_element();
    }

private:
    std::string_view input_;
    size_t pos_ = 0;

    void skip_whitespace() {
        while (pos_ < input_.size() && (input_[pos_] == ' ' || input_[pos_] == '\t' ||
               input_[pos_] == '\n' || input_[pos_] == '\r')) {
            ++pos_;
        }
    }

    result<xml_element> parse_element() {
        skip_whitespace();
        if (pos_ >= input_.size() || input_[pos_] != '<') {
            return result<xml_element>::err(error_code::parse_error);
        }

        ++pos_;

        xml_element elem;
        auto name_result = parse_name();
        if (name_result.is_err()) {
            return result<xml_element>::err(error_code::parse_error);
        }
        elem.name = name_result.value();

        skip_whitespace();

        if (pos_ >= input_.size()) {
            return result<xml_element>::err(error_code::parse_error);
        }

        if (input_[pos_] == '/') {
            ++pos_;
            if (pos_ >= input_.size() || input_[pos_] != '>') {
                return result<xml_element>::err(error_code::parse_error);
            }
            ++pos_;
            elem.is_empty = true;
            return result<xml_element>::ok(std::move(elem));
        }

        if (input_[pos_] != '>') {
            return result<xml_element>::err(error_code::parse_error);
        }
        ++pos_;

        while (pos_ < input_.size()) {
            if (input_[pos_] == '<' && pos_ + 1 < input_.size() && input_[pos_ + 1] == '/') {
                pos_ += 2;
                skip_whitespace();
                auto close_name = parse_name();
                if (close_name.is_err() || close_name.value() != elem.name) {
                    return result<xml_element>::err(error_code::parse_error);
                }
                skip_whitespace();
                if (pos_ >= input_.size() || input_[pos_] != '>') {
                    return result<xml_element>::err(error_code::parse_error);
                }
                ++pos_;
                return result<xml_element>::ok(std::move(elem));
            }

            if (input_[pos_] == '<') {
                auto child_result = parse_element();
                if (child_result.is_err()) {
                    return child_result;
                }
                elem.children.push_back(std::move(child_result.value()));
            } else {
                std::string text;
                while (pos_ < input_.size() && input_[pos_] != '<') {
                    if (input_[pos_] == '&') {
                        auto entity = parse_entity();
                        if (entity.is_err()) {
                            return result<xml_element>::err(error_code::parse_error);
                        }
                        text += entity.value();
                    } else {
                        text += input_[pos_];
                        ++pos_;
                    }
                }
                elem.content += text;
            }
        }

        return result<xml_element>::err(error_code::parse_error);
    }

    result<std::string> parse_name() {
        skip_whitespace();
        std::string name;
        while (pos_ < input_.size() && (std::isalnum(static_cast<unsigned char>(input_[pos_])) ||
               input_[pos_] == '_' || input_[pos_] == '-' || input_[pos_] == '.' ||
               input_[pos_] == ':')) {
            name += input_[pos_];
            ++pos_;
        }
        if (name.empty()) {
            return result<std::string>::err(error_code::parse_error);
        }
        return result<std::string>::ok(std::move(name));
    }

    result<char> parse_entity() {
        if (pos_ >= input_.size() || input_[pos_] != '&') {
            return result<char>::err(error_code::parse_error);
        }

        size_t entity_start = pos_;
        ++pos_;

        std::string ref;
        while (pos_ < input_.size() && input_[pos_] != ';') {
            ref += input_[pos_];
            ++pos_;
        }

        if (pos_ >= input_.size() || input_[pos_] != ';') {
            pos_ = entity_start;
            return result<char>::err(error_code::parse_error);
        }
        ++pos_;

        if (ref == "lt") return result<char>::ok('<');
        if (ref == "gt") return result<char>::ok('>');
        if (ref == "amp") return result<char>::ok('&');
        if (ref == "quot") return result<char>::ok('"');
        if (ref == "apos") return result<char>::ok('\'');

        return result<char>::err(error_code::parse_error);
    }
};

class xer_decoder {
public:
    static result<int64_t> decode_integer(std::string_view xml) {
        xml_parser parser;
        auto parse_result = parser.parse(xml);
        if (parse_result.is_err()) {
            return result<int64_t>::err(error_code::parse_error);
        }
        auto& elem = parse_result.value();
        if (elem.content.empty()) {
            return result<int64_t>::err(error_code::parse_error);
        }
        return result<int64_t>::ok(std::strtoll(elem.content.c_str(), nullptr, 10));
    }

    static int64_t decode_integer_from_string(const std::string& s) {
        return std::strtoll(s.c_str(), nullptr, 10);
    }

    static result<bool> decode_boolean(std::string_view xml) {
        xml_parser parser;
        auto parse_result = parser.parse(xml);
        if (parse_result.is_err() || parse_result.value().children.empty()) {
            return result<bool>::err(error_code::parse_error);
        }
        auto& child = parse_result.value().children[0];
        if (child.name == "true") return result<bool>::ok(true);
        if (child.name == "false") return result<bool>::ok(false);
        return result<bool>::err(error_code::parse_error);
    }

    static result<std::vector<uint8_t>> decode_octet_string(std::string_view xml) {
        xml_parser parser;
        auto parse_result = parser.parse(xml);
        if (parse_result.is_err()) {
            return result<std::vector<uint8_t>>::err(error_code::parse_error);
        }
        auto& elem = parse_result.value();
        if (elem.content.empty()) {
            return result<std::vector<uint8_t>>::ok(std::vector<uint8_t>{});
        }
        return result<std::vector<uint8_t>>::ok(base64_decode(elem.content));
    }

    static result<std::vector<uint8_t>> decode_bit_string(std::string_view xml) {
        return decode_octet_string(xml);
    }

    static result<int64_t> decode_enumerated(std::string_view xml, const char* const* names) {
        xml_parser parser;
        auto parse_result = parser.parse(xml);
        if (parse_result.is_err()) {
            return result<int64_t>::err(error_code::parse_error);
        }
        auto& elem = parse_result.value();
        if (names != nullptr) {
            for (int64_t i = 0; names[i] != nullptr; ++i) {
                if (elem.content == names[i]) {
                    return result<int64_t>::ok(i);
                }
            }
        }
        return result<int64_t>::err(error_code::parse_error);
    }

    static result<std::string> decode_sequence_field_content(std::string_view xml, const char* field_name) {
        xml_parser parser;
        auto parse_result = parser.parse(xml);
        if (parse_result.is_err()) {
            return result<std::string>::err(error_code::parse_error);
        }
        auto& elem = parse_result.value();
        for (auto& child : elem.children) {
            if (child.name == field_name) {
                if (child.children.size() == 1 && child.children[0].is_empty) {
                    return result<std::string>::ok(std::string("<") + child.children[0].name + "/>");
                }
                return result<std::string>::ok(child.content);
            }
        }
        return result<std::string>::err(error_code::parse_error);
    }

    static result<std::string> decode_sequence_field_xml(std::string_view xml, const char* field_name) {
        xml_parser parser;
        auto parse_result = parser.parse(xml);
        if (parse_result.is_err()) {
            return result<std::string>::err(error_code::parse_error);
        }
        auto& elem = parse_result.value();
        for (auto& child : elem.children) {
            if (child.name == field_name) {
                if (child.children.size() == 1) {
                    if (child.children[0].is_empty) {
                        return result<std::string>::ok(std::string("<") + child.children[0].name + "/>");
                    }
                    return result<std::string>::ok(child.children[0].content);
                }
                return result<std::string>::ok(child.content);
            }
        }
        return result<std::string>::err(error_code::parse_error);
    }

    static result<void> open_sequence(const char* name, std::string_view xml) {
        xml_parser parser;
        auto parse_result = parser.parse(xml);
        if (parse_result.is_err()) {
            return result<void>::err(error_code::parse_error);
        }
        if (parse_result.value().name != name) {
            return result<void>::err(error_code::parse_error);
        }
        if (current_sequence_) {
            return result<void>::err(error_code::parse_error);
        }
        current_sequence_ = new std::vector<xml_element>(std::move(parse_result.value().children));
        return result<void>::ok();
    }

    static bool field_is_present(const char* field_name) {
        if (!current_sequence_) return false;
        for (auto& child : *current_sequence_) {
            if (child.name == field_name) return true;
        }
        return false;
    }

    static result<std::string> decode_choice_alternative(std::string_view xml) {
        xml_parser parser;
        auto parse_result = parser.parse(xml);
        if (parse_result.is_err() || parse_result.value().children.empty()) {
            return result<std::string>::err(error_code::parse_error);
        }
        return result<std::string>::ok(std::string(parse_result.value().children[0].name));
    }

    static result<std::string> decode_choice_child_content(std::string_view xml) {
        xml_parser parser;
        auto parse_result = parser.parse(xml);
        if (parse_result.is_err() || parse_result.value().children.empty()) {
            return result<std::string>::err(error_code::parse_error);
        }
        return result<std::string>::ok(std::string(parse_result.value().children[0].content));
    }

    static error_code flush_decode() noexcept {
        return error_code::ok;
    }

private:
    static inline std::vector<xml_element>* current_sequence_ = nullptr;
};

} // namespace asn1pp::xer

#endif  // ASN1PP_NO_TEXT_CODECS
