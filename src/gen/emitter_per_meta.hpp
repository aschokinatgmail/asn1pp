#pragma once

#include "emitter.hpp"
#include "ast.hpp"

#include <string>
#include <string_view>

namespace asn1pp::gen {

std::string emit_per_meta(const type_ref& type,
                           std::string_view type_name,
                           const emitter_options& opts = {});

}  // namespace asn1pp::gen
