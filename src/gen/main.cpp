#include "cli.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "diagnostics.hpp"
#include "generator.hpp"

#include <algorithm>
#include <array>
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

static constexpr std::array valid_encodings = {
    std::string_view{"ber"},  std::string_view{"der"},  std::string_view{"per"},
    std::string_view{"uper"}, std::string_view{"oer"},  std::string_view{"coer"},
    std::string_view{"xer"},  std::string_view{"cxer"}, std::string_view{"jer"},
};

bool is_valid_encoding(std::string_view enc) {
    return std::find(valid_encodings.begin(), valid_encodings.end(), enc) != valid_encodings.end();
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

    // Validate encoding value
    if (!args.encoding_rule.empty() && !is_valid_encoding(args.encoding_rule)) {
        std::cerr << "error: unknown encoding rule: " << args.encoding_rule << "\n";
        return 1;
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

    // Generate C++ output
    const auto& module = module_result.value();
    asn1pp::gen::generator gen;
    asn1pp::gen::emitter_options opts;

    // If --output specified, write to file; otherwise write to stdout
    if (!args.output_dir.empty()) {
        std::ofstream out(args.output_dir);
        if (!out) {
            std::cerr << "error: failed to open output file: " << args.output_dir << "\n";
            return 1;
        }
        auto gen_result = gen.generate(module, opts, out);
        if (!gen_result.is_ok()) {
            std::cerr << "error: code generation failed\n";
            return 1;
        }
    } else {
        auto gen_result = gen.generate(module, opts, std::cout);
        if (!gen_result.is_ok()) {
            std::cerr << "error: code generation failed\n";
            return 1;
        }
    }

    return 0;
}