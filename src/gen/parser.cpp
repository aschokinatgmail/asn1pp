#include "parser.hpp"

#include <charconv>
#include <cctype>
#include <string_view>

namespace {

[[nodiscard]] bool equals_ignore_case(std::string_view lhs, std::string_view rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
        const auto l = static_cast<unsigned char>(lhs[i]);
        const auto r = static_cast<unsigned char>(rhs[i]);
        if (std::toupper(l) != std::toupper(r)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool is_identifier_value(const asn1pp::gen::token& tok, std::string_view value) noexcept {
    return tok.type == asn1pp::gen::token_type::identifier && equals_ignore_case(tok.value, value);
}

[[nodiscard]] bool is_implied_token(const asn1pp::gen::token& tok) noexcept {
    return is_identifier_value(tok, "IMPLIED");
}

[[nodiscard]] bool is_extensibility_token(const asn1pp::gen::token& tok) noexcept {
    return is_identifier_value(tok, "EXTENSIBILITY") || tok.type == asn1pp::gen::token_type::kw_extendability;
}

[[nodiscard]] bool is_name_token(const asn1pp::gen::token& tok) noexcept {
    return tok.type == asn1pp::gen::token_type::identifier || tok.type == asn1pp::gen::token_type::kw_tagged;
}

[[nodiscard]] bool is_assignment_boundary(asn1pp::gen::lexer& lex,
                                          const asn1pp::gen::token& current) {
    return is_name_token(current) && lex.peek_token().type == asn1pp::gen::token_type::assignment;
}

template<typename Int>
[[nodiscard]] Int parse_integer_text(std::string_view text) noexcept {
    Int value = 0;
    (void)std::from_chars(text.data(), text.data() + text.size(), value);
    return value;
}

void skip_balanced(asn1pp::gen::lexer& lex, asn1pp::gen::token& current,
                   asn1pp::gen::token_type open, asn1pp::gen::token_type close) {
    if (current.type != open) {
        return;
    }
    size_t depth = 0;
    do {
        if (current.type == open) {
            ++depth;
        } else if (current.type == close) {
            --depth;
        }
        current = lex.next_token();
    } while (depth > 0 && current.type != asn1pp::gen::token_type::eof);
}

}  // namespace

namespace asn1pp::gen {

namespace {
[[nodiscard]] std::string token_type_to_string(token_type t) noexcept {
    switch (t) {
        case token_type::kw_integer: return "INTEGER";
        case token_type::kw_boolean: return "BOOLEAN";
        case token_type::kw_null: return "NULL";
        case token_type::kw_real: return "REAL";
        case token_type::kw_octet: return "OCTET STRING";
        case token_type::kw_bit: return "BIT STRING";
        case token_type::kw_sequence: return "SEQUENCE";
        case token_type::kw_set: return "SET";
        case token_type::kw_choice: return "CHOICE";
        case token_type::kw_enumerated: return "ENUMERATED";
        default: return "UNKNOWN";
    }
}
}  // namespace

parser::parser(std::string_view source, std::string_view filename, diagnostic_engine* diag)
    : lexer_(source, filename, diag), diag_(diag)
{
    advance();
}

source_location parser::current_loc() const noexcept {
    return current_.loc;
}

void parser::error(source_location loc, std::string msg) {
    if (diag_) {
        diag_->add_error(loc, std::move(msg));
    }
}

void parser::advance() {
    current_ = lexer_.next_token();
}

bool parser::check(token_type type) const noexcept {
    return current_.type == type;
}

bool parser::check_identifier(std::string_view value) const noexcept {
    return is_identifier_value(current_, value);
}

token parser::consume(token_type type, std::string_view msg) {
    if (check(type)) {
        token t = std::move(current_);
        advance();
        return t;
    }
    error(current_.loc, std::string(msg));
    return {};
}

token parser::consume_identifier(std::string_view msg) {
    if (current_.type == token_type::identifier) {
        token t = std::move(current_);
        advance();
        return t;
    }
    error(current_.loc, std::string(msg));
    return {};
}

bool parser::consume_or_recover(token_type type, std::string_view msg) {
    if (check(type)) {
        advance();
        return true;
    }
    error(current_.loc, std::string(msg));
    synchronize();
    return false;
}

bool parser::match(token_type type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool parser::match_identifier(std::string_view value) {
    if (check_identifier(value)) {
        advance();
        return true;
    }
    return false;
}

bool parser::consume_identifier_value(std::string_view value, std::string_view msg) {
    if (check_identifier(value)) {
        advance();
        return true;
    }
    error(current_.loc, std::string(msg));
    return false;
}

void parser::synchronize() {
    while (!check(token_type::eof)) {
        if (check(token_type::semicolon) || check(token_type::right_brace)) {
            advance();
            return;
        }
        advance();
    }
}

asn1pp::result<module_definition> parser::parse_module() {
    return parse_module_definition();
}

asn1pp::result<module_definition> parser::parse_module_definition() {
    module_definition mod;

    if (current_.type == token_type::identifier) {
        mod.name = current_.value;
        advance();
    } else {
        error(current_.loc, "expected module name identifier");
        return asn1pp::result<module_definition>::err(asn1pp::error_code::parse_error);
    }

    if (check(token_type::left_brace)) {
        advance();
        mod.module_oid = "{ " + current_.value;
        advance();
        while (!check(token_type::right_brace) && !check(token_type::eof)) {
            mod.module_oid += " " + current_.value;
            advance();
        }
        if (check(token_type::right_brace)) {
            mod.module_oid += " }";
            advance();
        }
    }

    if (!check(token_type::kw_definitions)) {
        error(current_.loc, "expected DEFINITIONS after module name");
        return asn1pp::result<module_definition>::err(asn1pp::error_code::parse_error);
    }
    advance();

    auto td = parse_tag_default();
    if (td.is_ok()) {
        mod.default_tagging = td.value();
    } else {
        mod.default_tagging = tag_default::automatic_tag;
    }

    if (is_extensibility_token(current_)) {
        advance();
        if (is_implied_token(current_)) {
            advance();
            mod.extensibility_implied = true;
        } else {
            error(current_.loc, "expected IMPLIED after EXTENSIBILITY");
        }
    }

    if (!check(token_type::assignment)) {
        error(current_.loc, "expected ::= after DEFINITIONS clause");
        return asn1pp::result<module_definition>::err(asn1pp::error_code::parse_error);
    }
    advance();

    if (!check(token_type::kw_begin)) {
        error(current_.loc, "expected BEGIN");
        return asn1pp::result<module_definition>::err(asn1pp::error_code::parse_error);
    }
    advance();
    auto body_result = parse_module_body(mod);
    if (body_result.is_err()) {
        return asn1pp::result<module_definition>::err(body_result.error());
    }
    if (!check(token_type::kw_end)) {
        error(current_.loc, "expected END");
    } else {
        advance();
    }

    return asn1pp::result<module_definition>::ok(std::move(mod));
}

asn1pp::result<tag_default> parser::parse_tag_default() {
    if (check(token_type::kw_explicit)) {
        advance();
        (void)consume_identifier_value("TAGS", "expected TAGS after EXPLICIT");
        return asn1pp::result<tag_default>::ok(tag_default::explicit_tag);
    }
    if (check(token_type::kw_implicit)) {
        advance();
        (void)consume_identifier_value("TAGS", "expected TAGS after IMPLICIT");
        return asn1pp::result<tag_default>::ok(tag_default::implicit_tag);
    }
    if (check(token_type::kw_automatic)) {
        advance();
        (void)consume_identifier_value("TAGS", "expected TAGS after AUTOMATIC");
        return asn1pp::result<tag_default>::ok(tag_default::automatic_tag);
    }
    return asn1pp::result<tag_default>::err(asn1pp::error_code::parse_error);
}

asn1pp::result<void> parser::parse_module_body(module_definition& mod) {
    while (!check(token_type::kw_end) && !check(token_type::eof)) {
        if (check(token_type::kw_imports)) {
            auto ir = parse_imports(mod);
            if (ir.is_err()) {
                return ir;
            }
        } else if (check(token_type::kw_exports)) {
            auto er = parse_exports(mod);
            if (er.is_err()) {
                return er;
            }
        } else if (is_name_token(current_)) {
            auto a = parse_assignment();
            if (a.is_ok()) {
                mod.assignments.push_back(std::move(a.value()));
            } else {
                synchronize();
            }
        } else {
            error(current_.loc, "unexpected token in module body");
            synchronize();
        }
    }
    return asn1pp::result<void>::ok();
}

asn1pp::result<void> parser::parse_imports(module_definition&) {
    advance();

    if (check(token_type::semicolon)) {
        advance();
        return asn1pp::result<void>::ok();
    }

    while (!check(token_type::kw_from) && !check(token_type::semicolon) && !check(token_type::eof)) {
        if (current_.type == token_type::identifier || current_.type == token_type::kw_imports) {
            advance();
        } else if (check(token_type::comma)) {
            advance();
        } else {
            error(current_.loc, "expected symbol name or comma in imports list");
            synchronize();
            return asn1pp::result<void>::err(asn1pp::error_code::parse_error);
        }
    }

    if (check(token_type::kw_from)) {
        advance();
        if (current_.type == token_type::identifier) {
            advance();
        } else {
            error(current_.loc, "expected module name after FROM");
        }
    }

    (void)consume_or_recover(token_type::semicolon, "expected ; after IMPORTS");
    return asn1pp::result<void>::ok();
}

asn1pp::result<void> parser::parse_exports(module_definition&) {
    advance();

    if (check(token_type::semicolon)) {
        advance();
        return asn1pp::result<void>::ok();
    }

    if (check_identifier("ALL") || check(token_type::kw_all)) {
        advance();
    } else {
        while (!check(token_type::semicolon) && !check(token_type::eof)) {
            if (check(token_type::comma)) {
                advance();
            } else if (current_.type == token_type::identifier) {
                advance();
            } else {
                error(current_.loc, "expected identifier in EXPORTS list");
                break;
            }
        }
    }

    (void)consume_or_recover(token_type::semicolon, "expected ; after EXPORTS");
    return asn1pp::result<void>::ok();
}

asn1pp::result<assignment> parser::parse_assignment() {
    if (!is_name_token(current_)) {
        error(current_.loc, "expected identifier at start of assignment");
        return asn1pp::result<assignment>::err(asn1pp::error_code::parse_error);
    }

    std::string name = current_.value;
    source_location name_loc = current_.loc;
    advance();

    if (current_.type == token_type::identifier && known_classes_.count(current_.value) > 0) {
        std::string class_name = current_.value;
        advance();

        if (!check(token_type::assignment)) {
            error(current_.loc, "expected ::= after class name");
            return asn1pp::result<assignment>::err(asn1pp::error_code::parse_error);
        }
        advance();

        if (check(token_type::left_brace)) {
            // Lookahead: & starts information object, identifier/ellipsis starts object set
            auto peeked = lexer_.peek_token();
            if (peeked.type == token_type::ampersand) {
                auto obj = parse_information_object_def(name, class_name, name_loc);
                if (obj.is_ok()) {
                    assignment a;
                    a.content = std::move(obj.value());
                    return asn1pp::result<assignment>::ok(std::move(a));
                }
            } else {
                auto os = parse_object_set_def(name, class_name);
                if (os.is_ok()) {
                    assignment a;
                    a.content = std::move(os.value());
                    return asn1pp::result<assignment>::ok(std::move(a));
                }
            }
            return asn1pp::result<assignment>::err(asn1pp::error_code::parse_error);
        }
    }

    if (check(token_type::left_brace)) {
        auto pta_result = parse_parameterized_type_assignment(name, name_loc);
        if (pta_result.is_ok()) {
            assignment a;
            a.content = std::move(pta_result.value());
            return asn1pp::result<assignment>::ok(std::move(a));
        }
        return asn1pp::result<assignment>::err(pta_result.error());
    }

    if (!check(token_type::assignment)) {
        if (is_type_keyword() || current_.type == token_type::identifier) {
            auto ty_result = parse_type();
            if (ty_result.is_err()) {
                return asn1pp::result<assignment>::err(ty_result.error());
            }
            if (check(token_type::assignment)) {
                return parse_value_assignment(name, std::move(ty_result.value()), name_loc)
                    .map([](value_assignment va) -> assignment {
                        assignment a;
                        a.content = std::move(va);
                        return a;
                    });
            }
            error(current_.loc, "expected ::= after identifier and type");
            return asn1pp::result<assignment>::err(asn1pp::error_code::parse_error);
        }
        error(current_.loc, "expected ::= after identifier");
        return asn1pp::result<assignment>::err(asn1pp::error_code::parse_error);
    }

    advance();

    if (check(token_type::kw_class)) {
        auto cd = parse_class_def(name);
        if (cd.is_ok()) {
            assignment a;
            a.content = std::move(cd.value());
            return asn1pp::result<assignment>::ok(std::move(a));
        }
        return asn1pp::result<assignment>::err(cd.error());
    }

    if (is_type_keyword() || is_builtin_start(current_.type)) {
        return parse_type_assignment(name, name_loc)
            .map([](type_assignment ta) -> assignment {
                assignment a;
                a.content = std::move(ta);
                return a;
            });
    }

    error(current_.loc, "expected type after ::=");
    return asn1pp::result<assignment>::err(asn1pp::error_code::parse_error);
}

asn1pp::result<type_assignment> parser::parse_type_assignment(
    const std::string& name, source_location)
{
    auto ty = parse_type();
    if (ty.is_err()) {
        return asn1pp::result<type_assignment>::err(ty.error());
    }
    type_assignment ta;
    ta.name = name;
    ta.type = std::make_unique<type_ref>(std::move(ty.value()));
    return asn1pp::result<type_assignment>::ok(std::move(ta));
}

asn1pp::result<value_assignment> parser::parse_value_assignment(
    const std::string& name, type_ref ty, source_location)
{
    advance();

    auto val = parse_value();
    if (val.is_err()) {
        return asn1pp::result<value_assignment>::err(val.error());
    }

    value_assignment va;
    va.name = name;
    va.type = std::make_unique<type_ref>(std::move(ty));
    va.value = val.value();
    return asn1pp::result<value_assignment>::ok(std::move(va));
}

bool parser::is_type_keyword() const noexcept {
    return current_.type != token_type::identifier && is_builtin_start(current_.type);
}

bool parser::is_builtin_start(token_type t) const noexcept {
    switch (t) {
        case token_type::kw_integer:
        case token_type::kw_boolean:
        case token_type::kw_null:
        case token_type::kw_real:
        case token_type::kw_octet:
        case token_type::kw_bit:
        case token_type::kw_sequence:
        case token_type::kw_set:
        case token_type::kw_choice:
        case token_type::kw_enumerated:
        case token_type::kw_selected:
        case token_type::left_bracket:
        case token_type::identifier:
            return true;
        default:
            return false;
    }
}

asn1pp::result<type_ref> parser::parse_type() {
    if (check(token_type::left_bracket)) {
        auto tt = parse_tagged_type();
        if (tt.is_err()) {
            return asn1pp::result<type_ref>::err(tt.error());
        }
        type_ref tr;
        tr.content = std::make_unique<tagged_type>(std::move(tt.value()));
        return asn1pp::result<type_ref>::ok(std::move(tr));
    }

    if (current_.type == token_type::identifier) {
        std::string name = current_.value;
        source_location name_loc = current_.loc;

        if (equals_ignore_case(name, "OBJECT")) {
            advance();
            if (check_identifier("IDENTIFIER")) {
                advance();
                type_ref tr;
                tr.content = object_identifier_type{};
                return finish_type_with_constraints(std::move(tr));
            }
            error(name_loc, "expected IDENTIFIER after OBJECT");
            return asn1pp::result<type_ref>::err(asn1pp::error_code::parse_error);
        }

        if (equals_ignore_case(name, "RELATIVE")) {
            advance();
            if (check_identifier("OID")) {
                advance();
                type_ref tr;
                tr.content = relative_oid_type{};
                return finish_type_with_constraints(std::move(tr));
            }
            error(name_loc, "expected OID after RELATIVE");
            return asn1pp::result<type_ref>::err(asn1pp::error_code::parse_error);
        }

        if (equals_ignore_case(name, "OCTET") || equals_ignore_case(name, "BIT")) {
            advance();
            (void)consume_identifier_value("STRING", "expected STRING after " + name);
            type_ref tr;
            if (equals_ignore_case(name, "OCTET")) {
                tr.content = octet_string_type{};
                return finish_type_with_constraints(std::move(tr));
            }
            if (check(token_type::left_brace)) {
                auto bst = parse_bit_string_type_body();
                if (bst.is_ok()) {
                    tr.content = std::move(bst.value());
                } else {
                    tr.content = bit_string_type{};
                }
            } else {
                tr.content = bit_string_type{};
            }
            return finish_type_with_constraints(std::move(tr));
        }

        if (equals_ignore_case(name, "ANY")) {
            advance();
            type_ref tr;
            tr.content = any_type{};
            return finish_type_with_constraints(std::move(tr));
        }

        advance();
        if (check(token_type::left_brace)) {
            auto ti_result = parse_type_instantiation(name);
            if (ti_result.is_err()) {
                return asn1pp::result<type_ref>::err(ti_result.error());
            }
            type_ref tr;
            tr.content = std::make_unique<type_instantiation>(std::move(ti_result.value()));
            return finish_type_with_constraints(std::move(tr));
        }
        if (check(token_type::left_paren)) {
            auto c = parse_constraint();
            constrained_type ct;
            ct.underlying_type = std::make_unique<type_ref>();
            ct.underlying_type->content = std::string(name);
            if (c.is_ok()) {
                ct.constraints.push_back(c.value());
            }
            type_ref tr;
            tr.content = std::make_unique<constrained_type>(std::move(ct));
            return asn1pp::result<type_ref>::ok(std::move(tr));
        }
        return parse_named_type(name);
    }

    return parse_builtin_type(current_.type);
}

asn1pp::result<type_ref> parser::parse_builtin_type(token_type kw) {
    type_ref tr;

    switch (kw) {
        case token_type::kw_integer: {
            advance();
            if (check(token_type::left_brace)) {
                advance();
                while (!check(token_type::right_brace) && !check(token_type::eof)) {
                    advance();
                }
                if (check(token_type::right_brace)) {
                    advance();
                }
            }
            tr.content = integer_type{};
            return finish_type_with_constraints(std::move(tr));
        }
        case token_type::kw_boolean: {
            advance();
            tr.content = boolean_type{};
            return finish_type_with_constraints(std::move(tr));
        }
        case token_type::kw_null: {
            advance();
            tr.content = null_type{};
            return finish_type_with_constraints(std::move(tr));
        }
        case token_type::kw_real: {
            advance();
            tr.content = real_type{};
            return finish_type_with_constraints(std::move(tr));
        }
        case token_type::kw_octet: {
            advance();
            (void)consume_identifier_value("STRING", "expected STRING after OCTET");
            tr.content = octet_string_type{};
            return finish_type_with_constraints(std::move(tr));
        }
        case token_type::kw_bit: {
            advance();
            (void)consume_identifier_value("STRING", "expected STRING after BIT");
            if (check(token_type::left_brace)) {
                auto bst = parse_bit_string_type_body();
                if (bst.is_ok()) {
                    tr.content = std::move(bst.value());
                } else {
                    tr.content = bit_string_type{};
                }
            } else {
                tr.content = bit_string_type{};
            }
            return finish_type_with_constraints(std::move(tr));
        }
        case token_type::kw_sequence: {
            advance();
            if (check(token_type::kw_of)) {
                return parse_sequence_of_type();
            }
            if (check(token_type::kw_size)) {
                return parse_constrained_sequence_of();
            }
            if (check(token_type::left_brace)) {
                auto st = parse_sequence_type();
                if (st.is_err()) {
                    return asn1pp::result<type_ref>::err(st.error());
                }
                tr.content = std::make_unique<sequence_type>(std::move(st.value()));
            } else {
                tr.content = std::make_unique<sequence_type>();
            }
            return finish_type_with_constraints(std::move(tr));
        }
        case token_type::kw_set: {
            advance();
            if (check(token_type::kw_of)) {
                return parse_set_of_type();
            }
            if (check(token_type::left_brace)) {
                auto st = parse_set_type();
                if (st.is_err()) {
                    return asn1pp::result<type_ref>::err(st.error());
                }
                tr.content = std::make_unique<set_type>(std::move(st.value()));
            } else {
                tr.content = std::make_unique<set_type>();
            }
            return finish_type_with_constraints(std::move(tr));
        }
        case token_type::kw_choice: {
            advance();
            if (check(token_type::left_brace)) {
                auto ct = parse_choice_type();
                if (ct.is_err()) {
                    return asn1pp::result<type_ref>::err(ct.error());
                }
                tr.content = std::make_unique<choice_type>(std::move(ct.value()));
            } else {
                tr.content = std::make_unique<choice_type>();
            }
            return finish_type_with_constraints(std::move(tr));
        }
        case token_type::kw_enumerated: {
            advance();
            if (check(token_type::left_brace)) {
                auto et = parse_enumerated_type();
                if (et.is_err()) {
                    return asn1pp::result<type_ref>::err(et.error());
                }
                tr.content = std::move(et.value());
            } else {
                tr.content = enumerated_type{};
            }
            return finish_type_with_constraints(std::move(tr));
        }
        case token_type::kw_selected: {
            advance();
            error(current_.loc, "SELECTED type not yet supported");
            return asn1pp::result<type_ref>::err(asn1pp::error_code::parse_error);
        }
        default:
            error(current_.loc, "expected type keyword");
            return asn1pp::result<type_ref>::err(asn1pp::error_code::parse_error);
    }

    return asn1pp::result<type_ref>::ok(std::move(tr));
}

asn1pp::result<type_ref> parser::finish_type_with_constraints(type_ref tr) {
    if (!check(token_type::left_paren)) {
        return asn1pp::result<type_ref>::ok(std::move(tr));
    }

    constrained_type ct;
    ct.underlying_type = std::make_unique<type_ref>(std::move(tr));
    while (check(token_type::left_paren)) {
        auto c = parse_constraint();
        if (c.is_err()) {
            return asn1pp::result<type_ref>::err(c.error());
        }
        ct.constraints.push_back(std::move(c.value()));
    }

    type_ref result_tr;
    result_tr.content = std::make_unique<constrained_type>(std::move(ct));
    return asn1pp::result<type_ref>::ok(std::move(result_tr));
}

asn1pp::result<type_ref> parser::parse_named_type(const std::string& name) {
    type_ref tr;
    tr.content = std::string(name);
    return asn1pp::result<type_ref>::ok(std::move(tr));
}

asn1pp::result<sequence_type> parser::parse_sequence_type() {
    sequence_type seq;
    advance();

    while (!check(token_type::right_brace) && !check(token_type::eof)) {
        if (check(token_type::ellipsis)) {
            advance();
            seq.has_extension = true;
            if (check(token_type::comma)) {
                advance();
            }
            continue;
        }
        if (check(token_type::comma)) {
            advance();
            continue;
        }
        if (is_assignment_boundary(lexer_, current_)) {
            return asn1pp::result<sequence_type>::ok(std::move(seq));
        }
        auto comp = parse_component_type();
        if (comp.is_ok()) {
            seq.components.push_back(std::move(comp.value()));
        } else {
            synchronize();
        }
    }

    if (check(token_type::right_brace)) {
        advance();
    }
    return asn1pp::result<sequence_type>::ok(std::move(seq));
}

asn1pp::result<set_type> parser::parse_set_type() {
    set_type st;
    advance();

    while (!check(token_type::right_brace) && !check(token_type::eof)) {
        if (check(token_type::ellipsis)) {
            advance();
            st.has_extension = true;
            if (check(token_type::comma)) {
                advance();
            }
            continue;
        }
        if (check(token_type::comma)) {
            advance();
            continue;
        }
        auto comp = parse_component_type();
        if (comp.is_ok()) {
            st.components.push_back(std::move(comp.value()));
        } else {
            synchronize();
        }
    }

    (void)consume(token_type::right_brace, "expected } to close SET");
    return asn1pp::result<set_type>::ok(std::move(st));
}

asn1pp::result<choice_type> parser::parse_choice_type() {
    choice_type ch;
    advance();

    while (!check(token_type::right_brace) && !check(token_type::eof)) {
        if (check(token_type::ellipsis)) {
            advance();
            ch.has_extension = true;
            if (check(token_type::comma)) {
                advance();
            }
            continue;
        }
        if (check(token_type::comma)) {
            advance();
            continue;
        }
        auto alt = parse_choice_alternative();
        if (alt.is_ok()) {
            ch.alternatives.push_back(std::move(alt.value()));
        } else {
            synchronize();
        }
    }

    (void)consume(token_type::right_brace, "expected } to close CHOICE");
    return asn1pp::result<choice_type>::ok(std::move(ch));
}

asn1pp::result<enumerated_type> parser::parse_enumerated_type() {
    enumerated_type et;
    advance();

    while (!check(token_type::right_brace) && !check(token_type::eof)) {
        if (check(token_type::ellipsis)) {
            advance();
            et.has_extension = true;
            if (check(token_type::comma)) {
                advance();
            }
            continue;
        }
        if (check(token_type::comma)) {
            advance();
            continue;
        }

        enumeration_item item;
        if (current_.type == token_type::identifier) {
            item.name = current_.value;
            advance();
            if (check(token_type::left_paren)) {
                advance();
                if (current_.type == token_type::number) {
                    item.value = parse_integer_text<int64_t>(current_.value);
                    advance();
                }
                (void)consume(token_type::right_paren, "expected ) after enumeration value");
            }
            et.values.push_back(std::move(item));
        } else {
            error(current_.loc, "expected enumeration item name");
            synchronize();
        }
    }

    (void)consume(token_type::right_brace, "expected } to close ENUMERATED");
    return asn1pp::result<enumerated_type>::ok(std::move(et));
}

asn1pp::result<bit_string_type> parser::parse_bit_string_type_body() {
    bit_string_type bst;
    advance();

    while (!check(token_type::right_brace) && !check(token_type::eof)) {
        if (check(token_type::ellipsis)) {
            advance();
            bst.has_extension = true;
            if (check(token_type::comma)) {
                advance();
            }
            continue;
        }
        if (check(token_type::comma)) {
            advance();
            continue;
        }

        named_bit bit;
        if (current_.type == token_type::identifier) {
            bit.name = current_.value;
            advance();
            if (check(token_type::left_paren)) {
                advance();
                if (current_.type == token_type::number) {
                    bit.position = parse_integer_text<int64_t>(current_.value);
                    advance();
                }
                (void)consume(token_type::right_paren, "expected ) after bit position");
            }
            bst.named_bits.push_back(std::move(bit));
        } else {
            error(current_.loc, "expected named bit identifier");
            synchronize();
        }
    }

    (void)consume(token_type::right_brace, "expected } to close BIT STRING named bits");
    return asn1pp::result<bit_string_type>::ok(std::move(bst));
}

asn1pp::result<type_ref> parser::parse_sequence_of_type() {
    advance();

    auto elem = parse_type();
    if (elem.is_err()) {
        return elem;
    }

    sequence_of_type sot;
    sot.element_type = std::make_unique<type_ref>(std::move(elem.value()));
    type_ref tr;
    tr.content = std::make_unique<sequence_of_type>(std::move(sot));
    return asn1pp::result<type_ref>::ok(std::move(tr));
}

asn1pp::result<type_ref> parser::parse_constrained_sequence_of() {
    advance();  // consume SIZE
    auto size = parse_size_constraint();  // consume (0..10) etc.
    if (size.is_err()) {
        return asn1pp::result<type_ref>::err(size.error());
    }
    (void)consume(token_type::kw_of, "expected OF after SIZE constraint in SEQUENCE OF");

    auto elem = parse_type();
    if (elem.is_err()) {
        return elem;
    }

    sequence_of_type sot;
    sot.element_type = std::make_unique<type_ref>(std::move(elem.value()));
    type_ref inner_tr;
    inner_tr.content = std::make_unique<sequence_of_type>(std::move(sot));

    constraint c;
    c.content = size.value();

    constrained_type ct;
    ct.underlying_type = std::make_unique<type_ref>(std::move(inner_tr));
    ct.constraints.push_back(std::move(c));

    type_ref tr;
    tr.content = std::make_unique<constrained_type>(std::move(ct));
    return asn1pp::result<type_ref>::ok(std::move(tr));
}

asn1pp::result<type_ref> parser::parse_set_of_type() {
    advance();

    auto elem = parse_type();
    if (elem.is_err()) {
        return elem;
    }

    set_of_type sot;
    sot.element_type = std::make_unique<type_ref>(std::move(elem.value()));
    type_ref tr;
    tr.content = std::make_unique<set_of_type>(std::move(sot));
    return asn1pp::result<type_ref>::ok(std::move(tr));
}

asn1pp::result<component_type> parser::parse_component_type() {
    component_type comp;

    if (current_.type != token_type::identifier) {
        error(current_.loc, "expected component name identifier");
        return asn1pp::result<component_type>::err(asn1pp::error_code::parse_error);
    }

    comp.name = current_.value;
    advance();

    auto ty = parse_type();
        if (ty.is_err()) {
            if (current_.type == token_type::identifier) {
                comp.type = std::make_unique<type_ref>();
                comp.type->content = std::string(current_.value);
                advance();
            } else {
                return asn1pp::result<component_type>::err(ty.error());
            }
        } else {
            comp.type = std::make_unique<type_ref>(std::move(ty.value()));
        }

    if (check(token_type::kw_optional)) {
        advance();
        comp.optional = true;
    } else if (check(token_type::kw_default)) {
        advance();
        if (current_.type == token_type::identifier || current_.type == token_type::number ||
            current_.type == token_type::kw_true || current_.type == token_type::kw_false ||
            current_.type == token_type::kw_null || current_.type == token_type::character_string ||
            current_.type == token_type::binary_string || current_.type == token_type::hex_string) {
            comp.default_value = current_.value;
            advance();
        } else if (check(token_type::left_brace)) {
            comp.default_value = "{}";
            skip_balanced(lexer_, current_, token_type::left_brace, token_type::right_brace);
        } else {
            error(current_.loc, "expected DEFAULT value");
        }
    }

    return asn1pp::result<component_type>::ok(std::move(comp));
}

asn1pp::result<choice_alternative> parser::parse_choice_alternative() {
    choice_alternative alt;

    if (current_.type != token_type::identifier) {
        error(current_.loc, "expected choice alternative name identifier");
        return asn1pp::result<choice_alternative>::err(asn1pp::error_code::parse_error);
    }

    alt.name = current_.value;
    advance();

    auto ty = parse_type();
    if (ty.is_ok()) {
        alt.type = std::make_unique<type_ref>(std::move(ty.value()));
    } else {
        if (current_.type == token_type::identifier) {
            alt.type = std::make_unique<type_ref>();
            alt.type->content = std::string(current_.value);
            advance();
        } else {
            return asn1pp::result<choice_alternative>::err(ty.error());
        }
    }

    return asn1pp::result<choice_alternative>::ok(std::move(alt));
}

asn1pp::result<tagged_type> parser::parse_tagged_type() {
    tagged_type tt;
    advance();

    if (check(token_type::kw_universal)) {
        advance();
        tt.tag_value = asn1pp::tag{asn1pp::tag_class::universal, false, 0};
    } else if (check(token_type::kw_application)) {
        advance();
        tt.tag_value = asn1pp::tag{asn1pp::tag_class::application, false, 0};
    } else if (check(token_type::kw_private)) {
        advance();
        tt.tag_value = asn1pp::tag{asn1pp::tag_class::private_class, false, 0};
    } else {
        tt.tag_value = asn1pp::tag{asn1pp::tag_class::context_specific, false, 0};
    }

    if (current_.type != token_type::number) {
        error(current_.loc, "expected tag number");
    } else {
        tt.tag_value.number = parse_integer_text<uint32_t>(current_.value);
        advance();
    }

    (void)consume(token_type::right_bracket, "expected ] after tag");

    if (check(token_type::kw_implicit)) {
        advance();
        tt.implicit = true;
    } else if (check(token_type::kw_explicit)) {
        advance();
        tt.implicit = false;
    }

    auto ty = parse_type();
    if (ty.is_ok()) {
        tt.underlying_type = std::make_unique<type_ref>(std::move(ty.value()));
    } else {
        return asn1pp::result<tagged_type>::err(ty.error());
    }

    return asn1pp::result<tagged_type>::ok(std::move(tt));
}

asn1pp::result<constraint> parser::parse_constraint() {
    advance();

    constraint c;

    if (check(token_type::kw_size)) {
        advance();
        auto sc = parse_size_constraint();
        if (sc.is_err()) {
            return asn1pp::result<constraint>::err(sc.error());
        }
        c.content = sc.value();
        (void)consume(token_type::right_paren, "expected ) after constraint");
        return asn1pp::result<constraint>::ok(std::move(c));
    }

    if (check(token_type::kw_all)) {
        advance();
        if (current_.type == token_type::identifier && equals_ignore_case(current_.value, "EXCEPT")) {
            advance();
            while (!check(token_type::right_paren) && !check(token_type::eof)) {
                if (check(token_type::left_paren)) {
                    skip_balanced(lexer_, current_, token_type::left_paren, token_type::right_paren);
                    continue;
                }
                if (check(token_type::left_brace)) {
                    skip_balanced(lexer_, current_, token_type::left_brace, token_type::right_brace);
                    continue;
                }
                advance();
            }
            c.content = extension_constraint{};
            (void)consume(token_type::right_paren, "expected ) after ALL EXCEPT constraint");
            return asn1pp::result<constraint>::ok(std::move(c));
        }
        error(current_.loc, "expected EXCEPT after ALL in constraint");
        return asn1pp::result<constraint>::err(asn1pp::error_code::parse_error);
    }

    if (check(token_type::ellipsis)) {
        advance();
        c.content = extension_constraint{};
        (void)consume(token_type::right_paren, "expected ) after extension constraint");
        return asn1pp::result<constraint>::ok(std::move(c));
    }

    if (check(token_type::kw_from)) {
        advance();
        (void)consume(token_type::left_paren, "expected ( after FROM");
        auto vr = parse_value_range();
        if (vr.is_err()) {
            return asn1pp::result<constraint>::err(vr.error());
        }
        c.content = vr.value();
        (void)consume(token_type::right_paren, "expected ) after FROM range");
        (void)consume(token_type::right_paren, "expected ) after FROM constraint");
        return asn1pp::result<constraint>::ok(std::move(c));
    }

    auto vr = parse_value_range();
    if (vr.is_err()) {
        return asn1pp::result<constraint>::err(vr.error());
    }
    c.content = vr.value();

    (void)consume(token_type::right_paren, "expected ) after constraint");
    return asn1pp::result<constraint>::ok(std::move(c));
}

asn1pp::result<value_range_constraint> parser::parse_value_range() {
    value_range_constraint vr;

    if (check(token_type::kw_min)) {
        advance();
        vr.min_inclusive = true;
    } else if (current_.type == token_type::number) {
        vr.min_value = parse_integer_text<int64_t>(current_.value);
        advance();
    } else if (current_.type == token_type::character_string) {
        std::string s = current_.value;
        advance();
        if (!s.empty()) vr.min_value = static_cast<int64_t>(static_cast<uint8_t>(s[0]));
    } else {
        error(current_.loc, "expected range lower bound");
        return asn1pp::result<value_range_constraint>::err(asn1pp::error_code::parse_error);
    }

    if (!check(token_type::range)) {
        vr.max_value = vr.min_value;
        return asn1pp::result<value_range_constraint>::ok(vr);
    }
    advance();

    if (check(token_type::kw_max)) {
        advance();
        vr.max_inclusive = true;
    } else if (current_.type == token_type::number) {
        vr.max_value = parse_integer_text<int64_t>(current_.value);
        advance();
    } else if (current_.type == token_type::character_string) {
        std::string s = current_.value;
        advance();
        if (!s.empty()) vr.max_value = static_cast<int64_t>(static_cast<uint8_t>(s[0]));
    } else {
        error(current_.loc, "expected range upper bound");
        return asn1pp::result<value_range_constraint>::err(asn1pp::error_code::parse_error);
    }

    return asn1pp::result<value_range_constraint>::ok(vr);
}

asn1pp::result<size_constraint> parser::parse_size_constraint() {
    size_constraint sc;
    (void)consume(token_type::left_paren, "expected ( after SIZE");

    if (current_.type == token_type::number) {
        sc.min_size = parse_integer_text<size_t>(current_.value);
        advance();

        if (check(token_type::range)) {
            advance();
            if (current_.type == token_type::number) {
                sc.max_size = parse_integer_text<size_t>(current_.value);
                advance();
            } else {
                error(current_.loc, "expected SIZE upper bound");
                return asn1pp::result<size_constraint>::err(asn1pp::error_code::parse_error);
            }
        } else {
            sc.max_size = sc.min_size;
        }
    } else {
        error(current_.loc, "expected SIZE value");
        return asn1pp::result<size_constraint>::err(asn1pp::error_code::parse_error);
    }

    (void)consume(token_type::right_paren, "expected ) after SIZE constraint body");
    return asn1pp::result<size_constraint>::ok(sc);
}

asn1pp::result<value_ref> parser::parse_value() {
    value_ref vr;

    if (check(token_type::kw_true)) {
        advance();
        vr.content = true;
    } else if (check(token_type::kw_false)) {
        advance();
        vr.content = false;
    } else if (check(token_type::kw_null)) {
        advance();
        vr.content = null_value{};
    } else if (current_.type == token_type::number) {
        const int64_t val = parse_integer_text<int64_t>(current_.value);
        advance();
        vr.content = val;
    } else if (current_.type == token_type::binary_string || current_.type == token_type::hex_string) {
        std::vector<uint8_t> bytes;
        bytes.reserve(current_.value.size());
        for (char ch : current_.value) {
            bytes.push_back(static_cast<uint8_t>(ch));
        }
        advance();
        vr.content = std::move(bytes);
    } else if (check(token_type::left_brace)) {
        std::string value = "{";
        advance();
        while (!check(token_type::right_brace) && !check(token_type::eof)) {
            if (!value.empty() && value.back() != '{') {
                value.push_back(' ');
            }
            value += current_.value;
            advance();
        }
        (void)consume(token_type::right_brace, "expected } after value");
        value += "}";
        vr.content = std::move(value);
    } else if (current_.type == token_type::identifier || current_.type == token_type::character_string) {
        std::string s = current_.value;
        advance();
        vr.content = std::move(s);
    } else {
        error(current_.loc, "expected value literal");
        return asn1pp::result<value_ref>::err(asn1pp::error_code::parse_error);
    }

    return asn1pp::result<value_ref>::ok(std::move(vr));
}

asn1pp::result<parameterized_type_assignment> parser::parse_parameterized_type_assignment(
    const std::string& name, source_location)
{
    parameterized_type_assignment pta;
    pta.name = name;

    advance();

    while (!check(token_type::right_brace) && !check(token_type::eof)) {
        if (check(token_type::comma)) {
            advance();
            continue;
        }
        auto fp = parse_formal_parameter();
        if (fp.is_ok()) {
            pta.parameters.push_back(fp.value());
        } else {
            synchronize();
            return asn1pp::result<parameterized_type_assignment>::err(fp.error());
        }
    }

    if (!consume_or_recover(token_type::right_brace, "expected } after parameter list")) {
        return asn1pp::result<parameterized_type_assignment>::err(asn1pp::error_code::parse_error);
    }

    if (!check(token_type::assignment)) {
        error(current_.loc, "expected ::= after parameter list");
        return asn1pp::result<parameterized_type_assignment>::err(asn1pp::error_code::parse_error);
    }
    advance();

    auto ty_result = parse_type();
    if (ty_result.is_err()) {
        return asn1pp::result<parameterized_type_assignment>::err(ty_result.error());
    }
    pta.type = std::make_unique<type_ref>(std::move(ty_result.value()));

    return asn1pp::result<parameterized_type_assignment>::ok(std::move(pta));
}

asn1pp::result<formal_parameter> parser::parse_formal_parameter() {
    formal_parameter fp;

    if (current_.type == token_type::identifier && !is_type_keyword()) {
        fp.name = current_.value;
        advance();

        if (check(token_type::colon)) {
            advance();
            if (is_type_keyword() || current_.type == token_type::identifier) {
                fp.param_type = fp.name;
                fp.name = current_.value;
                advance();
            } else {
                error(current_.loc, "expected parameter name after :");
                return asn1pp::result<formal_parameter>::err(asn1pp::error_code::parse_error);
            }
        }
        return asn1pp::result<formal_parameter>::ok(std::move(fp));
    }

    if (is_type_keyword()) {
        std::string type_name;
        if (current_.type == token_type::identifier) {
            type_name = current_.value;
            advance();
        } else {
            type_name = token_type_to_string(current_.type);
            advance();
        }

        if (!consume_or_recover(token_type::colon, "expected : in formal parameter")) {
            return asn1pp::result<formal_parameter>::err(asn1pp::error_code::parse_error);
        }

        if (current_.type == token_type::identifier) {
            fp.name = current_.value;
            advance();
        } else {
            error(current_.loc, "expected parameter name after :");
            return asn1pp::result<formal_parameter>::err(asn1pp::error_code::parse_error);
        }

        fp.param_type = type_name;
        return asn1pp::result<formal_parameter>::ok(std::move(fp));
    }

    error(current_.loc, "expected formal parameter");
    return asn1pp::result<formal_parameter>::err(asn1pp::error_code::parse_error);
}

asn1pp::result<type_instantiation> parser::parse_type_instantiation(const std::string& type_name) {
    type_instantiation ti;
    ti.type_name = type_name;

    advance();  // consume {

    while (!check(token_type::right_brace) && !check(token_type::eof)) {
        if (check(token_type::comma)) {
            advance();
            continue;
        }

        actual_parameter ap;
        if (is_type_keyword() || current_.type == token_type::identifier) {
            auto ty = parse_type();
            if (ty.is_err()) {
                return asn1pp::result<type_instantiation>::err(ty.error());
            }
            ap.value = std::make_unique<type_ref>(std::move(ty.value()));
        } else if (current_.type == token_type::number) {
            ap.value = current_.value;
            advance();
        } else {
            error(current_.loc, "expected type or value in parameter list");
            return asn1pp::result<type_instantiation>::err(asn1pp::error_code::parse_error);
        }
        ti.arguments.push_back(std::move(ap));
    }

    if (!consume_or_recover(token_type::right_brace, "expected } after parameter list")) {
        return asn1pp::result<type_instantiation>::err(asn1pp::error_code::parse_error);
    }

    return asn1pp::result<type_instantiation>::ok(std::move(ti));
}

// ========================================================================
// Information Object Class parsing (X.681)
// ========================================================================

asn1pp::result<class_type> parser::parse_class_def(const std::string& name) {
    advance();  // consume CLASS

    class_type ct;
    ct.name = name;

    if (!check(token_type::left_brace)) {
        error(current_.loc, "expected { after CLASS");
        return asn1pp::result<class_type>::err(asn1pp::error_code::parse_error);
    }
    advance();  // consume {

    while (!check(token_type::right_brace) && !check(token_type::eof)) {
        if (check(token_type::comma)) {
            advance();
            continue;
        }

        class_field cf;
        if (check(token_type::ampersand)) {
            advance();
            if (current_.type != token_type::identifier) {
                error(current_.loc, "expected field name after &");
                return asn1pp::result<class_type>::err(asn1pp::error_code::parse_error);
            }
            cf.name = "&" + current_.value;
            advance();

            if (is_type_keyword() || current_.type == token_type::identifier) {
                cf.type_name = current_.value;
                advance();
                // Handle multi-word types: OCTET STRING, BIT STRING, OBJECT IDENTIFIER
                if ((cf.type_name == "OCTET" && check_identifier("STRING")) ||
                    (cf.type_name == "BIT" && check_identifier("STRING")) ||
                    (cf.type_name == "OBJECT" && check_identifier("IDENTIFIER"))) {
                    *cf.type_name += " " + current_.value;
                    advance();
                }
            }

            if (check(token_type::kw_unique)) {
                cf.unique = true;
                advance();
            }

            if (check(token_type::kw_optional)) {
                cf.optional = true;
                advance();
            }
            ct.fields.push_back(std::move(cf));
        } else {
            error(current_.loc, "expected &field in CLASS definition");
            synchronize();
            return asn1pp::result<class_type>::err(asn1pp::error_code::parse_error);
        }
    }

    (void)consume(token_type::right_brace, "expected } after CLASS fields");
    known_classes_.insert(name);

    if (check(token_type::kw_with)) {
        advance();
        if (!check_identifier("SYNTAX") && !check(token_type::kw_syntax)) {
            error(current_.loc, "expected SYNTAX after WITH");
            return asn1pp::result<class_type>::err(asn1pp::error_code::parse_error);
        }
        advance();

        if (!check(token_type::left_brace)) {
            error(current_.loc, "expected { after WITH SYNTAX");
            return asn1pp::result<class_type>::err(asn1pp::error_code::parse_error);
        }
        advance();

        while (!check(token_type::right_brace) && !check(token_type::eof)) {
            with_syntax_item wsi;

            if (check(token_type::character_string)) {
                wsi.literal = current_.value;
                advance();
            } else if (current_.type == token_type::identifier ||
                       current_.type == token_type::kw_class ||
                       current_.type == token_type::kw_with ||
                       current_.type == token_type::kw_syntax ||
                       current_.type == token_type::kw_integer ||
                       current_.type == token_type::kw_boolean ||
                       current_.type == token_type::kw_octet ||
                       current_.type == token_type::kw_size ||
                       current_.type == token_type::kw_constraint) {
                wsi.literal = current_.value;
                advance();
            }

            if (check(token_type::ampersand)) {
                advance();
                if (current_.type == token_type::identifier) {
                    wsi.field_ref = "&" + current_.value;
                    advance();
                }
            }

            if (!wsi.literal.empty() || wsi.field_ref.has_value()) {
                ct.with_syntax.push_back(std::move(wsi));
            } else {
                break;
            }
        }

        (void)consume(token_type::right_brace, "expected } after WITH SYNTAX");
    }

    return asn1pp::result<class_type>::ok(std::move(ct));
}

asn1pp::result<information_object> parser::parse_information_object_def(
    const std::string& name, const std::string& class_name, source_location)
{
    advance();  // consume {

    information_object obj;
    obj.name = name;
    obj.class_name = class_name;

    while (!check(token_type::right_brace) && !check(token_type::eof)) {
        if (check(token_type::comma)) {
            advance();
            continue;
        }

        information_object_field_value fv;
        if (check(token_type::ampersand)) {
            advance();
            if (current_.type == token_type::identifier) {
                fv.field_name = "&" + current_.value;
                advance();

                auto val = parse_value();
                if (val.is_ok()) {
                    fv.value = val.value();
                }
                obj.field_values.push_back(std::move(fv));
            } else {
                error(current_.loc, "expected field name after &");
                break;
            }
        } else {
            error(current_.loc, "expected &fieldName in object value");
            break;
        }
    }

    (void)consume(token_type::right_brace, "expected } after object field values");

    return asn1pp::result<information_object>::ok(std::move(obj));
}

asn1pp::result<object_set> parser::parse_object_set_def(
    const std::string& name, const std::string& class_name)
{
    advance();  // consume {

    object_set os;
    os.name = name;
    os.class_name = class_name;

    while (!check(token_type::right_brace) && !check(token_type::eof)) {
        if (check(token_type::comma)) {
            advance();
            continue;
        }

        if (check(token_type::ellipsis)) {
            os.has_extension = true;
            advance();
            continue;
        }

        if (current_.type == token_type::identifier) {
            os.objects.push_back(current_.value);
            advance();
        } else {
            error(current_.loc, "expected object name in object set");
            break;
        }
    }

    (void)consume(token_type::right_brace, "expected } after object set");

    return asn1pp::result<object_set>::ok(std::move(os));
}

}  // namespace asn1pp::gen
