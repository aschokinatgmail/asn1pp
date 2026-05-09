#pragma once

#include "ast.hpp"
#include "diagnostics.hpp"
#include "lexer.hpp"

#include "../../libs/codec/result.hpp"

#include <string>
#include <string_view>

namespace asn1pp::gen {

class parser {
public:
    explicit parser(std::string_view source,
                    std::string_view filename = "",
                    diagnostic_engine* diag = nullptr);

    asn1pp::result<module_definition> parse_module();

private:
    lexer lexer_;
    diagnostic_engine* diag_;
    token current_;

    // ========================================================================
    // Token management
    // ========================================================================
    void advance();
    [[nodiscard]] bool check(token_type type) const noexcept;
    [[nodiscard]] bool check_identifier(std::string_view value) const noexcept;
    [[nodiscard]] token consume(token_type type, std::string_view msg);
    [[nodiscard]] token consume_identifier(std::string_view msg);
    [[nodiscard]] bool consume_or_recover(token_type type, std::string_view msg);
    [[nodiscard]] bool match(token_type type);
    [[nodiscard]] bool match_identifier(std::string_view value);
    [[nodiscard]] bool consume_identifier_value(std::string_view value, std::string_view msg);

    void error(source_location loc, std::string msg);
    void synchronize();

    // ========================================================================
    // Module structure
    // ========================================================================
    asn1pp::result<module_definition> parse_module_definition();
    asn1pp::result<tag_default> parse_tag_default();
    asn1pp::result<void> parse_module_body(module_definition& mod);

    // ========================================================================
    // Imports / Exports
    // ========================================================================
    asn1pp::result<void> parse_imports(module_definition& mod);
    asn1pp::result<void> parse_exports(module_definition& mod);

    // ========================================================================
    // Assignments
    // ========================================================================
    asn1pp::result<assignment> parse_assignment();
    asn1pp::result<type_assignment> parse_type_assignment(
        const std::string& name, source_location loc);
    asn1pp::result<value_assignment> parse_value_assignment(
        const std::string& name, type_ref ty, source_location loc);

    // ========================================================================
    // Types
    // ========================================================================
    asn1pp::result<type_ref> parse_type();
    asn1pp::result<type_ref> parse_builtin_type(token_type kw);
    asn1pp::result<type_ref> parse_named_type(const std::string& name);
    asn1pp::result<type_ref> finish_type_with_constraints(type_ref tr);

    asn1pp::result<sequence_type> parse_sequence_type();
    asn1pp::result<set_type> parse_set_type();
    asn1pp::result<choice_type> parse_choice_type();
    asn1pp::result<enumerated_type> parse_enumerated_type();
    asn1pp::result<bit_string_type> parse_bit_string_type_body();
    asn1pp::result<type_ref> parse_sequence_of_type();
    asn1pp::result<type_ref> parse_set_of_type();
    asn1pp::result<type_ref> parse_constrained_sequence_of();

    asn1pp::result<component_type> parse_component_type();
    asn1pp::result<choice_alternative> parse_choice_alternative();

    // ========================================================================
    // Tagged types
    // ========================================================================
    asn1pp::result<tagged_type> parse_tagged_type();

    // ========================================================================
    // Constraints
    // ========================================================================
    asn1pp::result<constraint> parse_constraint();
    asn1pp::result<value_range_constraint> parse_value_range();
    asn1pp::result<size_constraint> parse_size_constraint();

    // ========================================================================
    // Values
    // ========================================================================
    asn1pp::result<value_ref> parse_value();

    // ========================================================================
    // Helpers
    // ========================================================================
    [[nodiscard]] bool is_type_keyword() const noexcept;
    [[nodiscard]] bool is_builtin_start(token_type t) const noexcept;
    [[nodiscard]] source_location current_loc() const noexcept;
};

}  // namespace asn1pp::gen
