#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "ast.hpp"

namespace asn1pp::gen {

/// Emit C++ template code for an ASN.1 parameterized type definition.
///
/// For type parameters (e.g. {T}), generates: template<typename T> struct Name { ... };
/// For value parameters (e.g. {INTEGER: n}), generates: template<int64_t n> struct Name { ... };
///
/// \param pta        The parsed parameterized_type_assignment AST node.
/// \param tagging    The tagging mode for the module.
/// \return           C++ source code as a string.
[[nodiscard]] std::string emit_parameterized(
    const parameterized_type_assignment& pta,
    tag_default tagging = tag_default::automatic_tag);

/// Resolve parameter substitution: given a type that may reference formal parameters,
/// substitute with actual argument type names.
///
/// \param type         The type_ref to resolve.
/// \param params       The formal parameters.
/// \param args         The actual arguments.
/// \param resolved     [out] The resolved type.
/// \return             true if resolution succeeded.
[[nodiscard]] bool resolve_parameters(
    const type_ref& type,
    const std::vector<formal_parameter>& params,
    const std::vector<actual_parameter>& args,
    type_ref& resolved);

/// Generate a template specialization for a specific instantiation.
///
/// \param base_name    The parameterized type name.
/// \param params       The formal parameters.
/// \param inst         The type_instantiation with actual arguments.
/// \param pta_type     The body type of the parameterized definition.
/// \param tagging      The tagging mode.
/// \return             C++ source code for the specialization.
[[nodiscard]] std::string emit_specialization(
    std::string_view base_name,
    const std::vector<formal_parameter>& params,
    const type_instantiation& inst,
    const type_ref& pta_type,
    tag_default tagging = tag_default::automatic_tag);

/// Get the C++ template parameter declaration for a formal parameter.
/// Type parameters become "typename T", value parameters become "int64_t N" etc.
[[nodiscard]] std::string template_param_decl(const formal_parameter& fp);

/// Get the C++ template argument for an actual parameter.
[[nodiscard]] std::string template_arg_str(const actual_parameter& ap);

}  // namespace asn1pp::gen
