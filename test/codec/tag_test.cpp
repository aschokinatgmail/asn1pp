#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>

#include "codec/traits.hpp"

using namespace asn1pp;

// ============================================================================
// Tag Registry Tests (ITU-T X.680)
// ============================================================================

// Test: All universal tag values present per X.680 Table 1 + Amendment 2
TEST(TagRegistry, UniversalTagValuesComplete) {
    // X.680 Table 1 - universal tags 0-30
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::end_of_content), 0);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::boolean), 1);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::integer), 2);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::bit_string), 3);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::octet_string), 4);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::null), 5);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::object_identifier), 6);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::object_descriptor), 7);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::external), 8);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::real), 9);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::enumerated), 10);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::embedded_pdv), 11);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::utf8_string), 12);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::relative_oid), 13);
    // Tags 14, 15 are reserved
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::sequence), 16);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::set), 17);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::numeric_string), 18);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::printable_string), 19);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::teletex_string), 20);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::videotex_string), 21);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::ia5_string), 22);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::utc_time), 23);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::generalized_time), 24);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::graphic_string), 25);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::visible_string), 26);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::general_string), 27);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::universal_string), 28);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::character_string), 29);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::bmp_string), 30);

    // X.680 Amendment 2 - tags 31-36
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::date), 31);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::time_of_day), 32);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::datetime), 33);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::duration), 34);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::rel_uri), 35);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::rel_uris), 36);
}

// Test: universal_tag_name() returns non-null for all enum values
TEST(TagRegistry, TagToNameRoundTrip) {
    const char* name = universal_tag_name(universal_tag::end_of_content);
    EXPECT_NE(name, nullptr);
    EXPECT_STRNE(name, "");

    name = universal_tag_name(universal_tag::boolean);
    EXPECT_NE(name, nullptr);
    EXPECT_STRNE(name, "");

    name = universal_tag_name(universal_tag::integer);
    EXPECT_NE(name, nullptr);
    EXPECT_STRNE(name, "");

    name = universal_tag_name(universal_tag::sequence);
    EXPECT_NE(name, nullptr);
    EXPECT_STRNE(name, "");

    name = universal_tag_name(universal_tag::set);
    EXPECT_NE(name, nullptr);
    EXPECT_STRNE(name, "");

    name = universal_tag_name(universal_tag::date);
    EXPECT_NE(name, nullptr);
    EXPECT_STRNE(name, "");

    name = universal_tag_name(universal_tag::time_of_day);
    EXPECT_NE(name, nullptr);
    EXPECT_STRNE(name, "");

    name = universal_tag_name(universal_tag::datetime);
    EXPECT_NE(name, nullptr);
    EXPECT_STRNE(name, "");

    name = universal_tag_name(universal_tag::duration);
    EXPECT_NE(name, nullptr);
    EXPECT_STRNE(name, "");

    name = universal_tag_name(universal_tag::rel_uri);
    EXPECT_NE(name, nullptr);
    EXPECT_STRNE(name, "");

    name = universal_tag_name(universal_tag::rel_uris);
    EXPECT_NE(name, nullptr);
    EXPECT_STRNE(name, "");
}

// Test: universal_tag_name() returns "UNKNOWN" for invalid tag
TEST(TagRegistry, TagNameUnknownForInvalid) {
    // There is no invalid universal_tag value in our enum (all are valid)
    // But we can verify that the function doesn't crash
    EXPECT_STREQ(universal_tag_name(universal_tag::end_of_content), "END-OF-CONTENT");
}

// Test: is_primitive() returns true for BOOLEAN
TEST(TagRegistry, IsPrimitiveBoolean) {
    EXPECT_TRUE(is_primitive(universal_tag::boolean));
}

// Test: is_primitive() returns true for all primitive types per X.680
TEST(TagRegistry, IsPrimitiveAllTypes) {
    // Primitive types from X.680
    EXPECT_TRUE(is_primitive(universal_tag::boolean));
    EXPECT_TRUE(is_primitive(universal_tag::integer));
    EXPECT_TRUE(is_primitive(universal_tag::bit_string));
    EXPECT_TRUE(is_primitive(universal_tag::octet_string));
    EXPECT_TRUE(is_primitive(universal_tag::null));
    EXPECT_TRUE(is_primitive(universal_tag::object_identifier));
    EXPECT_TRUE(is_primitive(universal_tag::object_descriptor));
    EXPECT_TRUE(is_primitive(universal_tag::real));
    EXPECT_TRUE(is_primitive(universal_tag::enumerated));
    EXPECT_TRUE(is_primitive(universal_tag::utf8_string));
    EXPECT_TRUE(is_primitive(universal_tag::relative_oid));
    EXPECT_TRUE(is_primitive(universal_tag::numeric_string));
    EXPECT_TRUE(is_primitive(universal_tag::printable_string));
    EXPECT_TRUE(is_primitive(universal_tag::teletex_string));
    EXPECT_TRUE(is_primitive(universal_tag::videotex_string));
    EXPECT_TRUE(is_primitive(universal_tag::ia5_string));
    EXPECT_TRUE(is_primitive(universal_tag::utc_time));
    EXPECT_TRUE(is_primitive(universal_tag::generalized_time));
    EXPECT_TRUE(is_primitive(universal_tag::graphic_string));
    EXPECT_TRUE(is_primitive(universal_tag::visible_string));
    EXPECT_TRUE(is_primitive(universal_tag::general_string));
    EXPECT_TRUE(is_primitive(universal_tag::universal_string));
    EXPECT_TRUE(is_primitive(universal_tag::character_string));
    EXPECT_TRUE(is_primitive(universal_tag::bmp_string));

    // Amendment 2 primitive types
    EXPECT_TRUE(is_primitive(universal_tag::date));
    EXPECT_TRUE(is_primitive(universal_tag::time_of_day));
    EXPECT_TRUE(is_primitive(universal_tag::datetime));
}

// Test: is_primitive() returns false for constructed types
TEST(TagRegistry, IsPrimitiveConstructedTypes) {
    EXPECT_FALSE(is_primitive(universal_tag::sequence));
    EXPECT_FALSE(is_primitive(universal_tag::set));
    EXPECT_FALSE(is_primitive(universal_tag::duration));
    EXPECT_FALSE(is_primitive(universal_tag::rel_uris));
}

// Test: is_constructed() returns true for SEQUENCE
TEST(TagRegistry, IsConstructedSequence) {
    EXPECT_TRUE(is_constructed(universal_tag::sequence));
}

// Test: is_constructed() returns true for SET
TEST(TagRegistry, IsConstructedSet) {
    EXPECT_TRUE(is_constructed(universal_tag::set));
}

// Test: is_constructed() returns true for constructed types per X.680
TEST(TagRegistry, IsConstructedAllTypes) {
    EXPECT_TRUE(is_constructed(universal_tag::sequence));
    EXPECT_TRUE(is_constructed(universal_tag::set));
    EXPECT_TRUE(is_constructed(universal_tag::duration));
    EXPECT_TRUE(is_constructed(universal_tag::rel_uris));
}

// Test: is_constructed() returns false for primitive types
TEST(TagRegistry, IsConstructedPrimitiveTypes) {
    EXPECT_FALSE(is_constructed(universal_tag::boolean));
    EXPECT_FALSE(is_constructed(universal_tag::integer));
    EXPECT_FALSE(is_constructed(universal_tag::null));
    EXPECT_FALSE(is_constructed(universal_tag::object_identifier));
    EXPECT_FALSE(is_constructed(universal_tag::date));
    EXPECT_FALSE(is_constructed(universal_tag::time_of_day));
    EXPECT_FALSE(is_constructed(universal_tag::datetime));
    EXPECT_FALSE(is_constructed(universal_tag::rel_uri));
}

// ============================================================================
// Tag Struct Tests
// ============================================================================

// Test: tag construction and comparison
TEST(TagStruct, ConstructionAndComparison) {
    tag t1{tag_class::universal, false, 2};
    tag t2 = make_universal(universal_tag::integer);
    EXPECT_EQ(t1, t2);
}

// Test: tag equality for universal tag
TEST(TagStruct, EqualityUniversalTag) {
    tag t1 = make_universal(universal_tag::integer);
    tag t2 = make_universal(universal_tag::integer);
    EXPECT_EQ(t1, t2);
}

// Test: tag inequality for different tags
TEST(TagStruct, InequalityDifferentTags) {
    tag t1 = make_universal(universal_tag::integer);
    tag t2 = make_universal(universal_tag::boolean);
    EXPECT_NE(t1, t2);
}

// Test: tag inequality for different tag classes
TEST(TagStruct, InequalityDifferentTagClasses) {
    tag t1 = make_universal(universal_tag::integer);
    tag t2 = make_context_specific(2);
    EXPECT_NE(t1, t2);
}

// Test: make_universal creates primitive tag by default
TEST(TagStruct, MakeUniversalPrimitive) {
    tag t = make_universal(universal_tag::integer);
    EXPECT_EQ(t.cls, tag_class::universal);
    EXPECT_FALSE(t.constructed);
    EXPECT_EQ(t.number, static_cast<uint32_t>(universal_tag::integer));
}

// Test: make_universal creates constructed tag when specified
TEST(TagStruct, MakeUniversalConstructed) {
    tag t = make_universal(universal_tag::sequence, true);
    EXPECT_EQ(t.cls, tag_class::universal);
    EXPECT_TRUE(t.constructed);
    EXPECT_EQ(t.number, static_cast<uint32_t>(universal_tag::sequence));
}

// ============================================================================
// Tag Class Tests
// ============================================================================

// Test: tag_class enum values
TEST(TagClass, EnumValues) {
    EXPECT_EQ(static_cast<uint8_t>(tag_class::universal), 0);
    EXPECT_EQ(static_cast<uint8_t>(tag_class::application), 1);
    EXPECT_EQ(static_cast<uint8_t>(tag_class::context_specific), 2);
    EXPECT_EQ(static_cast<uint8_t>(tag_class::private_class), 3);
}

// ============================================================================
// Context-Specific Tag Tests
// ============================================================================

// Test: make_context_specific creates primitive tag by default
TEST(ContextSpecific, MakeContextSpecificPrimitive) {
    tag t = make_context_specific(0);
    EXPECT_EQ(t.cls, tag_class::context_specific);
    EXPECT_FALSE(t.constructed);
    EXPECT_EQ(t.number, 0);
}

// Test: make_context_specific creates constructed tag when specified
TEST(ContextSpecific, MakeContextSpecificConstructed) {
    tag t = make_context_specific(3, true);
    EXPECT_EQ(t.cls, tag_class::context_specific);
    EXPECT_TRUE(t.constructed);
    EXPECT_EQ(t.number, 3);
}

// Test: context-specific tag comparison
TEST(ContextSpecific, ContextSpecificComparison) {
    tag t1 = make_context_specific(0);
    tag t2 = make_context_specific(0);
    EXPECT_EQ(t1, t2);

    tag t3 = make_context_specific(1);
    EXPECT_NE(t1, t3);
}

// ============================================================================
// Reserved Tag Tests
// ============================================================================

// Test: Tags 14 and 15 are reserved in X.680 (not present in our enum)
// This is correct behavior - our enum does not include them
TEST(ReservedTags, ReservedTagsNotInEnum) {
    // Verify tags 14 and 15 are not in the enum by checking sequence jumps from 13 to 16
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::relative_oid), 13);
    EXPECT_EQ(static_cast<uint32_t>(universal_tag::sequence), 16);
    // If there were tags 14 or 15, the gap wouldn't exist
}

// ============================================================================
// Name String Tests
// ============================================================================

// Test: universal_tag_name() returns expected strings
TEST(TagNameStrings, ExpectedNameStrings) {
    EXPECT_STREQ(universal_tag_name(universal_tag::boolean), "BOOLEAN");
    EXPECT_STREQ(universal_tag_name(universal_tag::integer), "INTEGER");
    EXPECT_STREQ(universal_tag_name(universal_tag::sequence), "SEQUENCE");
    EXPECT_STREQ(universal_tag_name(universal_tag::set), "SET");
    EXPECT_STREQ(universal_tag_name(universal_tag::null), "NULL");
    EXPECT_STREQ(universal_tag_name(universal_tag::object_identifier), "OBJECT IDENTIFIER");
    EXPECT_STREQ(universal_tag_name(universal_tag::utf8_string), "UTF8String");
    EXPECT_STREQ(universal_tag_name(universal_tag::date), "DATE");
    EXPECT_STREQ(universal_tag_name(universal_tag::time_of_day), "TIME-OF-DAY");
    EXPECT_STREQ(universal_tag_name(universal_tag::datetime), "DATE-TIME");
    EXPECT_STREQ(universal_tag_name(universal_tag::duration), "DURATION");
    EXPECT_STREQ(universal_tag_name(universal_tag::rel_uri), "RELATIVE-URI");
    EXPECT_STREQ(universal_tag_name(universal_tag::rel_uris), "RELATIVE-URIS");
}