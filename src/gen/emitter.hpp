#pragma once

#include <string>
#include <vector>
#include <string_view>

namespace asn1pp::gen {

struct emitter_options {
    bool include_operators = true;
    bool include_validate = true;
    bool include_to_string = false;
    std::string namespace_name;
};

class code_emitter {
public:
    virtual ~code_emitter() = default;

    static std::string emit_header_guard(std::string_view name);
    static std::string emit_includes(const std::vector<std::string>& includes);
    static std::string emit_namespace_open(const std::string& ns);
    static std::string emit_namespace_close(const std::string& ns);
};

}  // namespace asn1pp::gen