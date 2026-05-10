#include "cli.hpp"

#include <iostream>
#include <cstring>
#include <string>
#include <string_view>

namespace asn1pp::gen {

namespace {
constexpr std::string_view version_string = "asn1pp-gen 0.1.0";

bool starts_with(std::string_view sv, std::string_view prefix) noexcept {
    return sv.size() >= prefix.size() && sv.substr(0, prefix.size()) == prefix;
}
}  // namespace

cli_result parse_args(int argc, char** argv) {
    cli_args args;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "--help") {
            args.help = true;
            return asn1pp::result<cli_args>::ok(std::move(args));
        }
        if (arg == "--version") {
            args.version = true;
            return asn1pp::result<cli_args>::ok(std::move(args));
        }
        if (arg == "--dry-run") {
            args.dry_run = true;
            continue;
        }
        if (starts_with(arg, "--input=")) {
            args.input_file = std::string(arg.substr(8));
            continue;
        }
        if (arg == "--input") {
            if (i + 1 >= argc) {
                std::cerr << "error: --input requires a value\n";
                return asn1pp::result<cli_args>::err(asn1pp::error_code::parse_error);
            }
            args.input_file = std::string(argv[++i]);
            continue;
        }
        if (starts_with(arg, "--output=")) {
            args.output_dir = std::string(arg.substr(9));
            continue;
        }
        if (arg == "--output") {
            if (i + 1 >= argc) {
                std::cerr << "error: --output requires a value\n";
                return asn1pp::result<cli_args>::err(asn1pp::error_code::parse_error);
            }
            args.output_dir = std::string(argv[++i]);
            continue;
        }
        if (starts_with(arg, "--encoding=")) {
            args.encoding_rule = std::string(arg.substr(11));
            continue;
        }
        if (arg == "--encoding") {
            if (i + 1 >= argc) {
                std::cerr << "error: --encoding requires a value\n";
                return asn1pp::result<cli_args>::err(asn1pp::error_code::parse_error);
            }
            args.encoding_rule = std::string(argv[++i]);
            continue;
        }

        std::cerr << "error: unknown option: " << arg << "\n";
        return asn1pp::result<cli_args>::err(asn1pp::error_code::parse_error);
    }

    if (args.input_file.empty() && !args.dry_run) {
        std::cerr << "error: --input is required\n";
        return asn1pp::result<cli_args>::err(asn1pp::error_code::parse_error);
    }

    return asn1pp::result<cli_args>::ok(std::move(args));
}

void print_help() {
    std::cout << "Usage: asn1pp-gen [options]\n";
    std::cout << "\n";
    std::cout << "Options:\n";
    std::cout << "  --input <file>      Input ASN.1 file (.asn)\n";
    std::cout << "  --output <dir>      Output directory for generated files\n";
    std::cout << "  --encoding <rule>   Target encoding rule (ber, der, per, uper, oer, xer, jer)\n";
    std::cout << "  --dry-run           Parse and validate only, don't emit files\n";
    std::cout << "  --help              Print usage and exit\n";
    std::cout << "  --version           Print version and exit\n";
}

void print_version() {
    std::cout << version_string << "\n";
}

}  // namespace asn1pp::gen
