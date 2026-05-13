#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../../src/gen/schema_parser.hpp"

#include <filesystem>
#include <string_view>

using namespace asn1pp::gen;
namespace {

const type_assignment& expect_type_assignment(const module_definition& module,
                                             size_t index,
                                             std::string_view expected_name) {
    EXPECT_LT(index, module.assignments.size());
    const auto& assignment = module.assignments.at(index).content;
    EXPECT_TRUE(std::holds_alternative<type_assignment>(assignment));

    const auto& type = std::get<type_assignment>(assignment);
    EXPECT_EQ(type.name, expected_name);
    EXPECT_NE(type.type, nullptr);
    return type;
}

void expect_integer_assignment(const module_definition& module,
                               size_t index,
                               std::string_view expected_name) {
    const auto& type = expect_type_assignment(module, index, expected_name);
    ASSERT_NE(type.type, nullptr);
    EXPECT_TRUE(type.type->holds_alternative<integer_type>());
}

std::filesystem::path simple_asn_path() {
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "simple.asn";
}

}  // namespace

TEST(SchemaParser, ParseValidModule_UT_GSP_001) {
    schema_parser parser;

    auto result = parser.parse_string("MyModule DEFINITIONS ::= BEGIN X ::= INTEGER END", "valid.asn");

    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(parser.diagnostics().has_errors());
    EXPECT_EQ(parser.diagnostics().warning_count(), 0U);

    const auto& module = result.value();
    EXPECT_EQ(module.name, "MyModule");
    ASSERT_EQ(module.assignments.size(), 1U);
    expect_integer_assignment(module, 0, "X");
}

TEST(SchemaParser, ParseEmptyModule_UT_GSP_002) {
    schema_parser parser;

    auto result = parser.parse_string("EmptyModule DEFINITIONS ::= BEGIN END", "empty.asn");

    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(parser.diagnostics().has_errors());

    const auto& module = result.value();
    EXPECT_EQ(module.name, "EmptyModule");
    EXPECT_TRUE(module.assignments.empty());
}

TEST(SchemaParser, ParseMultipleTypeDefinitions_UT_GSP_003) {
    schema_parser parser;

    auto result = parser.parse_string(R"ASN(
        MultiModule DEFINITIONS ::= BEGIN
            Age ::= INTEGER
            Enabled ::= BOOLEAN
            Payload ::= OCTET STRING
        END
    )ASN", "multiple.asn");

    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(parser.diagnostics().has_errors());

    const auto& module = result.value();
    EXPECT_EQ(module.name, "MultiModule");
    ASSERT_EQ(module.assignments.size(), 3U);
    expect_integer_assignment(module, 0, "Age");

    const auto& enabled = expect_type_assignment(module, 1, "Enabled");
    ASSERT_NE(enabled.type, nullptr);
    EXPECT_TRUE(enabled.type->holds_alternative<boolean_type>());

    const auto& payload = expect_type_assignment(module, 2, "Payload");
    ASSERT_NE(payload.type, nullptr);
    EXPECT_TRUE(payload.type->holds_alternative<octet_string_type>());
}

TEST(SchemaParser, ParseInvalidSyntax_UT_GSP_004) {
    schema_parser parser;

    auto result = parser.parse_string("BrokenModule DEFINITIONS ::= BEGIN X INTEGER END", "broken.asn");

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error(), asn1pp::error_code::parse_error);
    EXPECT_TRUE(parser.diagnostics().has_errors());
    EXPECT_GT(parser.diagnostics().error_count(), 0U);
}

TEST(SchemaParser, ParseImportsWarns_UT_GSP_005) {
    schema_parser parser;

    auto result = parser.parse_string(R"ASN(
        ImportingModule DEFINITIONS ::= BEGIN
            IMPORTS ExternalType FROM ExternalModule;
            LocalType ::= INTEGER
        END
    )ASN", "imports.asn");

    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(parser.diagnostics().has_errors());
    EXPECT_GT(parser.diagnostics().warning_count(), 0U);

    const auto& module = result.value();
    EXPECT_EQ(module.name, "ImportingModule");
    ASSERT_EQ(module.assignments.size(), 1U);
    expect_integer_assignment(module, 0, "LocalType");
}

TEST(SchemaParser, ParseFileReadsSimpleAsn_UT_GSP_006) {
    schema_parser parser;

    auto result = parser.parse_file(simple_asn_path().string());

    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(parser.diagnostics().has_errors());

    const auto& module = result.value();
    EXPECT_EQ(module.name, "MyModule");
    ASSERT_EQ(module.assignments.size(), 1U);
    expect_integer_assignment(module, 0, "X");
}
