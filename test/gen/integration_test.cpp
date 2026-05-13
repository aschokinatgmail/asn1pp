#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../../src/gen/generator.hpp"
#include "../../src/gen/schema_parser.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

using ::testing::HasSubstr;

std::filesystem::path data_path(std::string_view filename) {
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "data" /
           std::string(filename);
}

asn1pp::gen::emitter_options make_options(std::string namespace_name = "generated") {
    asn1pp::gen::emitter_options opts;
    opts.namespace_name = std::move(namespace_name);
    return opts;
}

std::string parse_and_generate(std::string_view filename, std::string namespace_name = "generated") {
    asn1pp::gen::schema_parser parser;
    auto module = parser.parse_file(data_path(filename).string());
    EXPECT_FALSE(parser.diagnostics().has_errors());
    EXPECT_TRUE(module.is_ok());
    if (module.is_err()) return {};

    asn1pp::gen::generator gen;
    std::ostringstream output;
    auto generated = gen.generate(module.value(), make_options(std::move(namespace_name)), output);
    EXPECT_TRUE(generated.is_ok());
    if (generated.is_err()) return {};

    return output.str();
}

std::string shell_quote(const std::filesystem::path& path) {
    std::string quoted = "'";
    for (char ch : path.string()) {
        if (ch == '\'') quoted += "'\\''";
        else quoted += ch;
    }
    quoted += "'";
    return quoted;
}

std::filesystem::path write_temp_file(std::string_view stem,
                                      std::string_view extension,
                                      std::string_view contents) {
    const auto dir = std::filesystem::temp_directory_path() / "asn1pp_gen_integration_tests";
    std::filesystem::create_directories(dir);

    auto path = dir / (std::string(stem) + "_" +
                       std::to_string(::testing::UnitTest::GetInstance()->random_seed()) +
                       std::string(extension));
    std::ofstream file(path);
    file << contents;
    return path;
}

int run_command(const std::string& command) {
    int status = std::system(command.c_str());
    if (status == -1) return status;
#if defined(WIFEXITED) && defined(WEXITSTATUS)
    if (WIFEXITED(status)) return WEXITSTATUS(status);
#endif
    return status;
}

void expect_syntax_only_compiles(std::string_view stem, const std::string& generated_code) {
    const auto header = write_temp_file(stem, ".hpp", generated_code);
    const auto include_dir = std::filesystem::temp_directory_path() /
                             "asn1pp_gen_integration_tests" / "include";
    std::filesystem::create_directories(include_dir / "asn1pp");

    {
        std::ofstream codec(include_dir / "asn1pp" / "codec.hpp");
        codec << "#pragma once\n#include \"codec/codec_interface.hpp\"\n"
                 "using asn1pp::asn1_tag;\n"
                 "using asn1pp::error_code;\n"
                 "using asn1pp::result;\n"
                 "using asn1pp::universal_tag;\n";
    }
    {
        std::ofstream traits(include_dir / "asn1pp" / "traits.hpp");
        traits << "#pragma once\n#include \"codec/traits.hpp\"\n";
    }

    const auto libs_dir = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() /
                          "libs";
    const auto command = "g++ -std=c++20 -fsyntax-only -I" + shell_quote(include_dir) +
                         " -I" + shell_quote(libs_dir) + " " + shell_quote(header);

    EXPECT_EQ(run_command(command), 0) << command;
}

}  // namespace

TEST(GenIntegration, SimpleAsnGeneratedCppCompiles_E2E_GEN_001) {
    const auto code = parse_and_generate("simple.asn", "asn1pp");

    ASSERT_FALSE(code.empty());
    EXPECT_THAT(code, HasSubstr("struct X"));
    expect_syntax_only_compiles("simple", code);
}

TEST(GenIntegration, SequenceAsnGeneratedCodeHasExpectedStructFields_E2E_GEN_002) {
    const auto code = parse_and_generate("sequence.asn");

    ASSERT_FALSE(code.empty());
    EXPECT_THAT(code, HasSubstr("struct MySequence"));
    EXPECT_THAT(code, HasSubstr("int64_t intField{}"));
    EXPECT_THAT(code, HasSubstr("std::vector<uint8_t> strField{}"));
    EXPECT_THAT(code, HasSubstr("bool flag{}"));
}

TEST(GenIntegration, ChoiceAsnGeneratedCodeUsesStdVariant_E2E_GEN_003) {
    const auto code = parse_and_generate("choice.asn");

    ASSERT_FALSE(code.empty());
    EXPECT_THAT(code, HasSubstr("struct MyChoice"));
    EXPECT_THAT(code, HasSubstr("std::variant<int64_t, bool> value"));
    EXPECT_THAT(code, HasSubstr("is_intVal"));
    EXPECT_THAT(code, HasSubstr("is_flag"));
}

TEST(GenIntegration, RealisticAsnGeneratedCppCompiles_E2E_GEN_004) {
    const auto code = parse_and_generate("realistic.asn", "asn1pp");

    ASSERT_FALSE(code.empty());
    EXPECT_THAT(code, HasSubstr("struct ProtocolMessage"));
    expect_syntax_only_compiles("realistic", code);
}

TEST(GenIntegration, BadSyntaxAsnReportsMeaningfulParseError_E2E_GEN_005) {
    asn1pp::gen::schema_parser parser;

    auto module = parser.parse_file(data_path("bad.asn").string());

    if (module.is_err()) {
        EXPECT_EQ(module.error(), asn1pp::error_code::parse_error);
        EXPECT_TRUE(parser.diagnostics().has_errors());
        EXPECT_GT(parser.diagnostics().error_count(), 0U);
        return;
    }

    asn1pp::gen::generator gen;
    std::ostringstream output;
    auto generated = gen.generate(module.value(), make_options(), output);
    ASSERT_TRUE(generated.is_err())
        << "Malformed schema must fail in SchemaParser or Generator, not produce code:\n"
        << output.str();
    EXPECT_EQ(generated.error(), asn1pp::error_code::parse_error);
}

TEST(GenIntegration, GeneratedCodeIncludesCodecHeadersForCodecLinkage_E2E_GEN_006) {
    const auto code = parse_and_generate("simple.asn");

    ASSERT_FALSE(code.empty());
    EXPECT_THAT(code, HasSubstr("#include \"asn1pp/codec.hpp\""));
    EXPECT_THAT(code, HasSubstr("#include \"asn1pp/traits.hpp\""));
}
