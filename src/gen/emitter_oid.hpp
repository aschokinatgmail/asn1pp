#pragma once

#include "emitter.hpp"
#include "ast.hpp"

#include <string>
#include <vector>

namespace asn1pp::gen {

std::string emit_object_identifier_type(const object_identifier_type& type,
                                       const std::string& name,
                                       const std::vector<constraint>& constraints,
                                       const emitter_options& opts = {});

std::string emit_relative_oid_type(const relative_oid_type& type,
                                    const std::string& name,
                                    const std::vector<constraint>& constraints,
                                    const emitter_options& opts = {});

}  // namespace asn1pp::gen