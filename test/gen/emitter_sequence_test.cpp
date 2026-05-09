#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "../../src/gen/emitter_sequence.hpp"
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

type_ref make_null() {
    type_ref t;
    t.content = null_type{};
    return t;
}

type_ref make_real() {
    type_ref t;
    t.content = real_type{};
    return t;
}

type_ref make_octet_string() {
    type_ref t;
    t.content = octet_string_type{};
    return t;
}

type_ref make_oid() {
    type_ref t;
    t.content = object_identifier_type{};
    return t;
}

type_ref make_enumerated(std::vector<enumeration_item> values) {
    type_ref t;
    enumerated_type e;
    e.values = std::move(values);
    t.content = std::move(e);
    return t;
}

type_ref make_bit_string() {
    type_ref t;
    t.content = bit_string_type{};
    return t;
}

type_ref make_type_ref_name(const std::string& name) {
    type_ref t;
    t.content = name;
    return t;
}

type_ref make_sequence_of(type_ref element_type) {
    type_ref t;
    auto so = std::make_unique<sequence_of_type>();
    so->element_type = std::make_unique<type_ref>(std::move(element_type));
    t.content = std::move(so);
    return t;
}

type_ref make_nested_sequence(std::vector<component_type> components) {
    type_ref t;
    auto seq = std::make_unique<sequence_type>();
    seq->components = std::move(components);
    t.content = std::move(seq);
    return t;
}

component_type make_component(const std::string& name, type_ref type,
                               bool optional = false,
                               std::optional<std::string> default_val = std::nullopt) {
    component_type c;
    c.name = name;
    c.type = std::make_unique<type_ref>(std::move(type));
    c.optional = optional;
    c.default_value = std::move(default_val);
    return c;
}

sequence_type make_seq(std::vector<component_type> components, bool has_ext = false) {
    sequence_type s;
    s.components = std::move(components);
    s.has_extension = has_ext;
    return s;
}

set_type make_set(std::vector<component_type> components, bool has_ext = false) {
    set_type s;
    s.components = std::move(components);
    s.has_extension = has_ext;
    return s;
}

std::vector<component_type> comps() {
    return {};
}

template<typename... Args>
std::vector<component_type> comps(Args&&... args) {
    std::vector<component_type> v;
    v.reserve(sizeof...(Args));
    (v.push_back(std::forward<Args>(args)), ...);
    return v;
}

void expect_contains(const std::string& haystack, const std::string& needle) {
    EXPECT_NE(haystack.find(needle), std::string::npos)
        << "Expected to find '" << needle << "' in output:\n" << haystack;
}

void expect_not_contains(const std::string& haystack, const std::string& needle) {
    EXPECT_EQ(haystack.find(needle), std::string::npos)
        << "Expected NOT to find '" << needle << "' in output:\n" << haystack;
}

}  // namespace

// ============================================================
// 1. Simple SEQUENCE with 2-3 fields
// ============================================================

TEST(EmitterSequence, SimpleSequenceWithThreeFields_UT_SEQUENCE_001) {
    auto seq = make_seq(comps(
        make_component("name", make_octet_string()),
        make_component("age", make_integer()),
        make_component("active", make_boolean())
    ));

    std::string result = emit_sequence("Person", seq);

    expect_contains(result, "struct Person {");
    expect_contains(result, "std::vector<uint8_t> name{}");
    expect_contains(result, "int64_t age{}");
    expect_contains(result, "bool active{}");
    expect_contains(result, "operator==(const Person&) const = default");
    expect_contains(result, "asn1_tag<Person>");
    expect_contains(result, "universal_tag::sequence");
}

TEST(EmitterSequence, SingleFieldSequence_UT_SEQUENCE_002) {
    auto seq = make_seq(comps(
        make_component("id", make_integer())
    ));

    std::string result = emit_sequence("Id", seq);

    expect_contains(result, "struct Id {");
    expect_contains(result, "int64_t id{}");
    expect_contains(result, "asn1pp::asn1_tag<Id>");
}

// ============================================================
// 2. SEQUENCE with OPTIONAL field
// ============================================================

TEST(EmitterSequence, OptionalFieldGeneratesOptional_UT_SEQUENCE_003) {
    auto seq = make_seq(comps(
        make_component("name", make_octet_string()),
        make_component("email", make_octet_string(), true)
    ));

    std::string result = emit_sequence("Person", seq);

    expect_contains(result, "std::vector<uint8_t> name{}");
    expect_contains(result, "std::optional<std::vector<uint8_t>> email;");
}

TEST(EmitterSequence, MultipleOptionalFields_UT_SEQUENCE_004) {
    auto seq = make_seq(comps(
        make_component("a", make_integer()),
        make_component("b", make_boolean(), true),
        make_component("c", make_real(), true)
    ));

    std::string result = emit_sequence("OptSeq", seq);

    expect_contains(result, "int64_t a{}");
    expect_contains(result, "std::optional<bool> b;");
    expect_contains(result, "std::optional<double> c;");
}

// ============================================================
// 3. SEQUENCE with DEFAULT field
// ============================================================

TEST(EmitterSequence, DefaultIntegerField_UT_SEQUENCE_005) {
    auto seq = make_seq(comps(
        make_component("id", make_integer()),
        make_component("count", make_integer(), false, std::string{"0"})
    ));

    std::string result = emit_sequence("Record", seq);

    expect_contains(result, "int64_t id{}");
    expect_contains(result, "int64_t count{0}");
}

TEST(EmitterSequence, DefaultBooleanField_UT_SEQUENCE_006) {
    auto seq = make_seq(comps(
        make_component("flag", make_boolean(), false, std::string{"TRUE"})
    ));

    std::string result = emit_sequence("FlagSeq", seq);

    expect_contains(result, "bool flag{true}");
}

TEST(EmitterSequence, DefaultStringField_UT_SEQUENCE_007) {
    auto seq = make_seq(comps(
        make_component("data", make_octet_string(), false, std::string{"0x48656C6C6F"})
    ));

    std::string result = emit_sequence("Msg", seq);

    expect_contains(result, "std::vector<uint8_t>");
    expect_contains(result, "{0x48656C6C6F}");
}

TEST(EmitterSequence, DefaultWithNumericValue_UT_SEQUENCE_008) {
    auto seq = make_seq(comps(
        make_component("size", make_integer(), false, std::string{"42"})
    ));

    std::string result = emit_sequence("Sized", seq);

    expect_contains(result, "int64_t size{42}");
}

// ============================================================
// 4. Nested SEQUENCE type
// ============================================================

TEST(EmitterSequence, NestedSequence_UT_SEQUENCE_009) {
    auto inner_comps = comps(
        make_component("x", make_integer()),
        make_component("y", make_integer())
    );

    auto seq = make_seq(comps(
        make_component("position", make_nested_sequence(std::move(inner_comps))),
        make_component("label", make_octet_string())
    ));

    std::string result = emit_sequence("Widget", seq);

    expect_contains(result, "struct Position");
    expect_contains(result, "struct Widget {");

    expect_contains(result, "Position position;");
    expect_contains(result, "std::vector<uint8_t> label{}");
    expect_contains(result, "asn1_tag<Widget>");
}

TEST(EmitterSequence, DeeplyNestedSequence_UT_SEQUENCE_010) {
    auto inner_comps = comps(
        make_component("value", make_integer())
    );

    auto seq = make_seq(comps(
        make_component("data", make_nested_sequence(std::move(inner_comps)))
    ));

    std::string result = emit_sequence("Outer", seq);

    expect_contains(result, "struct Data");
    expect_contains(result, "struct Outer");
    expect_contains(result, "Data data;");
}

// ============================================================
// 5. SET type
// ============================================================

TEST(EmitterSequence, SetTypeHasSetTag_UT_SET_001) {
    auto s = make_set(comps(
        make_component("id", make_integer()),
        make_component("name", make_octet_string())
    ));

    std::string result = emit_set("Record", s);

    expect_contains(result, "struct Record {");
    expect_contains(result, "int64_t id{}");
    expect_contains(result, "universal_tag::set");
    expect_contains(result, "asn1_tag<Record>");
}

TEST(EmitterSequence, SetTypeNoSequenceTag_UT_SET_002) {
    auto s = make_set(comps(
        make_component("value", make_integer())
    ));

    std::string result = emit_set("SimpleSet", s);

    expect_contains(result, "universal_tag::set");
    expect_not_contains(result, "universal_tag::sequence");
}

TEST(EmitterSequence, SetWithOptionalAndDefault_UT_SET_003) {
    auto s = make_set(comps(
        make_component("required", make_integer()),
        make_component("optional_field", make_octet_string(), true),
        make_component("with_default", make_integer(), false, std::string{"100"})
    ));

    std::string result = emit_set("Mixed", s);

    expect_contains(result, "int64_t required{}");
    expect_contains(result, "std::optional<std::vector<uint8_t>> optional_field;");
    expect_contains(result, "int64_t with_default{100}");
    expect_contains(result, "universal_tag::set");
}

// ============================================================
// 6. Extension marker in SEQUENCE
// ============================================================

TEST(EmitterSequence, ExtensionMarkerAddsExtensionData_UT_SEQUENCE_011) {
    auto seq = make_seq(comps(
        make_component("id", make_integer())
    ), true);

    std::string result = emit_sequence("ExtSeq", seq);

    expect_contains(result, "std::vector<uint8_t> _extension_data;");
    expect_contains(result, "int64_t id{}");
}

TEST(EmitterSequence, ExtensionMarkerInSet_UT_SET_004) {
    auto s = make_set(comps(
        make_component("value", make_integer())
    ), true);

    std::string result = emit_set("ExtSet", s);

    expect_contains(result, "std::vector<uint8_t> _extension_data;");
    expect_contains(result, "universal_tag::set");
}

TEST(EmitterSequence, NoExtensionWhenFalse_UT_SEQUENCE_012) {
    auto seq = make_seq(comps(
        make_component("id", make_integer())
    ), false);

    std::string result = emit_sequence("NoExt", seq);

    expect_not_contains(result, "_extension_data");
}

// ============================================================
// 7. AUTOMATIC TAGS (tag specialization always generated)
// ============================================================

TEST(EmitterSequence, TagIsConstructed_UT_SEQUENCE_013) {
    auto seq = make_seq(comps(
        make_component("x", make_integer())
    ));

    std::string result = emit_sequence("Tagged", seq);

    expect_contains(result, "make_universal(asn1pp::universal_tag::sequence, true)");
}

// ============================================================
// 8. Field type mapping: all primitive ASN.1 types
// ============================================================

TEST(EmitterSequence, IntegerFieldMapsToInt64T_UT_SEQUENCE_014) {
    auto seq = make_seq(comps(
        make_component("count", make_integer())
    ));

    std::string result = emit_sequence("IntSeq", seq);
    expect_contains(result, "int64_t count{}");
}

TEST(EmitterSequence, BooleanFieldMapsToBool_UT_SEQUENCE_015) {
    auto seq = make_seq(comps(
        make_component("flag", make_boolean())
    ));

    std::string result = emit_sequence("BoolSeq", seq);
    expect_contains(result, "bool flag{}");
}

TEST(EmitterSequence, NullFieldMapsToMonostate_UT_SEQUENCE_016) {
    auto seq = make_seq(comps(
        make_component("nothing", make_null())
    ));

    std::string result = emit_sequence("NullSeq", seq);
    expect_contains(result, "std::monostate nothing;");
}

TEST(EmitterSequence, RealFieldMapsToDouble_UT_SEQUENCE_017) {
    auto seq = make_seq(comps(
        make_component("ratio", make_real())
    ));

    std::string result = emit_sequence("RealSeq", seq);
    expect_contains(result, "double ratio{}");
}

TEST(EmitterSequence, OctetStringMapsToVectorUint8_UT_SEQUENCE_018) {
    auto seq = make_seq(comps(
        make_component("data", make_octet_string())
    ));

    std::string result = emit_sequence("OcSeq", seq);
    expect_contains(result, "std::vector<uint8_t> data{}");
}

TEST(EmitterSequence, OidMapsToVectorUint32_UT_SEQUENCE_019) {
    auto seq = make_seq(comps(
        make_component("oid", make_oid())
    ));

    std::string result = emit_sequence("OidSeq", seq);
    expect_contains(result, "std::vector<uint32_t> oid{}");
}

TEST(EmitterSequence, EnumeratedBecomesNestedType_UT_SEQUENCE_020) {
    auto seq = make_seq(comps(
        make_component("color", make_enumerated({{"red", 0}, {"green", 1}}))
    ));

    std::string result = emit_sequence("EnumSeq", seq);

    expect_contains(result, "struct Color_enum");
    expect_contains(result, "Color_enum color;");
}

TEST(EmitterSequence, BitStringMapsToPair_UT_SEQUENCE_021) {
    auto seq = make_seq(comps(
        make_component("bits", make_bit_string())
    ));

    std::string result = emit_sequence("BitSeq", seq);
    expect_contains(result, "std::pair<std::vector<uint8_t>, size_t> bits{}");
}

TEST(EmitterSequence, TypeRefNamePassedThrough_UT_SEQUENCE_022) {
    auto seq = make_seq(comps(
        make_component("ref", make_type_ref_name("MyCustomType"))
    ));

    std::string result = emit_sequence("RefSeq", seq);
    expect_contains(result, "MyCustomType ref;");
}

TEST(EmitterSequence, SequenceOfMapsToVector_UT_SEQUENCE_023) {
    auto seq = make_seq(comps(
        make_component("items", make_sequence_of(make_integer()))
    ));

    std::string result = emit_sequence("ListSeq", seq);
    expect_contains(result, "std::vector<int64_t> items{}");
}

TEST(EmitterSequence, SequenceOfNestedType_UT_SEQUENCE_024) {
    auto seq = make_seq(comps(
        make_component("codes", make_sequence_of(make_type_ref_name("ErrorCode")))
    ));

    std::string result = emit_sequence("CodeList", seq);
    expect_contains(result, "std::vector<ErrorCode> codes{}");
}

// ============================================================
// 9. Generated code compiles as valid C++20
// ============================================================

TEST(EmitterSequence, GeneratesValidCppStruct_UT_SEQUENCE_025) {
    auto seq = make_seq(comps(
        make_component("id", make_integer()),
        make_component("name", make_octet_string()),
        make_component("flag", make_boolean()),
        make_component("ratio", make_real()),
        make_component("optional_field", make_octet_string(), true),
        make_component("count", make_integer(), false, std::string{"0"})
    ));

    std::string code = emit_sequence("ValidSeq", seq);

    expect_contains(code, "struct ValidSeq {");
    expect_contains(code, "int64_t id{}");
    expect_contains(code, "std::vector<uint8_t> name{}");
    expect_contains(code, "bool flag{}");
    expect_contains(code, "double ratio{}");
    expect_contains(code, "std::optional<std::vector<uint8_t>> optional_field;");
    expect_contains(code, "int64_t count{0}");
    expect_contains(code, "operator==(const ValidSeq&) const = default");
    expect_contains(code, "};");
    expect_contains(code, "};\n");

    expect_not_contains(code, "new ");
    expect_not_contains(code, "delete ");
    expect_not_contains(code, "malloc");
    expect_not_contains(code, "free");
    expect_not_contains(code, "virtual");
    expect_contains(code, "std::vector");
    expect_contains(code, "std::optional");
}

// ============================================================
// 10. operator== is present
// ============================================================

TEST(EmitterSequence, OperatorEqualsIsDefaulted_UT_SEQUENCE_026) {
    auto seq = make_seq(comps(
        make_component("x", make_integer())
    ));

    std::string result = emit_sequence("Equals", seq);
    expect_contains(result, "bool operator==(const Equals&) const = default;");
}

// ============================================================
// Edge cases
// ============================================================

TEST(EmitterSequence, EmptySequence_UT_SEQUENCE_027) {
    auto seq = make_seq(comps());

    std::string result = emit_sequence("Empty", seq);

    expect_contains(result, "struct Empty {");
    expect_contains(result, "operator==(const Empty&) const = default");
    expect_contains(result, "universal_tag::sequence");
}

TEST(EmitterSequence, SpecialCharacterFieldName_UT_SEQUENCE_028) {
    auto seq = make_seq(comps(
        make_component("hyphen-field", make_integer())
    ));

    std::string result = emit_sequence("Hyphen", seq);

    expect_contains(result, "int64_t hyphen_field{}");
    expect_not_contains(result, "hyphen-field");
}

TEST(EmitterSequence, MixedFieldTypes_UT_SEQUENCE_029) {
    auto seq = make_seq(comps(
        make_component("a", make_integer()),
        make_component("b", make_octet_string()),
        make_component("c", make_boolean()),
        make_component("d", make_real()),
        make_component("e", make_null()),
        make_component("f", make_bit_string()),
        make_component("g", make_oid())
    ));

    std::string result = emit_sequence("AllTypes", seq);

    expect_contains(result, "int64_t a{}");
    expect_contains(result, "std::vector<uint8_t> b{}");
    expect_contains(result, "bool c{}");
    expect_contains(result, "double d{}");
    expect_contains(result, "std::monostate e;");
    expect_contains(result, "std::pair<std::vector<uint8_t>, size_t> f{}");
    expect_contains(result, "std::vector<uint32_t> g{}");
}

TEST(EmitterSequence, OptionalWithDefaultIsIgnored_UT_SEQUENCE_030) {
    auto seq = make_seq(comps(
        make_component("name", make_octet_string()),
        make_component("email", make_octet_string(), true, std::string{"none"})
    ));

    std::string result = emit_sequence("OptDef", seq);

    expect_contains(result, "std::optional<std::vector<uint8_t>> email;");
}
