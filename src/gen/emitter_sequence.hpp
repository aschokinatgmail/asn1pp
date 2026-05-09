#pragma once

#include <string>
#include <string_view>

#include "ast.hpp"

namespace asn1pp::gen {

/// Emit C++ code for an ASN.1 SEQUENCE type.
/// Generates a struct with fields, optional/discard handling,
/// default initializers, nested inline types, and tag specialization.
///
/// \param type_name  The C++ type name for the generated struct.
/// \param seq        The parsed sequence_type AST node.
/// \param tagging    The tagging mode for the module (affects comments).
/// \return           C++ source code as a string.
[[nodiscard]] std::string emit_sequence(
    std::string_view type_name,
    const sequence_type& seq,
    tag_default tagging = tag_default::automatic_tag);

/// Emit C++ code for an ASN.1 SET type.
/// Identical to SEQUENCE except the universal tag is `set` instead of `sequence`.
[[nodiscard]] std::string emit_set(
    std::string_view type_name,
    const set_type& s,
    tag_default tagging = tag_default::automatic_tag);

}  // namespace asn1pp::gen
