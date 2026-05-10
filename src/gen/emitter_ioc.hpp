#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include "ast.hpp"

namespace asn1pp::gen {

std::string emit_class(const class_type& ct);

std::string emit_information_object(
    const information_object& obj,
    const std::unordered_map<std::string, class_type>& class_defs);

std::string emit_object_set(const object_set& os);

}  // namespace asn1pp::gen
