#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "../../src/gen/emitter_per_meta.hpp"
#include "../../src/gen/ast.hpp"

namespace {

using namespace asn1pp::gen;

type_ref make_integer_type() {
    type_ref t;
    t.content = integer_type{};
    return t;
}

type_ref make_octet_string_type() {
    type_ref t;
    t.content = octet_string_type{};
    return t;
}

type_ref make_enumerated_type(std::vector<enumeration_item> values, bool has_ext = false) {
    type_ref t;
    enumerated_type e;
    e.values = std::move(values);
    e.has_extension = has_ext;
    t.content = std::move(e);
    return t;
}

type_ref make_sequence_type(std::vector<component_type> components, bool has_ext = false) {
    type_ref t;
    auto seq = std::make_unique<sequence_type>();
    seq->components = std::move(components);
    seq->has_extension = has_ext;
    t.content = std::move(seq);
    return t;
}

type_ref make_choice_type(std::vector<choice_alternative> alternatives, bool has_ext = false) {
    type_ref t;
    auto ch = std::make_unique<choice_type>();
    ch->alternatives = std::move(alternatives);
    ch->has_extension = has_ext;
    t.content = std::move(ch);
    return t;
}

component_type make_component(const std::string& name, type_ref type, bool optional = false) {
    component_type c;
    c.name = name;
    c.type = std::make_unique<type_ref>(std::move(type));
    c.optional = optional;
    return c;
}

choice_alternative make_alternative(const std::string& name, type_ref type) {
    choice_alternative a;
    a.name = name;
    a.type = std::make_unique<type_ref>(std::move(type));
    return a;
}

type_ref make_constrained(type_ref underlying, std::vector<constraint> constraints) {
    type_ref t;
    auto ct = std::make_unique<constrained_type>();
    ct->underlying_type = std::make_unique<type_ref>(std::move(underlying));
    ct->constraints = std::move(constraints);
    t.content = std::move(ct);
    return t;
}

constraint make_value_range(std::optional<int64_t> min_val, std::optional<int64_t> max_val,
                             bool min_inclusive = true, bool max_inclusive = true) {
    constraint c;
    value_range_constraint vrc;
    vrc.min_value = min_val;
    vrc.max_value = max_val;
    vrc.min_inclusive = min_inclusive;
    vrc.max_inclusive = max_inclusive;
    c.content = std::move(vrc);
    return c;
}

constraint make_size_constraint(std::optional<size_t> min_sz, std::optional<size_t> max_sz) {
    constraint c;
    size_constraint sc;
    sc.min_size = min_sz;
    sc.max_size = max_sz;
    c.content = std::move(sc);
    return c;
}

template<typename... Args>
std::vector<component_type> comps(Args&&... args) {
    std::vector<component_type> v;
    v.reserve(sizeof...(Args));
    (v.push_back(std::forward<Args>(args)), ...);
    return v;
}

template<typename... Args>
std::vector<choice_alternative> alts(Args&&... args) {
    std::vector<choice_alternative> v;
    v.reserve(sizeof...(Args));
    (v.push_back(std::forward<Args>(args)), ...);
    return v;
}

// ============================================================================
// 1. INTEGER with range (0..255)
// ============================================================================

TEST(EmitterPerMeta, IntegerRange0to255) {
    auto constrained = make_constrained(
        make_integer_type(),
        {make_value_range(0, 255)});

    std::string result = emit_per_meta(constrained, "X");
    EXPECT_NE(result.find("static constexpr int64_t min_value"), std::string::npos);
    EXPECT_NE(result.find("= 0"), std::string::npos);
    EXPECT_NE(result.find("static constexpr int64_t max_value"), std::string::npos);
    EXPECT_NE(result.find("= 255"), std::string::npos);
    EXPECT_NE(result.find("static constexpr bool has_range_constraint"), std::string::npos);
    EXPECT_NE(result.find("= true"), std::string::npos);
    EXPECT_NE(result.find("static constexpr bool is_extension_permitted"), std::string::npos);
}

TEST(EmitterPerMeta, IntegerRange0to255ExclusiveMax) {
    auto constrained = make_constrained(
        make_integer_type(),
        {make_value_range(0, 255, true, false)});

    std::string result = emit_per_meta(constrained, "X");
    EXPECT_NE(result.find("min_value"), std::string::npos);
    EXPECT_NE(result.find("max_value"), std::string::npos);
    EXPECT_NE(result.find("= 0"), std::string::npos);
    EXPECT_NE(result.find("= 255"), std::string::npos);
}

// ============================================================================
// 2. INTEGER with range (MIN..MAX) -- unbounded
// ============================================================================

TEST(EmitterPerMeta, IntegerRangeUnbounded) {
    auto constrained = make_constrained(
        make_integer_type(),
        {make_value_range(std::nullopt, std::nullopt)});

    std::string result = emit_per_meta(constrained, "X");
    EXPECT_NE(result.find("has_range_constraint"), std::string::npos);
}

TEST(EmitterPerMeta, IntegerRangeMinOnly) {
    auto constrained = make_constrained(
        make_integer_type(),
        {make_value_range(0, std::nullopt)});

    std::string result = emit_per_meta(constrained, "X");
    EXPECT_NE(result.find("min_value"), std::string::npos);
    EXPECT_NE(result.find("= 0"), std::string::npos);
    EXPECT_NE(result.find("max_value"), std::string::npos);
    EXPECT_NE(result.find("static constexpr bool has_range_constraint = true"), std::string::npos);
}

TEST(EmitterPerMeta, IntegerRangeMaxOnly) {
    auto constrained = make_constrained(
        make_integer_type(),
        {make_value_range(std::nullopt, 1000)});

    std::string result = emit_per_meta(constrained, "X");
    EXPECT_NE(result.find("min_value"), std::string::npos);
    EXPECT_NE(result.find("max_value"), std::string::npos);
    EXPECT_NE(result.find("= 1000"), std::string::npos);
    EXPECT_NE(result.find("has_range_constraint = true"), std::string::npos);
}

// ============================================================================
// 3. OCTET STRING with SIZE(1..32)
// ============================================================================

TEST(EmitterPerMeta, OctetStringSize1to32) {
    auto constrained = make_constrained(
        make_octet_string_type(),
        {make_size_constraint(1, 32)});

    std::string result = emit_per_meta(constrained, "Y");
    EXPECT_NE(result.find("static constexpr size_t min_size"), std::string::npos);
    EXPECT_NE(result.find("= 1"), std::string::npos);
    EXPECT_NE(result.find("static constexpr size_t max_size"), std::string::npos);
    EXPECT_NE(result.find("= 32"), std::string::npos);
    EXPECT_NE(result.find("static constexpr bool has_size_constraint"), std::string::npos);
    EXPECT_NE(result.find("= true"), std::string::npos);
}

TEST(EmitterPerMeta, OctetStringSizeMinOnly) {
    auto constrained = make_constrained(
        make_octet_string_type(),
        {make_size_constraint(1, std::nullopt)});

    std::string result = emit_per_meta(constrained, "Y");
    EXPECT_NE(result.find("min_size"), std::string::npos);
    EXPECT_NE(result.find("= 1"), std::string::npos);
    EXPECT_NE(result.find("max_size"), std::string::npos);
    EXPECT_NE(result.find("has_size_constraint = true"), std::string::npos);
}

TEST(EmitterPerMeta, OctetStringSizeMaxOnly) {
    auto constrained = make_constrained(
        make_octet_string_type(),
        {make_size_constraint(std::nullopt, 255)});

    std::string result = emit_per_meta(constrained, "Y");
    EXPECT_NE(result.find("min_size"), std::string::npos);
    EXPECT_NE(result.find("= 0"), std::string::npos);
    EXPECT_NE(result.find("max_size"), std::string::npos);
    EXPECT_NE(result.find("= 255"), std::string::npos);
    EXPECT_NE(result.find("has_size_constraint = true"), std::string::npos);
}

// ============================================================================
// 4. ENUMERATED with values
// ============================================================================

TEST(EmitterPerMeta, EnumeratedNormalIndexCount) {
    auto et = make_enumerated_type({
        {"red", 0},
        {"green", 1},
        {"blue", 2}
    });

    std::string result = emit_per_meta(et, "E");
    EXPECT_NE(result.find("static constexpr size_t normal_index_count"), std::string::npos);
    EXPECT_NE(result.find("= 3"), std::string::npos);
    EXPECT_NE(result.find("static constexpr bool has_extension"), std::string::npos);
    EXPECT_NE(result.find("= false"), std::string::npos);
}

TEST(EmitterPerMeta, EnumeratedWithExtension) {
    auto et = make_enumerated_type({
        {"red", 0},
        {"green", 1},
        {"blue", 2}
    }, true);

    std::string result = emit_per_meta(et, "E");
    EXPECT_NE(result.find("normal_index_count"), std::string::npos);
    EXPECT_NE(result.find("= 3"), std::string::npos);
    EXPECT_NE(result.find("has_extension = true"), std::string::npos);
}

TEST(EmitterPerMeta, EnumeratedSingleValue) {
    auto et = make_enumerated_type({{"only", 0}});

    std::string result = emit_per_meta(et, "E");
    EXPECT_NE(result.find("normal_index_count = 1"), std::string::npos);
}

// ============================================================================
// 5. SEQUENCE with OPTIONAL fields -- optional bitmap
// ============================================================================

TEST(EmitterPerMeta, SequenceOptionalBitmap) {
    auto seq = make_sequence_type(comps(
        make_component("a", make_integer_type(), false),
        make_component("b", make_integer_type(), true),
        make_component("c", make_integer_type(), true)
    ));

    std::string result = emit_per_meta(seq, "S");
    EXPECT_NE(result.find("static constexpr size_t field_count"), std::string::npos);
    EXPECT_NE(result.find("= 3"), std::string::npos);
    EXPECT_NE(result.find("static constexpr bool optional_bitmap[3]"), std::string::npos);
    EXPECT_NE(result.find("= {false, true, true}"), std::string::npos);
}

TEST(EmitterPerMeta, SequenceNoOptionalFields) {
    auto seq = make_sequence_type(comps(
        make_component("a", make_integer_type(), false),
        make_component("b", make_integer_type(), false)
    ));

    std::string result = emit_per_meta(seq, "S");
    EXPECT_NE(result.find("field_count = 2"), std::string::npos);
    EXPECT_NE(result.find("optional_bitmap[2] = {false, false}"), std::string::npos);
}

TEST(EmitterPerMeta, SequenceAllOptional) {
    auto seq = make_sequence_type(comps(
        make_component("a", make_integer_type(), true),
        make_component("b", make_integer_type(), true)
    ));

    std::string result = emit_per_meta(seq, "S");
    EXPECT_NE(result.find("optional_bitmap[2] = {true, true}"), std::string::npos);
}

TEST(EmitterPerMeta, SequenceEmpty) {
    auto seq = make_sequence_type(comps());

    std::string result = emit_per_meta(seq, "S");
    EXPECT_NE(result.find("field_count = 0"), std::string::npos);
    EXPECT_NE(result.find("optional_bitmap[0]"), std::string::npos);
}

// ============================================================================
// 6. SEQUENCE/CHOICE with extension marker
// ============================================================================

TEST(EmitterPerMeta, SequenceWithExtension) {
    auto seq = make_sequence_type(comps(
        make_component("a", make_integer_type())
    ), true);

    std::string result = emit_per_meta(seq, "S");
    EXPECT_NE(result.find("has_extension = true"), std::string::npos);
    EXPECT_NE(result.find("field_count = 1"), std::string::npos);
}

TEST(EmitterPerMeta, SequenceWithoutExtension) {
    auto seq = make_sequence_type(comps(
        make_component("a", make_integer_type())
    ), false);

    std::string result = emit_per_meta(seq, "S");
    EXPECT_NE(result.find("has_extension = false"), std::string::npos);
}

TEST(EmitterPerMeta, ChoiceWithExtension) {
    auto ch = make_choice_type(alts(
        make_alternative("a", make_integer_type()),
        make_alternative("b", make_integer_type())
    ), true);

    std::string result = emit_per_meta(ch, "C");
    EXPECT_NE(result.find("static constexpr size_t alternative_count"), std::string::npos);
    EXPECT_NE(result.find("= 2"), std::string::npos);
    EXPECT_NE(result.find("has_extension = true"), std::string::npos);
}

TEST(EmitterPerMeta, ChoiceWithoutExtension) {
    auto ch = make_choice_type(alts(
        make_alternative("a", make_integer_type()),
        make_alternative("b", make_integer_type())
    ), false);

    std::string result = emit_per_meta(ch, "C");
    EXPECT_NE(result.find("alternative_count = 2"), std::string::npos);
    EXPECT_NE(result.find("has_extension = false"), std::string::npos);
}

// ============================================================================
// 7. Unconstrained type -- no constraint metadata or defaults
// ============================================================================

TEST(EmitterPerMeta, UnconstrainedInteger) {
    auto t = make_integer_type();
    std::string result = emit_per_meta(t, "X");
    EXPECT_NE(result.find("X_per_meta"), std::string::npos);
    EXPECT_NE(result.find("has_range_constraint = false"), std::string::npos);
    EXPECT_NE(result.find("is_extension_permitted = false"), std::string::npos);
}

TEST(EmitterPerMeta, UnconstrainedOctetString) {
    auto t = make_octet_string_type();
    std::string result = emit_per_meta(t, "Y");
    EXPECT_NE(result.find("has_size_constraint = false"), std::string::npos);
}

TEST(EmitterPerMeta, StructNamePattern) {
    auto seq = make_sequence_type(comps(
        make_component("field1", make_integer_type())
    ));

    std::string result = emit_per_meta(seq, "MyType");
    EXPECT_NE(result.find("struct MyType_per_meta"), std::string::npos);
}

}  // anonymous namespace
