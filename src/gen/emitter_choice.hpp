#pragma once

#include <string>
#include <string_view>

#include "ast.hpp"

namespace asn1pp::gen {

/// Emit C++ code for an ASN.1 CHOICE type.
///
/// Generates a struct with:
///   - A `which` enum enumerating the alternatives
///   - A `std::variant<...> value` member
///   - Type-safe accessor methods: `is_<name>()`, `get_<name>()`
///   - `index()` returning the current which value
///   - Defaulted `operator==`
///   - Tag specialization: `make_universal(universal_tag::sequence, true)`
///
/// If `has_extension` is true, a `_unknown_extension` enum value and
/// `std::vector<uint8_t>` variant alternative are added.
///
/// \param type_name  The C++ type name for the generated struct.
/// \param choice     The parsed choice_type AST node.
/// \param tagging    The tagging mode for the module (affects comments only).
/// \return           C++ source code as a string.
[[nodiscard]] std::string emit_choice(
    std::string_view type_name,
    const choice_type& choice,
    tag_default tagging = tag_default::automatic_tag);

}  // namespace asn1pp::gen
