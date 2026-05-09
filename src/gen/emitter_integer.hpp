#pragma once

#include "emitter.hpp"
#include "ast.hpp"

#include <string>
#include <vector>

namespace asn1pp::gen {

std::string emit_integer_type(const integer_type& type,
                               const std::string& name,
                               const std::vector<constraint>& constraints,
                               const emitter_options& opts = {});

}  // namespace asn1pp::gen