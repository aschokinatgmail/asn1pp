#include "codec/xer/cxer_validator.hpp"

#include <cctype>

namespace asn1pp::xer {

result<void> cxer_validator::validate(const std::string& xml) {
    if (!has_no_extra_whitespace(xml)) {
        return result<void>::err(error_code::parse_error);
    }
    return result<void>::ok();
}

bool cxer_validator::has_no_extra_whitespace(std::string_view xml) {
    for (size_t i = 0; i < xml.size(); ++i) {
        if (xml[i] == '<' || xml[i] == '>') {
            continue;
        }
        if (std::isspace(xml[i])) {
            if (i > 0 && xml[i-1] == '>') {
                return false;
            }
            if (i + 1 < xml.size() && xml[i+1] == '<') {
                return false;
            }
            size_t j = i;
            while (j < xml.size() && std::isspace(xml[j])) {
                ++j;
            }
            if (j > i + 1) {
                return false;
            }
        }
    }
    return true;
}

bool cxer_validator::elements_in_order(std::string_view) {
    return true;
}

bool cxer_validator::has_canonical_boolean_encoding(std::string_view) {
    return true;
}

bool cxer_validator::has_significant_whitespace(std::string_view xml) {
    for (size_t i = 0; i < xml.size(); ++i) {
        if (std::isspace(xml[i])) {
            return true;
        }
    }
    return false;
}

} // namespace asn1pp::xer