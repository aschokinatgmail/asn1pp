#pragma once

#include <string>
#include <string_view>

#include "ast.hpp"
#include "emitter.hpp"

namespace asn1pp::gen {

std::string emit_tagged_type(std::string_view type_name,
                             const tagged_type& tt,
                             const emitter_options& opts = {});

}  // namespace asn1pp::gen
