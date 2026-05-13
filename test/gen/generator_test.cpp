#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../libs/codec/result.hpp"
#include "../../src/gen/ast.hpp"
#include "../../src/gen/generator.hpp"

namespace {

using namespace asn1pp::gen;
using ::testing::ContainsRegex;
using ::testing::HasSubstr;

type_ref make_integer() {
    type_ref t;
    t.content = integer_type{};
    return t;
}

type_ref make_boolean() {
    type_ref t;
    t.content = boolean_type{};
    return t;
}

type_ref make_octet_string() {
    type_ref t;
    t.content = octet_string_type{};
    return t;
}

type_ref make_named_type(const std::string& name) {
    type_ref t;
    t.content = name;
    return t;
}

component_type make_component(const std::string& name, type_ref type) {
    component_type c;
    c.name = name;
    c.type = std::make_unique<type_ref>(std::move(type));
    return c;
}

type_ref make_sequence(std::vector<component_type> components) {
    type_ref t;
    auto seq = std::make_unique<sequence_type>();
    seq->components = std::move(components);
    t.content = std::move(seq);
    return t;
}

type_assignment make_type_assignment(const std::string& name, type_ref type) {
    type_assignment ta;
    ta.name = name;
    ta.type = std::make_unique<type_ref>(std::move(type));
    return ta;
}

assignment make_assignment(type_assignment ta) {
    assignment a;
    a.content = std::move(ta);
    return a;
}

module_definition make_module(std::vector<assignment> assignments) {
    module_definition module;
    module.name = "TestModule";
    module.default_tagging = tag_default::automatic_tag;
    module.assignments = std::move(assignments);
    return module;
}

module_definition make_module_with_type(const std::string& name, type_ref type) {
    std::vector<assignment> assignments;
    assignments.push_back(make_assignment(make_type_assignment(name, std::move(type))));
    return make_module(std::move(assignments));
}

template<typename... Args>
[[maybe_unused]] std::vector<component_type> components(Args&&... args) {
    std::vector<component_type> v;
    v.reserve(sizeof...(Args));
    (v.push_back(std::forward<Args>(args)), ...);
    return v;
}

emitter_options make_options() {
    emitter_options opts;
    opts.namespace_name = "generated";
    return opts;
}

}  // namespace

TEST(Generator, Construction_UT_GEN_001) {
    generator gen;
    std::ostringstream output;

    auto module = make_module_with_type("Counter", make_integer());
    auto result = gen.generate(module, make_options(), output);

    ASSERT_TRUE(result.is_ok())
        << "Generator construction should register the INTEGER emitter by default";
    EXPECT_THAT(output.str(), HasSubstr("struct Counter"));
    EXPECT_THAT(output.str(), HasSubstr("int64_t value{}"));
}

TEST(Generator, EmptyModuleReturnsParseError_UT_GEN_002) {
    generator gen;
    std::ostringstream output;

    auto module = make_module(std::vector<assignment>{});
    auto result = gen.generate(module, make_options(), output);

    ASSERT_TRUE(result.is_err())
        << "Generating an empty module must fail instead of producing a header";
    EXPECT_EQ(result.error(), asn1pp::error_code::parse_error);
    EXPECT_TRUE(output.str().empty())
        << "Empty module failure should not write partial output:\n" << output.str();
}

TEST(Generator, SingleIntegerType_UT_GEN_003) {
    generator gen;
    std::ostringstream output;

    auto module = make_module_with_type("UserId", make_integer());
    auto result = gen.generate(module, make_options(), output);

    ASSERT_TRUE(result.is_ok()) << "INTEGER assignment should be dispatched to the integer emitter";
    EXPECT_THAT(output.str(), HasSubstr("struct UserId"));
    EXPECT_THAT(output.str(), HasSubstr("int64_t value{}"));
    EXPECT_THAT(output.str(), HasSubstr("asn1_tag<generated::UserId>"));
}

TEST(Generator, SequenceOfMixedTypes_UT_GEN_004) {
    generator gen;
    std::ostringstream output;

    auto seq = make_sequence(components(
        make_component("id", make_integer()),
        make_component("enabled", make_boolean()),
        make_component("payload", make_octet_string())
    ));
    auto module = make_module_with_type("Record", std::move(seq));
    auto result = gen.generate(module, make_options(), output);

    ASSERT_TRUE(result.is_ok()) << "SEQUENCE assignment should be dispatched to the sequence emitter";
    EXPECT_THAT(output.str(), HasSubstr("struct Record"));
    EXPECT_THAT(output.str(), HasSubstr("int64_t id{}"));
    EXPECT_THAT(output.str(), HasSubstr("bool enabled{}"));
    EXPECT_THAT(output.str(), HasSubstr("std::vector<uint8_t> payload{}"));
}

TEST(Generator, UnknownTypeReturnsInvalidTag_UT_GEN_005) {
    generator gen;
    std::ostringstream output;

    auto module = make_module_with_type("Alias", make_named_type("MissingType"));
    auto result = gen.generate(module, make_options(), output);

    ASSERT_TRUE(result.is_err())
        << "Unregistered type_ref alternatives must fail deterministically";
    EXPECT_EQ(result.error(), asn1pp::error_code::invalid_tag);
}

TEST(Generator, OutputContainsHeaderGuard_UT_GEN_006) {
    generator gen;
    std::ostringstream output;

    auto module = make_module_with_type("Counter", make_integer());
    auto result = gen.generate(module, make_options(), output);

    ASSERT_TRUE(result.is_ok());
    EXPECT_THAT(output.str(), HasSubstr("#ifndef ASN1PP_TESTMODULE_HPP"));
    EXPECT_THAT(output.str(), HasSubstr("#define ASN1PP_TESTMODULE_HPP"));
    EXPECT_THAT(output.str(), ContainsRegex("#endif[[:space:]]*// ASN1PP_TESTMODULE_HPP"));
}

TEST(Generator, OutputContainsNamespaceDeclaration_UT_GEN_007) {
    generator gen;
    std::ostringstream output;

    auto module = make_module_with_type("Counter", make_integer());
    auto result = gen.generate(module, make_options(), output);

    ASSERT_TRUE(result.is_ok());
    EXPECT_THAT(output.str(), HasSubstr("namespace generated {"));
    EXPECT_THAT(output.str(), HasSubstr("}  // namespace generated"));
}

TEST(Generator, OutputIncludesAsn1ppHeaders_UT_GEN_008) {
    generator gen;
    std::ostringstream output;

    auto module = make_module_with_type("Counter", make_integer());
    auto result = gen.generate(module, make_options(), output);

    ASSERT_TRUE(result.is_ok());
    EXPECT_THAT(output.str(), HasSubstr("#include \"asn1pp/codec.hpp\""));
    EXPECT_THAT(output.str(), HasSubstr("#include \"asn1pp/traits.hpp\""));
}
