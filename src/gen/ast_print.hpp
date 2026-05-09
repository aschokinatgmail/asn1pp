#pragma once

#include <string>
#include "ast.hpp"

namespace asn1pp::gen {

std::string to_string(const type_ref& type);
std::string to_string(const module_definition& module);
std::string to_string(const enumerated_type& type);
std::string to_string(const bit_string_type& type);
std::string to_string(const sequence_type& type);
std::string to_string(const set_type& type);
std::string to_string(const choice_type& type);
std::string to_string(const sequence_of_type& type);
std::string to_string(const set_of_type& type);
std::string to_string(const tagged_type& type);
std::string to_string(const constrained_type& type);
std::string to_string(const selection_type& type);
std::string to_string(const component_type& comp);
std::string to_string(const constraint& c);

}  // namespace asn1pp::gen
