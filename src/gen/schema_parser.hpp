#pragma once

#include "ast.hpp"
#include "diagnostics.hpp"
#include "parser.hpp"

#include "../../libs/codec/result.hpp"

#include <string>
#include <string_view>

namespace asn1pp::gen {

class schema_parser {
public:
    schema_parser() = default;

    asn1pp::result<module_definition> parse_file(const std::string& path);
    asn1pp::result<module_definition> parse_string(std::string_view source,
                                                    std::string_view filename = "");

    [[nodiscard]] diagnostic_engine& diagnostics() noexcept { return diag_; }
    [[nodiscard]] const diagnostic_engine& diagnostics() const noexcept { return diag_; }

private:
    diagnostic_engine diag_;
};

}  // namespace asn1pp::gen
