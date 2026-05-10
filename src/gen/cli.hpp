#pragma once

#include "../../libs/codec/result.hpp"

#include <string>

namespace asn1pp::gen {

struct cli_args {
    std::string input_file;
    std::string output_dir;
    std::string encoding_rule;
    bool dry_run = false;
    bool help = false;
    bool version = false;
};

using cli_result = asn1pp::result<cli_args>;

cli_result parse_args(int argc, char** argv);
void print_help();
void print_version();

}  // namespace asn1pp::gen
