#pragma once

#include <string>
#include <string_view>

#include "ast.hpp"

namespace asn1pp::gen {

std::string emit_enumerated(std::string_view type_name, const enumerated_type& enum_type);

}  // namespace asn1pp::gen
