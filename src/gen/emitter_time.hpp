#pragma once

#include <string>

#include "../../libs/codec/traits.hpp"

namespace asn1pp::gen {

struct type_ref;
struct constrained_type;

namespace emitter {

struct emit_result {
    std::string code;
    bool success;
    std::string error;
};

emit_result emit_utc_time(const type_ref& type, const std::string& name);
emit_result emit_generalized_time(const type_ref& type, const std::string& name);

} // namespace emitter
} // namespace asn1pp::gen