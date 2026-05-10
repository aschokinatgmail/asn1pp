#include "cli.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "diagnostics.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <cstdlib>

namespace {

bool file_exists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        return {};
    }
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

}  // namespace

int main(int argc, char** argv) {
    auto result = asn1pp::gen::parse_args(argc, argv);

    if (!result.is_ok()) {
        std::cerr << "error: failed to parse arguments\n";
        return 1;
    }

    const auto& args = result.value();

    if (args.help) {
        asn1pp::gen::print_help();
        return 0;
    }

    if (args.version) {
        asn1pp::gen::print_version();
        return 0;
    }

    // Validate input file exists (not required for dry-run)
    if (!args.dry_run && !file_exists(args.input_file)) {
        std::cerr << "error: input file not found: " << args.input_file << "\n";
        return 1;
    }

    // Read input file
    std::string source;
    if (!args.dry_run) {
        source = read_file(args.input_file);
        if (source.empty() && !args.input_file.empty()) {
            std::cerr << "error: failed to read input file: " << args.input_file << "\n";
            return 1;
        }
    }

    // Set up diagnostic engine
    asn1pp::gen::diagnostic_engine diag;

    // Lex and parse
    asn1pp::gen::parser p(source, args.input_file, &diag);
    auto module_result = p.parse_module();

    if (diag.has_errors()) {
        std::cerr << diag.format_all();
        return 1;
    }

    if (!module_result.is_ok()) {
        std::cerr << "error: parse failed\n";
        return 1;
    }

    if (args.dry_run) {
        std::cout << "Parsing successful\n";
        return 0;
    }

    std::cout << "Processing successful\n";
    return 0;
}