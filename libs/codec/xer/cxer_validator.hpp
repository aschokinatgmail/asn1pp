#pragma once

#include <string>
#include <string_view>

#include "codec/result.hpp"

namespace asn1pp::xer {

// CXER (Canonical XER) validator
// Enforces deterministic rules for canonical XML encoding:
// 1. No optional whitespace between elements
// 2. Elements must appear in defined order
// 3. Attribute ordering is deterministic
// 4. Boolean encoding: use canonical form

class cxer_validator {
public:
    // Validate XML string for CXER compliance
    static result<void> validate(const std::string& xml);

private:
    // Check for extra whitespace between elements (not allowed in CXER)
    static bool has_no_extra_whitespace(std::string_view xml);

    // Check that elements appear in order (basic check)
    static bool elements_in_order(std::string_view xml);

    // Check for canonical boolean encoding
    static bool has_canonical_boolean_encoding(std::string_view xml);

    // Skip whitespace but report if any exists between tags
    static bool has_significant_whitespace(std::string_view xml);
};

} // namespace asn1pp::xer