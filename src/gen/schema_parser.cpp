#include "schema_parser.hpp"

#include "lexer.hpp"
#include "parser.hpp"

#include <fstream>
#include <iterator>
#include <string>

namespace asn1pp::gen {

namespace {

[[nodiscard]] bool source_has_imports(std::string_view source,
                                      std::string_view filename,
                                      diagnostic_engine& diag) {
    lexer lex(source, filename, &diag);

    for (;;) {
        const token tok = lex.next_token();
        if (tok.type == token_type::kw_imports) {
            diag.add_warning(tok.loc, "IMPORTS are parsed but external module resolution is not implemented");
            return true;
        }
        if (tok.type == token_type::eof || tok.type == token_type::error) {
            return false;
        }
    }
}

}  // namespace

asn1pp::result<module_definition> schema_parser::parse_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        diag_.add_error(source_location{path, 0, 0}, "failed to open ASN.1 schema file");
        return asn1pp::result<module_definition>::err(asn1pp::error_code::parse_error);
    }

    std::string source{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if (input.bad()) {
        diag_.add_error(source_location{path, 0, 0}, "failed to read ASN.1 schema file");
        return asn1pp::result<module_definition>::err(asn1pp::error_code::parse_error);
    }

    return parse_string(source, path);
}

asn1pp::result<module_definition> schema_parser::parse_string(std::string_view source,
                                                              std::string_view filename) {
    diag_.clear();
    (void)source_has_imports(source, filename, diag_);

    parser p(source, filename, &diag_);
    auto result = p.parse_module();
    if (result.is_err()) {
        return asn1pp::result<module_definition>::err(result.error());
    }
    if (diag_.has_errors()) {
        return asn1pp::result<module_definition>::err(asn1pp::error_code::parse_error);
    }

    return result;
}

}  // namespace asn1pp::gen
