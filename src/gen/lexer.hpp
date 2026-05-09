#pragma once

#include "ast.hpp"
#include "diagnostics.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace asn1pp::gen {

enum class token_type {
    identifier,        // user-defined names (myModule, MyType)
    number,            // integer literals (42, 0, 12345)
    binary_string,     // '0101'B
    hex_string,        // 'A0FF'H
    character_string,  // "hello world"

    kw_integer,
    kw_boolean,
    kw_null,
    kw_real,
    kw_bit,
    kw_octet,
    kw_sequence,
    kw_set,
    kw_choice,
    kw_of,
    kw_optional,
    kw_default,
    kw_components,
    kw_imports,
    kw_exports,
    kw_from,
    kw_definitions,
    kw_begin,
    kw_end,
    kw_tagged,
    kw_implicit,
    kw_explicit,
    kw_automatic,
    kw_application,
    kw_universal,
    kw_private,
    kw_enumerated,
    kw_min,
    kw_max,
    kw_size,
    kw_constraint,
    kw_with,
    kw_selected,
    kw_true,
    kw_false,
    kw_extendability,
    kw_present,
    kw_absent,
    kw_all,
    kw_class,
    kw_unique,
    kw_instance,

    assignment,     // ::=
    range,          // ..
    ellipsis,       // ...
    left_brace,     // {
    right_brace,    // }
    left_bracket,   // [
    right_bracket,  // ]
    left_paren,     // (
    right_paren,    // )
    comma,          // ,
    semicolon,      // ;
    pipe,           // |
    caret,          // ^
    ampersand,      // &
    colon,          // :
    double_dot,

    eof,
    error,
};

struct token {
    token_type type;
    std::string value;      // lexeme text
    source_location loc;    // position in source
};

class lexer {
public:
    explicit lexer(std::string_view source,
                   std::string_view filename = "",
                   diagnostic_engine* diag = nullptr);

    token next_token();
    token peek_token();
    [[nodiscard]] source_location current_location() const;
    [[nodiscard]] bool has_error() const noexcept { return has_error_; }

private:
    std::string_view source_;
    size_t pos_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;
    std::string_view filename_;
    diagnostic_engine* diag_;
    std::optional<token> peeked_;
    bool has_error_ = false;

    [[nodiscard]] char peek(size_t offset = 0) const;
    char advance();
    void skip_whitespace_and_comments();
    bool skip_line_comment();
    bool skip_block_comment();
    token scan_string();
    token scan_binary_or_hex();
    token scan_number();
    token scan_identifier_or_keyword();
    token make_token(token_type type, std::string value, source_location loc);
    void report_error(source_location loc, std::string message);

    static const std::unordered_map<std::string, token_type>& keyword_table();
};

}  // namespace asn1pp::gen
