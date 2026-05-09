#include <gtest/gtest.h>
#include <cstdint>
#include <string>

#include "codec/traits.hpp"
#include "../../src/gen/ast.hpp"

namespace asn1pp::codec {

TEST(TagMappingNativeTest, Int64MapsToInteger) {
    constexpr tag expected = make_universal(universal_tag::integer);
    static_assert(tag_for_type_v<int64_t> == expected);
    EXPECT_EQ(tag_for_type_v<int64_t>, expected);
}

TEST(TagMappingNativeTest, Uint64MapsToInteger) {
    constexpr tag expected = make_universal(universal_tag::integer);
    static_assert(tag_for_type_v<uint64_t> == expected);
    EXPECT_EQ(tag_for_type_v<uint64_t>, expected);
}

TEST(TagMappingNativeTest, BoolMapsToBoolean) {
    constexpr tag expected = make_universal(universal_tag::boolean);
    static_assert(tag_for_type_v<bool> == expected);
    EXPECT_EQ(tag_for_type_v<bool>, expected);
}

TEST(TagMappingNativeTest, StringMapsToOctetString) {
    constexpr tag expected = make_universal(universal_tag::octet_string);
    static_assert(tag_for_type_v<std::string> == expected);
    EXPECT_EQ(tag_for_type_v<std::string>, expected);
}

TEST(TagMappingAstTest, IntegerTypeMapsToInteger) {
    constexpr tag expected = make_universal(universal_tag::integer);
    static_assert(tag_for_type_v<gen::integer_type> == expected);
    EXPECT_EQ(tag_for_type_v<gen::integer_type>, expected);
}

TEST(TagMappingAstTest, BooleanTypeMapsToBoolean) {
    constexpr tag expected = make_universal(universal_tag::boolean);
    static_assert(tag_for_type_v<gen::boolean_type> == expected);
    EXPECT_EQ(tag_for_type_v<gen::boolean_type>, expected);
}

TEST(TagMappingAstTest, NullTypeMapsToNull) {
    constexpr tag expected = make_universal(universal_tag::null);
    static_assert(tag_for_type_v<gen::null_type> == expected);
    EXPECT_EQ(tag_for_type_v<gen::null_type>, expected);
}

TEST(TagMappingAstTest, OctetStringTypeMapsToOctetString) {
    constexpr tag expected = make_universal(universal_tag::octet_string);
    static_assert(tag_for_type_v<gen::octet_string_type> == expected);
    EXPECT_EQ(tag_for_type_v<gen::octet_string_type>, expected);
}

TEST(TagMappingAstTest, SequenceTypeMapsToSequenceConstructed) {
    constexpr tag expected = make_universal(universal_tag::sequence, true);
    static_assert(tag_for_type_v<gen::sequence_type> == expected);
    EXPECT_EQ(tag_for_type_v<gen::sequence_type>, expected);
}

TEST(TagMappingAstTest, SetTypeMapsToSetConstructed) {
    constexpr tag expected = make_universal(universal_tag::set, true);
    static_assert(tag_for_type_v<gen::set_type> == expected);
    EXPECT_EQ(tag_for_type_v<gen::set_type>, expected);
}

TEST(TagMappingAstTest, EnumeratedTypeMapsToEnumerated) {
    constexpr tag expected = make_universal(universal_tag::enumerated);
    static_assert(tag_for_type_v<gen::enumerated_type> == expected);
    EXPECT_EQ(tag_for_type_v<gen::enumerated_type>, expected);
}

TEST(TagMappingAstTest, BitStringTypeMapsToBitString) {
    constexpr tag expected = make_universal(universal_tag::bit_string);
    static_assert(tag_for_type_v<gen::bit_string_type> == expected);
    EXPECT_EQ(tag_for_type_v<gen::bit_string_type>, expected);
}

TEST(TagMappingAstTest, RealTypeMapsToReal) {
    constexpr tag expected = make_universal(universal_tag::real);
    static_assert(tag_for_type_v<gen::real_type> == expected);
    EXPECT_EQ(tag_for_type_v<gen::real_type>, expected);
}

TEST(TagMappingAstTest, ObjectIdentifierTypeMapsToObjectIdentifier) {
    constexpr tag expected = make_universal(universal_tag::object_identifier);
    static_assert(tag_for_type_v<gen::object_identifier_type> == expected);
    EXPECT_EQ(tag_for_type_v<gen::object_identifier_type>, expected);
}

TEST(TagMappingAstTest, RelativeOidTypeMapsToRelativeOid) {
    constexpr tag expected = make_universal(universal_tag::relative_oid);
    static_assert(tag_for_type_v<gen::relative_oid_type> == expected);
    EXPECT_EQ(tag_for_type_v<gen::relative_oid_type>, expected);
}

TEST(IsConstructedTest, SequenceTypeIsConstructed) {
    static_assert(is_type_constructed_v<gen::sequence_type>);
    EXPECT_TRUE(is_type_constructed_v<gen::sequence_type>);
}

TEST(IsConstructedTest, SetTypeIsConstructed) {
    static_assert(is_type_constructed_v<gen::set_type>);
    EXPECT_TRUE(is_type_constructed_v<gen::set_type>);
}

TEST(IsConstructedTest, ChoiceTypeIsConstructed) {
    static_assert(is_type_constructed_v<gen::choice_type>);
    EXPECT_TRUE(is_type_constructed_v<gen::choice_type>);
}

TEST(IsConstructedTest, SequenceOfTypeIsConstructed) {
    static_assert(is_type_constructed_v<gen::sequence_of_type>);
    EXPECT_TRUE(is_type_constructed_v<gen::sequence_of_type>);
}

TEST(IsConstructedTest, SetOfTypeIsConstructed) {
    static_assert(is_type_constructed_v<gen::set_of_type>);
    EXPECT_TRUE(is_type_constructed_v<gen::set_of_type>);
}

TEST(IsConstructedTest, IntegerTypeIsNotConstructed) {
    static_assert(!is_type_constructed_v<gen::integer_type>);
    EXPECT_FALSE(is_type_constructed_v<gen::integer_type>);
}

TEST(IsConstructedTest, BooleanTypeIsNotConstructed) {
    static_assert(!is_type_constructed_v<gen::boolean_type>);
    EXPECT_FALSE(is_type_constructed_v<gen::boolean_type>);
}

TEST(IsConstructedTest, NullTypeIsNotConstructed) {
    static_assert(!is_type_constructed_v<gen::null_type>);
    EXPECT_FALSE(is_type_constructed_v<gen::null_type>);
}

TEST(IsPrimitiveTest, IntegerTypeIsPrimitive) {
    static_assert(is_type_primitive_v<gen::integer_type>);
    EXPECT_TRUE(is_type_primitive_v<gen::integer_type>);
}

TEST(IsPrimitiveTest, BooleanTypeIsPrimitive) {
    static_assert(is_type_primitive_v<gen::boolean_type>);
    EXPECT_TRUE(is_type_primitive_v<gen::boolean_type>);
}

TEST(IsPrimitiveTest, NullTypeIsPrimitive) {
    static_assert(is_type_primitive_v<gen::null_type>);
    EXPECT_TRUE(is_type_primitive_v<gen::null_type>);
}

TEST(IsPrimitiveTest, RealTypeIsPrimitive) {
    static_assert(is_type_primitive_v<gen::real_type>);
    EXPECT_TRUE(is_type_primitive_v<gen::real_type>);
}

TEST(IsPrimitiveTest, OctetStringTypeIsPrimitive) {
    static_assert(is_type_primitive_v<gen::octet_string_type>);
    EXPECT_TRUE(is_type_primitive_v<gen::octet_string_type>);
}

TEST(IsPrimitiveTest, ObjectIdentifierTypeIsPrimitive) {
    static_assert(is_type_primitive_v<gen::object_identifier_type>);
    EXPECT_TRUE(is_type_primitive_v<gen::object_identifier_type>);
}

TEST(IsPrimitiveTest, RelativeOidTypeIsPrimitive) {
    static_assert(is_type_primitive_v<gen::relative_oid_type>);
    EXPECT_TRUE(is_type_primitive_v<gen::relative_oid_type>);
}

TEST(IsPrimitiveTest, SequenceTypeIsNotPrimitive) {
    static_assert(!is_type_primitive_v<gen::sequence_type>);
    EXPECT_FALSE(is_type_primitive_v<gen::sequence_type>);
}

TEST(IsPrimitiveTest, SetTypeIsNotPrimitive) {
    static_assert(!is_type_primitive_v<gen::set_type>);
    EXPECT_FALSE(is_type_primitive_v<gen::set_type>);
}

TEST(IsPrimitiveTest, ChoiceTypeIsNotPrimitive) {
    static_assert(!is_type_primitive_v<gen::choice_type>);
    EXPECT_FALSE(is_type_primitive_v<gen::choice_type>);
}

}  // namespace asn1pp::codec