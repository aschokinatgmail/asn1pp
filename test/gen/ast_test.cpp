#include <gtest/gtest.h>
#include "../../src/gen/ast.hpp"
#include "../../src/gen/ast_print.hpp"

namespace {

using namespace asn1pp::gen;

template<typename T>
std::unique_ptr<type_ref> make_type(T val) {
    auto tr = std::make_unique<type_ref>();
    tr->content = std::move(val);
    return tr;
}

std::unique_ptr<type_ref> make_int_ref() {
    auto tr = std::make_unique<type_ref>();
    tr->content = integer_type{};
    return tr;
}

std::unique_ptr<type_ref> make_bool_ref() {
    auto tr = std::make_unique<type_ref>();
    tr->content = boolean_type{};
    return tr;
}

std::unique_ptr<type_ref> make_octet_ref() {
    auto tr = std::make_unique<type_ref>();
    tr->content = octet_string_type{};
    return tr;
}

TEST(AstIntegerType, Construction) {
    type_ref ref;
    ref.content = integer_type{};
    EXPECT_TRUE(ref.holds_alternative<integer_type>());
    EXPECT_EQ(to_string(ref), "INTEGER");
}

TEST(AstBooleanType, Construction) {
    type_ref ref;
    ref.content = boolean_type{};
    EXPECT_TRUE(ref.holds_alternative<boolean_type>());
    EXPECT_EQ(to_string(ref), "BOOLEAN");
}

TEST(AstNullType, Construction) {
    type_ref ref;
    ref.content = null_type{};
    EXPECT_TRUE(ref.holds_alternative<null_type>());
    EXPECT_EQ(to_string(ref), "NULL");
}

TEST(AstRealType, Construction) {
    type_ref ref;
    ref.content = real_type{};
    EXPECT_TRUE(ref.holds_alternative<real_type>());
    EXPECT_EQ(to_string(ref), "REAL");
}

TEST(AstOctetStringType, Construction) {
    type_ref ref;
    ref.content = octet_string_type{};
    EXPECT_TRUE(ref.holds_alternative<octet_string_type>());
    EXPECT_EQ(to_string(ref), "OCTET STRING");
}

TEST(AstObjectIdentifierType, Construction) {
    type_ref ref;
    ref.content = object_identifier_type{};
    EXPECT_TRUE(ref.holds_alternative<object_identifier_type>());
    EXPECT_EQ(to_string(ref), "OBJECT IDENTIFIER");
}

TEST(AstRelativeOidType, Construction) {
    type_ref ref;
    ref.content = relative_oid_type{};
    EXPECT_TRUE(ref.holds_alternative<relative_oid_type>());
    EXPECT_EQ(to_string(ref), "RELATIVE-OID");
}

TEST(AstAnyType, Construction) {
    type_ref ref;
    ref.content = any_type{};
    EXPECT_TRUE(ref.holds_alternative<any_type>());
    EXPECT_EQ(to_string(ref), "ANY");
}

TEST(AstTypeRef, TypeNameReference) {
    type_ref ref;
    ref.content = std::string{"MyType"};
    EXPECT_TRUE(ref.holds_alternative<std::string>());
    EXPECT_EQ(ref.get<std::string>(), "MyType");
    EXPECT_EQ(to_string(ref), "MyType");
}

TEST(AstEnumeratedType, Construction) {
    enumerated_type t;
    t.values = {
        {"zero", 0},
        {"one", 1},
        {"two", std::nullopt}
    };
    EXPECT_EQ(t.values.size(), 3);
    EXPECT_FALSE(t.has_extension);
}

TEST(AstEnumeratedType, WithExtension) {
    enumerated_type t;
    t.values = {{"a", 0}, {"b", 1}};
    t.has_extension = true;
    EXPECT_TRUE(t.has_extension);
}

TEST(AstEnumeratedType, Print) {
    enumerated_type t;
    t.values = {{"a", 0}, {"b", 1}};
    std::string output = to_string(t);
    EXPECT_TRUE(output.find("ENUMERATED") != std::string::npos);
    EXPECT_TRUE(output.find("a") != std::string::npos);
    EXPECT_TRUE(output.find("b") != std::string::npos);
}

TEST(AstBitStringType, Construction) {
    bit_string_type t;
    t.named_bits = {{"bit0", 0}, {"bit1", std::nullopt}};
    EXPECT_EQ(t.named_bits.size(), 2);
    EXPECT_FALSE(t.has_extension);
}

TEST(AstBitStringType, WithExtension) {
    bit_string_type t;
    t.named_bits = {{"a", 0}};
    t.has_extension = true;
    EXPECT_TRUE(t.has_extension);
}

TEST(AstBitStringType, Print) {
    bit_string_type t;
    t.named_bits = {{"bit0", 0}};
    std::string output = to_string(t);
    EXPECT_TRUE(output.find("BIT STRING") != std::string::npos);
    EXPECT_TRUE(output.find("bit0") != std::string::npos);
}

TEST(AstSequenceType, Construction) {
    sequence_type t;
    component_type c1;
    c1.name = "field1";
    c1.type = make_int_ref();
    t.components.push_back(std::move(c1));
    EXPECT_EQ(t.components.size(), 1);
    EXPECT_FALSE(t.has_extension);
}

TEST(AstSequenceType, WithExtension) {
    sequence_type t;
    t.has_extension = true;
    EXPECT_TRUE(t.has_extension);
}

TEST(AstSequenceType, Print) {
    sequence_type t;
    component_type c1;
    c1.name = "field1";
    c1.type = make_int_ref();
    t.components.push_back(std::move(c1));
    std::string output = to_string(t);
    EXPECT_TRUE(output.find("SEQUENCE") != std::string::npos);
    EXPECT_TRUE(output.find("field1") != std::string::npos);
}

TEST(AstSetType, Construction) {
    set_type t;
    component_type c1;
    c1.name = "field1";
    c1.type = make_int_ref();
    t.components.push_back(std::move(c1));
    EXPECT_EQ(t.components.size(), 1);
    EXPECT_FALSE(t.has_extension);
}

TEST(AstChoiceType, Construction) {
    choice_type t;
    choice_alternative a1;
    a1.name = "alt1";
    a1.type = make_int_ref();
    t.alternatives.push_back(std::move(a1));
    EXPECT_EQ(t.alternatives.size(), 1);
    EXPECT_FALSE(t.has_extension);
}

TEST(AstChoiceType, WithExtension) {
    choice_type t;
    t.has_extension = true;
    EXPECT_TRUE(t.has_extension);
}

TEST(AstChoiceType, Print) {
    choice_type t;
    choice_alternative a1;
    a1.name = "alt1";
    a1.type = make_int_ref();
    t.alternatives.push_back(std::move(a1));
    std::string output = to_string(t);
    EXPECT_TRUE(output.find("CHOICE") != std::string::npos);
    EXPECT_TRUE(output.find("alt1") != std::string::npos);
}

TEST(AstSequenceOfType, Construction) {
    sequence_of_type t;
    t.element_type = make_int_ref();
    EXPECT_TRUE(t.element_type->holds_alternative<integer_type>());
}

TEST(AstSequenceOfType, Print) {
    sequence_of_type t;
    t.element_type = make_int_ref();
    std::string output = to_string(t);
    EXPECT_TRUE(output.find("SEQUENCE OF") != std::string::npos);
    EXPECT_TRUE(output.find("INTEGER") != std::string::npos);
}

TEST(AstSetOfType, Construction) {
    set_of_type t;
    t.element_type = make_int_ref();
    EXPECT_TRUE(t.element_type->holds_alternative<integer_type>());
}

TEST(AstSetOfType, Print) {
    set_of_type t;
    t.element_type = make_int_ref();
    std::string output = to_string(t);
    EXPECT_TRUE(output.find("SET OF") != std::string::npos);
    EXPECT_TRUE(output.find("INTEGER") != std::string::npos);
}

TEST(AstTaggedType, Construction) {
    tagged_type t;
    t.tag_value = asn1pp::make_context_specific(0, false);
    t.implicit = true;
    t.underlying_type = make_int_ref();
    EXPECT_TRUE(t.underlying_type->holds_alternative<integer_type>());
    EXPECT_TRUE(t.implicit);
}

TEST(AstTaggedType, Print) {
    tagged_type t;
    t.tag_value = asn1pp::make_context_specific(0, false);
    t.implicit = true;
    t.underlying_type = make_int_ref();
    std::string output = to_string(t);
    EXPECT_TRUE(output.find("context-specific") != std::string::npos);
    EXPECT_TRUE(output.find("IMPLICIT") != std::string::npos);
}

TEST(AstConstrainedType, Construction) {
    constrained_type t;
    t.underlying_type = make_int_ref();
    t.constraints.push_back(constraint{value_range_constraint{0, 100, true, true}});
    EXPECT_TRUE(t.underlying_type->holds_alternative<integer_type>());
    EXPECT_EQ(t.constraints.size(), 1);
}

TEST(AstConstrainedType, Print) {
    constrained_type t;
    t.underlying_type = make_int_ref();
    t.constraints.push_back(constraint{value_range_constraint{0, 100, true, true}});
    std::string output = to_string(t);
    EXPECT_TRUE(output.find("INTEGER") != std::string::npos);
    EXPECT_TRUE(output.find("VALUE_RANGE") != std::string::npos);
}

TEST(AstSelectionType, Construction) {
    selection_type t;
    t.selected_type = make_int_ref();
    t.field_name = "field1";
    EXPECT_TRUE(t.selected_type->holds_alternative<integer_type>());
    EXPECT_EQ(t.field_name, "field1");
}

TEST(AstSelectionType, Print) {
    selection_type t;
    t.selected_type = make_int_ref();
    t.field_name = "field1";
    std::string output = to_string(t);
    EXPECT_TRUE(output.find("SELECTION") != std::string::npos);
    EXPECT_TRUE(output.find("field1") != std::string::npos);
}

TEST(AstConstraint, ValueRangeConstraint) {
    constraint c{value_range_constraint{1, 10, true, false}};
    EXPECT_TRUE(std::holds_alternative<value_range_constraint>(c.content));
}

TEST(AstConstraint, SizeConstraint) {
    constraint c{size_constraint{1, 100}};
    EXPECT_TRUE(std::holds_alternative<size_constraint>(c.content));
}

TEST(AstConstraint, PermittedAlphabetConstraint) {
    constraint c{permitted_alphabet_constraint{"A-Z"}};
    EXPECT_TRUE(std::holds_alternative<permitted_alphabet_constraint>(c.content));
}

TEST(AstConstraint, ExtensionConstraint) {
    constraint c{extension_constraint{}};
    EXPECT_TRUE(std::holds_alternative<extension_constraint>(c.content));
}

TEST(AstConstraint, Print) {
    constraint c{value_range_constraint{1, 10, true, false}};
    std::string output = to_string(c);
    EXPECT_TRUE(output.find("VALUE_RANGE") != std::string::npos);
}

TEST(AstValueRef, Int64) {
    value_ref v;
    v.content = int64_t{42};
    EXPECT_TRUE(std::holds_alternative<int64_t>(v.content));
    EXPECT_EQ(std::get<int64_t>(v.content), 42);
}

TEST(AstValueRef, Bool) {
    value_ref v;
    v.content = true;
    EXPECT_TRUE(std::holds_alternative<bool>(v.content));
    EXPECT_TRUE(std::get<bool>(v.content));
}

TEST(AstValueRef, String) {
    value_ref v;
    v.content = std::string{"hello"};
    EXPECT_TRUE(std::holds_alternative<std::string>(v.content));
    EXPECT_EQ(std::get<std::string>(v.content), "hello");
}

TEST(AstValueRef, NullValue) {
    value_ref v;
    v.content = null_value{};
    EXPECT_TRUE(std::holds_alternative<null_value>(v.content));
}

TEST(AstValueRef, OctetString) {
    value_ref v;
    v.content = std::vector<uint8_t>{0xDE, 0xAD, 0xBE, 0xEF};
    EXPECT_TRUE(std::holds_alternative<std::vector<uint8_t>>(v.content));
}

TEST(AstAssignment, TypeAssignment) {
    type_assignment ta;
    ta.name = "MyInt";
    ta.type = make_int_ref();
    assignment a;
    a.content = std::move(ta);
    EXPECT_TRUE(std::holds_alternative<type_assignment>(a.content));
}

TEST(AstAssignment, ValueAssignment) {
    value_assignment va;
    va.name = "myValue";
    va.type = make_int_ref();
    va.value.content = int64_t{100};
    assignment a;
    a.content = std::move(va);
    EXPECT_TRUE(std::holds_alternative<value_assignment>(a.content));
}

TEST(AstModuleDefinition, Construction) {
    module_definition mod;
    mod.name = "TestModule";
    mod.module_oid = "1.2.3.4.5";
    mod.default_tagging = tag_default::automatic_tag;
    mod.extensibility_implied = false;
    EXPECT_EQ(mod.name, "TestModule");
    EXPECT_EQ(mod.module_oid, "1.2.3.4.5");
    EXPECT_EQ(mod.default_tagging, tag_default::automatic_tag);
    EXPECT_FALSE(mod.extensibility_implied);
}

TEST(AstModuleDefinition, WithAssignments) {
    module_definition mod;
    mod.name = "TestModule";
    mod.module_oid = "1.2.3.4.5";

    type_assignment ta;
    ta.name = "MyInt";
    ta.type = make_int_ref();
    assignment a;
    a.content = std::move(ta);
    mod.assignments.push_back(std::move(a));

    value_assignment va;
    va.name = "myValue";
    va.type = make_int_ref();
    va.value.content = int64_t{100};
    assignment a2;
    a2.content = std::move(va);
    mod.assignments.push_back(std::move(a2));

    EXPECT_EQ(mod.assignments.size(), 2);
}

TEST(AstModuleDefinition, Print) {
    module_definition mod;
    mod.name = "TestModule";
    mod.module_oid = "1.2.3.4.5";
    mod.default_tagging = tag_default::automatic_tag;

    type_assignment ta;
    ta.name = "MyInt";
    ta.type = make_int_ref();
    assignment a;
    a.content = std::move(ta);
    mod.assignments.push_back(std::move(a));

    std::string output = to_string(mod);
    EXPECT_TRUE(output.find("MODULE TestModule") != std::string::npos);
    EXPECT_TRUE(output.find("OID 1.2.3.4.5") != std::string::npos);
    EXPECT_TRUE(output.find("AUTOMATIC TAGS") != std::string::npos);
    EXPECT_TRUE(output.find("TYPE-ASSIGNMENT MyInt") != std::string::npos);
}

TEST(AstTagDefault, AllValues) {
    EXPECT_TRUE(tag_default::explicit_tag == tag_default::explicit_tag);
    EXPECT_TRUE(tag_default::implicit_tag == tag_default::implicit_tag);
    EXPECT_TRUE(tag_default::automatic_tag == tag_default::automatic_tag);
}

TEST(AstSourceLocation, Construction) {
    source_location loc;
    loc.file = "test.asn";
    loc.line = 10;
    loc.column = 5;
    EXPECT_EQ(loc.file, "test.asn");
    EXPECT_EQ(loc.line, 10);
    EXPECT_EQ(loc.column, 5);
}

TEST(AstComponentType, Optional) {
    component_type comp;
    comp.name = "optionalField";
    comp.type = make_bool_ref();
    comp.optional = true;
    EXPECT_TRUE(comp.optional);
    EXPECT_FALSE(comp.default_value.has_value());
}

TEST(AstComponentType, DefaultValue) {
    component_type comp;
    comp.name = "fieldWithDefault";
    comp.type = make_int_ref();
    comp.default_value = "defaultValue";
    EXPECT_TRUE(comp.default_value.has_value());
    EXPECT_EQ(comp.default_value.value(), "defaultValue");
}

TEST(AstComponentType, Print) {
    component_type comp;
    comp.name = "field1";
    comp.type = make_int_ref();
    std::string output = to_string(comp);
    EXPECT_TRUE(output.find("field1") != std::string::npos);
    EXPECT_TRUE(output.find("INTEGER") != std::string::npos);
}

TEST(AstComponentType, PrintOptional) {
    component_type comp;
    comp.name = "optionalField";
    comp.type = make_bool_ref();
    comp.optional = true;
    std::string output = to_string(comp);
    EXPECT_TRUE(output.find("OPTIONAL") != std::string::npos);
}

TEST(AstConstrainedType, MultipleConstraints) {
    constrained_type ct;
    ct.underlying_type = make_octet_ref();
    ct.constraints.push_back(constraint{size_constraint{1, 100}});
    ct.constraints.push_back(constraint{permitted_alphabet_constraint{"A-Z"}});
    std::string output = to_string(ct);
    EXPECT_TRUE(output.find("SIZE") != std::string::npos);
    EXPECT_TRUE(output.find("PERMITTED_ALPHABET") != std::string::npos);
}

}  // namespace
