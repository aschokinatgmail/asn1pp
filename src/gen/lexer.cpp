#include "lexer.hpp"

#include <cctype>
#include <algorithm>

namespace asn1pp::gen {

const std::unordered_map<std::string, token_type>& lexer::keyword_table() {
    static const std::unordered_map<std::string, token_type> table = {
        {"INTEGER",    token_type::kw_integer},
        {"BOOLEAN",    token_type::kw_boolean},
        {"NULL",       token_type::kw_null},
        {"REAL",       token_type::kw_real},
        {"BIT",        token_type::kw_bit},
        {"OCTET",      token_type::kw_octet},
        {"SEQUENCE",   token_type::kw_sequence},
        {"SET",        token_type::kw_set},
        {"CHOICE",     token_type::kw_choice},
        {"OF",         token_type::kw_of},
        {"OPTIONAL",   token_type::kw_optional},
        {"DEFAULT",    token_type::kw_default},
        {"COMPONENTS", token_type::kw_components},
        {"IMPORTS",    token_type::kw_imports},
        {"EXPORTS",    token_type::kw_exports},
        {"FROM",       token_type::kw_from},
        {"DEFINITIONS",token_type::kw_definitions},
        {"BEGIN",      token_type::kw_begin},
        {"END",        token_type::kw_end},
        {"TAGGED",     token_type::kw_tagged},
        {"IMPLICIT",   token_type::kw_implicit},
        {"EXPLICIT",   token_type::kw_explicit},
        {"AUTOMATIC",  token_type::kw_automatic},
        {"APPLICATION",token_type::kw_application},
        {"UNIVERSAL",  token_type::kw_universal},
        {"PRIVATE",    token_type::kw_private},
        {"ENUMERATED", token_type::kw_enumerated},
        {"MIN",        token_type::kw_min},
        {"MAX",        token_type::kw_max},
        {"SIZE",       token_type::kw_size},
        {"CONSTRAINT", token_type::kw_constraint},
        {"WITH",       token_type::kw_with},
        {"SELECTED",   token_type::kw_selected},
        {"TRUE",       token_type::kw_true},
        {"FALSE",      token_type::kw_false},
        {"EXTENDABILITY", token_type::kw_extendability},
        {"PRESENT",    token_type::kw_present},
        {"ABSENT",     token_type::kw_absent},
        {"ALL",        token_type::kw_all},
        {"CLASS",      token_type::kw_class},
        {"UNIQUE",     token_type::kw_unique},
        {"SYNTAX",     token_type::kw_syntax},
        {"INSTANCE",   token_type::kw_instance},
    };
    return table;
}

lexer::lexer(std::string_view source, std::string_view filename, diagnostic_engine* diag)
    : source_(source), filename_(filename), diag_(diag) {}

char lexer::peek(size_t offset) const {
    size_t idx = pos_ + offset;
    return (idx < source_.size()) ? source_[idx] : '\0';
}

char lexer::advance() {
    if (pos_ >= source_.size()) { return '\0'; }
    char c = source_[pos_++];
    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return c;
}

source_location lexer::current_location() const {
    return source_location{filename_, line_, column_};
}

token lexer::make_token(token_type type, std::string value, source_location loc) {
    return token{type, std::move(value), loc};
}

void lexer::report_error(source_location loc, std::string message) {
    has_error_ = true;
    if (diag_) {
        diag_->add_error(loc, std::move(message));
    }
}

void lexer::skip_whitespace_and_comments() {
    while (pos_ < source_.size()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '-' && peek(1) == '-') {
            skip_line_comment();
        } else if (c == '/' && peek(1) == '*') {
            skip_block_comment();
        } else {
            break;
        }
    }
}

bool lexer::skip_line_comment() {
    if (peek() != '-' || peek(1) != '-') { return false; }
    advance();  // first -
    advance();  // second -
    while (pos_ < source_.size() && peek() != '\n') {
        advance();
    }
    return true;
}

bool lexer::skip_block_comment() {
    if (peek() != '/' || peek(1) != '*') { return false; }
    source_location loc{filename_, line_, column_};
    advance();  // /
    advance();  // *
    while (pos_ < source_.size()) {
        if (peek() == '*' && peek(1) == '/') {
            advance();  // *
            advance();  // /
            return true;
        }
        advance();
    }
    // EOF without */
    report_error(loc, "unterminated block comment");
    return false;
}

token lexer::scan_string() {
    source_location loc{filename_, line_, column_};
    advance();  // opening "
    std::string value;
    while (pos_ < source_.size()) {
        char c = peek();
        if (c == '"') {
            advance();  // closing "
            return make_token(token_type::character_string, std::move(value), loc);
        }
        if (c == '\\' && peek(1) == '"') {
            advance();  // backslash
            advance();  // quote
            value.push_back('"');
        } else if (c == '\\' && peek(1) == '\\') {
            advance();  // first backslash
            advance();  // second backslash
            value.push_back('\\');
        } else {
            value.push_back(advance());
        }
    }
    report_error(loc, "unterminated string literal");
    return make_token(token_type::error, std::move(value), loc);
}

token lexer::scan_binary_or_hex() {
    source_location loc{filename_, line_, column_};
    advance();  // opening '
    std::string value;
    while (pos_ < source_.size() && peek() != '\'') {
        value.push_back(advance());
    }
    if (pos_ >= source_.size()) {
        report_error(loc, "unterminated binary/hex string");
        return make_token(token_type::error, std::move(value), loc);
    }
    advance();  // closing '

    // Determine type based on suffix
    if (pos_ < source_.size()) {
        char suffix = static_cast<char>(std::toupper(static_cast<unsigned char>(peek())));
        if (suffix == 'B') {
            advance();
            return make_token(token_type::binary_string, std::move(value), loc);
        }
        if (suffix == 'H') {
            advance();
            return make_token(token_type::hex_string, std::move(value), loc);
        }
    }

    // No suffix — treat as an error
    report_error(loc, "expected 'B' or 'H' after binary/hex string");
    return make_token(token_type::error, std::move(value), loc);
}

token lexer::scan_number() {
    source_location loc{filename_, line_, column_};
    std::string value;
    while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(peek()))) {
        value.push_back(advance());
    }
    return make_token(token_type::number, std::move(value), loc);
}

token lexer::scan_identifier_or_keyword() {
    source_location loc{filename_, line_, column_};
    std::string value;
    // First character: letter or underscore
    value.push_back(advance());
    // Subsequent characters: letter, digit, hyphen, underscore
    while (pos_ < source_.size()) {
        char c = peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') {
            value.push_back(advance());
        } else {
            break;
        }
    }

    // Build uppercase version for keyword lookup
    std::string upper;
    upper.reserve(value.size());
    for (char ch : value) {
        upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    }

    const auto& table = keyword_table();
    auto it = table.find(upper);
    if (it != table.end()) {
        return make_token(it->second, value, loc);
    }
    return make_token(token_type::identifier, std::move(value), loc);
}

token lexer::next_token() {
    // Return peeked token if available
    if (peeked_.has_value()) {
        token t = std::move(*peeked_);
        peeked_.reset();
        return t;
    }

    skip_whitespace_and_comments();

    if (pos_ >= source_.size()) {
        return make_token(token_type::eof, "", current_location());
    }

    source_location loc{filename_, line_, column_};
    char c = peek();

    // Strings
    if (c == '"') {
        return scan_string();
    }

    // Binary/Hex strings
    if (c == '\'') {
        return scan_binary_or_hex();
    }

    // Digits → number
    if (std::isdigit(static_cast<unsigned char>(c))) {
        return scan_number();
    }

    // Identifiers and keywords
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return scan_identifier_or_keyword();
    }

    // Multi-character operators
    if (c == ':') {
        if (peek(1) == ':' && peek(2) == '=') {
            advance();  // :
            advance();  // :
            advance();  // =
            return make_token(token_type::assignment, "::=", loc);
        }
        advance();
        return make_token(token_type::colon, ":", loc);
    }

    if (c == '.') {
        if (peek(1) == '.') {
            if (peek(2) == '.') {
                advance();  // .
                advance();  // .
                advance();  // .
                return make_token(token_type::ellipsis, "...", loc);
            }
            advance();  // .
            advance();  // .
            return make_token(token_type::range, "..", loc);
        }
        // Single dot — not a valid ASN.1 token, but we handle it to avoid errors
        advance();
        report_error(loc, "unexpected character '.'");
        return make_token(token_type::error, ".", loc);
    }

    // Single-character operators
    switch (c) {
        case '{': advance(); return make_token(token_type::left_brace,    "{", loc);
        case '}': advance(); return make_token(token_type::right_brace,   "}", loc);
        case '[': advance(); return make_token(token_type::left_bracket,  "[", loc);
        case ']': advance(); return make_token(token_type::right_bracket, "]", loc);
        case '(': advance(); return make_token(token_type::left_paren,    "(", loc);
        case ')': advance(); return make_token(token_type::right_paren,   ")", loc);
        case ',': advance(); return make_token(token_type::comma,         ",", loc);
        case ';': advance(); return make_token(token_type::semicolon,     ";", loc);
        case '|': advance(); return make_token(token_type::pipe,          "|", loc);
        case '^': advance(); return make_token(token_type::caret,         "^", loc);
        case '&': advance(); return make_token(token_type::ampersand,     "&", loc);
        default: break;
    }

    // Unknown character
    std::string bad(1, advance());
    report_error(loc, "unexpected character '" + bad + "'");
    return make_token(token_type::error, std::move(bad), loc);
}

token lexer::peek_token() {
    if (!peeked_.has_value()) {
        peeked_ = next_token();
    }
    return *peeked_;
}

}  // namespace asn1pp::gen
