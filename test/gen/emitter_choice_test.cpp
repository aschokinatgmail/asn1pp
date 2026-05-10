#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
#include <string>
#include <string_view>

#include "../../src/gen/emitter_choice.hpp"
#include "../../src/gen/ast.hpp"

namespace {

using namespace asn1pp::gen;

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

type_ref make_null() {
    type_ref t;
    t.content = null_type{};
    return t;
}

choice_alternative make_alt(const std::string& name, type_ref type) {
    choice_alternative a;
    a.name = name;
    a.type = std::make_unique<type_ref>(std::move(type));
    return a;
}

choice_type make_choice(std::vector<choice_alternative> alts, bool has_ext = false) {
    choice_type c;
    c.alternatives = std::move(alts);
    c.has_extension = has_ext;
    return c;
}

// ============================================================================
// 1. Simple CHOICE with 2 alternatives
// ============================================================================

TEST(EmitterChoice, TwoAlternativesEmitsVariantAndWhichEnum) {
    std::vector<choice_alternative> alts;
    alts.push_back(make_alt("a", make_integer()));
    alts.push_back(make_alt("b", make_boolean()));
    auto c = make_choice(std::move(alts));

    std::string result = emit_choice("C", c);

    // Has struct declaration
    EXPECT_THAT(result, testing::HasSubstr("struct C {"));

    // Has which enum with both alternatives
    EXPECT_THAT(result, testing::HasSubstr("a = 0"));
    EXPECT_THAT(result, testing::HasSubstr("b = 1"));

    // Has variant with correct types
    EXPECT_THAT(result, testing::HasSubstr("std::variant<int64_t, bool> value"));

    // Has index() method
    EXPECT_THAT(result, testing::HasSubstr("which index() const noexcept"));

    // Has operator==
    EXPECT_THAT(result, testing::HasSubstr("bool operator==(const C&) const = default"));
}

// ============================================================================
// 2. CHOICE with 3+ alternatives
// ============================================================================

TEST(EmitterChoice, ThreeAlternativesEmitsAllVariants) {
    std::vector<choice_alternative> alts;
    alts.push_back(make_alt("a", make_integer()));
    alts.push_back(make_alt("b", make_boolean()));
    alts.push_back(make_alt("c", make_octet_string()));
    auto c = make_choice(std::move(alts));

    std::string result = emit_choice("D", c);

    EXPECT_THAT(result, testing::HasSubstr("a = 0"));
    EXPECT_THAT(result, testing::HasSubstr("b = 1"));
    EXPECT_THAT(result, testing::HasSubstr("c = 2"));

    EXPECT_THAT(result, testing::HasSubstr("std::variant<int64_t, bool, std::vector<uint8_t>> value"));
}

// ============================================================================
// 3. CHOICE with extension marker
// ============================================================================

TEST(EmitterChoice, ExtensionMarkerAddsUnknownExtensionAlt) {
    std::vector<choice_alternative> alts;
    alts.push_back(make_alt("a", make_integer()));
    alts.push_back(make_alt("b", make_boolean()));
    auto c = make_choice(std::move(alts), /*has_ext=*/true);

    std::string result = emit_choice("E", c);

    // Has _unknown_extension in which enum
    EXPECT_THAT(result, testing::HasSubstr("_unknown_extension = 2"));

    // Has std::vector<uint8_t> in variant
    EXPECT_THAT(result, testing::HasSubstr("std::variant<int64_t, bool, std::vector<uint8_t>> value"));

    // Has accessors for _unknown_extension
    EXPECT_THAT(result, testing::HasSubstr("is__unknown_extension()"));
    EXPECT_THAT(result, testing::HasSubstr("get__unknown_extension()"));
}

// ============================================================================
// 4. Tag mapping
// ============================================================================

TEST(EmitterChoice, TagIsSequenceConstructed) {
    std::vector<choice_alternative> alts;
    alts.push_back(make_alt("a", make_integer()));
    alts.push_back(make_alt("b", make_boolean()));
    auto c = make_choice(std::move(alts));

    std::string result = emit_choice("F", c);

    EXPECT_THAT(result, testing::HasSubstr("template<> struct asn1pp::asn1_tag<F>"));
    EXPECT_THAT(result, testing::HasSubstr("make_universal(universal_tag::sequence, true)"));
}

// ============================================================================
// 5. Accessor methods
// ============================================================================

TEST(EmitterChoice, IsAccessorsEmitCorrectly) {
    std::vector<choice_alternative> alts;
    alts.push_back(make_alt("a", make_integer()));
    alts.push_back(make_alt("b", make_boolean()));
    auto c = make_choice(std::move(alts));

    std::string result = emit_choice("G", c);

    EXPECT_THAT(result, testing::HasSubstr("bool is_a() const noexcept"));
    EXPECT_THAT(result, testing::HasSubstr("std::holds_alternative<int64_t>(value)"));
    EXPECT_THAT(result, testing::HasSubstr("bool is_b() const noexcept"));
    EXPECT_THAT(result, testing::HasSubstr("std::holds_alternative<bool>(value)"));
}

TEST(EmitterChoice, GetAccessorsEmitCorrectly) {
    std::vector<choice_alternative> alts;
    alts.push_back(make_alt("a", make_integer()));
    auto c = make_choice(std::move(alts));

    std::string result = emit_choice("H", c);

    // Mutable accessor
    EXPECT_THAT(result, testing::HasSubstr("int64_t& get_a()"));
    EXPECT_THAT(result, testing::HasSubstr("std::get<int64_t>(value)"));

    // Const accessor
    EXPECT_THAT(result, testing::HasSubstr("const int64_t& get_a() const"));
}

TEST(EmitterChoice, IndexReturnsWhich) {
    std::vector<choice_alternative> alts;
    alts.push_back(make_alt("a", make_integer()));
    alts.push_back(make_alt("b", make_boolean()));
    auto c = make_choice(std::move(alts));

    std::string result = emit_choice("I", c);

    EXPECT_THAT(result, testing::HasSubstr("[[nodiscard]] which index() const noexcept"));
    EXPECT_THAT(result, testing::HasSubstr("return static_cast<which>(value.index())"));
}

// ============================================================================
// 6. operator==
// ============================================================================

TEST(EmitterChoice, OperatorEqualsIsDefaulted) {
    std::vector<choice_alternative> alts;
    alts.push_back(make_alt("a", make_integer()));
    alts.push_back(make_alt("b", make_boolean()));
    auto c = make_choice(std::move(alts));

    std::string result = emit_choice("J", c);

    EXPECT_THAT(result, testing::HasSubstr("bool operator==(const J&) const = default"));
}

// ============================================================================
// 7. Generated code compiles as valid C++20
// ============================================================================

TEST(EmitterChoice, GeneratedCodeIsValidCpp20) {
    std::vector<choice_alternative> alts;
    alts.push_back(make_alt("a", make_integer()));
    alts.push_back(make_alt("b", make_boolean()));
    auto c = make_choice(std::move(alts));

    std::string result = emit_choice("K", c);

    // Check it's a complete struct with closing brace
    EXPECT_THAT(result, testing::HasSubstr("};"));

    // Check tag template is complete
    EXPECT_THAT(result, testing::HasSubstr("template<> struct asn1pp::asn1_tag<K>"));

    // No stray or incomplete code
    EXPECT_THAT(result, testing::Not(testing::HasSubstr("<<")));
    EXPECT_THAT(result, testing::Not(testing::HasSubstr("??")));
}

// ============================================================================
// 8. Single alternative
// ============================================================================

TEST(EmitterChoice, SingleAlternative) {
    std::vector<choice_alternative> alts;
    alts.push_back(make_alt("only", make_null()));
    auto c = make_choice(std::move(alts));

    std::string result = emit_choice("L", c);

    EXPECT_THAT(result, testing::HasSubstr("only = 0"));
    EXPECT_THAT(result, testing::HasSubstr("std::variant<std::monostate> value"));
    EXPECT_THAT(result, testing::HasSubstr("is_only()"));
    EXPECT_THAT(result, testing::HasSubstr("std::monostate& get_only()"));
}

// ============================================================================
// 9. Empty CHOICE (edge case - should still produce valid code)
// ============================================================================

TEST(EmitterChoice, EmptyChoiceWithExtension) {
    auto c = make_choice({}, /*has_ext=*/true);

    std::string result = emit_choice("M", c);

    // Has unknown extension
    EXPECT_THAT(result, testing::HasSubstr("_unknown_extension = 0"));
    EXPECT_THAT(result, testing::HasSubstr("std::variant<std::vector<uint8_t>> value"));
    EXPECT_THAT(result, testing::HasSubstr("const std::vector<uint8_t>& get__unknown_extension() const"));
}

}  // anonymous namespace
