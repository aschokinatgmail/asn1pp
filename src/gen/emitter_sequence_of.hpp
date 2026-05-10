#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "ast.hpp"

namespace asn1pp::gen {

/// Emit C++ code for an ASN.1 SEQUENCE OF type.
/// Generates a struct with a std::vector member, optional SIZE constraint,
/// and tag specialization for the sequence universal tag.
///
/// \param type_name   The C++ type name for the generated struct.
/// \param type        The parsed sequence_of_type AST node.
/// \param constraints Constraints to apply (e.g., SIZE).
/// \return            C++ source code as a string.
[[nodiscard]] std::string emit_sequence_of(
    std::string_view type_name,
    const sequence_of_type& type,
    const std::vector<constraint>& constraints = {});

/// Emit C++ code for an ASN.1 SET OF type.
/// Identical to SEQUENCE OF except the universal tag is `set` instead of `sequence`.
[[nodiscard]] std::string emit_set_of(
    std::string_view type_name,
    const set_of_type& type,
    const std::vector<constraint>& constraints = {});

}  // namespace asn1pp::gen