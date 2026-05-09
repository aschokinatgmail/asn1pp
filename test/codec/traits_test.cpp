#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <concepts>

#include "codec/traits.hpp"

using namespace asn1pp;

// ============================================================================
// Mock types for concept testing
// ============================================================================

struct MockAsn1Type {};

struct MockEncoder {
    void encode(const MockAsn1Type&, buffer_view) {}
};

struct MockDecoder {
    void decode(buffer_view, MockAsn1Type&) {}
};

struct MockCodec {
    void encode(const MockAsn1Type&, buffer_view) {}
    void decode(buffer_view, MockAsn1Type&) {}
};

// An unrelated non-ASN.1 type
struct NotAsn1 {};

// A SEQUENCE-like type
struct MockSequence {};

// A CHOICE-like type
struct MockChoice {};

// A SET-like type
struct MockSet {};

// ============================================================================
// Trait specializations for mock types
// ============================================================================

// Specialize asn1_tag for mock types
template<>
struct asn1_tag<MockAsn1Type> {
    static constexpr auto value = universal_tag::integer;
    static constexpr bool is_specialized = true;
};

template<>
struct asn1_tag<MockSequence> {
    static constexpr auto value = universal_tag::sequence;
    static constexpr bool is_specialized = true;
};

template<>
struct asn1_tag<MockSet> {
    static constexpr auto value = universal_tag::set;
    static constexpr bool is_specialized = true;
};

// MockChoice has no asn1_tag specialization — CHOICE is not a universal tag
// Its type identity comes from is_choice trait

// Specialize type category traits
template<>
struct is_sequence<MockSequence> : std::true_type {};

template<>
struct is_choice<MockChoice> : std::true_type {};

template<>
struct is_set<MockSet> : std::true_type {};

// ============================================================================
// Static assertion tests: has_tag_v<T>
// ============================================================================

static_assert(has_tag_v<MockAsn1Type>,
              "MockAsn1Type (with asn1_tag specialization) must have_tag_v == true");
static_assert(has_tag_v<MockSequence>,
              "MockSequence (with asn1_tag specialization) must have_tag_v == true");
static_assert(!has_tag_v<NotAsn1>,
              "NotAsn1 (no asn1_tag specialization) must have_tag_v == false");
static_assert(!has_tag_v<int>,
              "int (built-in, no specialization) must have_tag_v == false");
static_assert(!has_tag_v<double>,
              "double (built-in, no specialization) must have_tag_v == false");

// ============================================================================
// Static assertion tests: encodable_v<T, Encoder>
// ============================================================================

static_assert(encodable_v<MockAsn1Type, MockEncoder>,
              "MockAsn1Type with MockEncoder.encode must be encodable_v == true");
static_assert(not encodable_v<int, MockEncoder>,
              "int must NOT be encodable_v with MockEncoder (no encode method)");
static_assert(not encodable_v<NotAsn1, MockEncoder>,
              "NotAsn1 must NOT be encodable_v with MockEncoder (no encode method)");

// ============================================================================
// Static assertion tests: decodable_v<T, Decoder>
// ============================================================================

static_assert(decodable_v<MockAsn1Type, MockDecoder>,
              "MockAsn1Type with MockDecoder.decode must be decodable_v == true");
static_assert(not decodable_v<int, MockDecoder>,
              "int must NOT be decodable_v with MockDecoder (no decode method)");
static_assert(not decodable_v<NotAsn1, MockDecoder>,
              "NotAsn1 must NOT be decodable_v with MockDecoder (no decode method)");

// ============================================================================
// Static assertion tests: codec_for_v<T, Codec>
// ============================================================================

static_assert(codec_for_v<MockAsn1Type, MockCodec>,
              "MockAsn1Type with MockCodec must be codec_for_v == true");
static_assert(codec_for_v<MockAsn1Type, MockEncoder>,
              "MockAsn1Type with MockEncoder (encoder_for) must satisfy codec_for_v == true");
static_assert(codec_for_v<MockAsn1Type, MockDecoder>,
              "MockAsn1Type with MockDecoder (decoder_for) must satisfy codec_for_v == true");
static_assert(not codec_for_v<int, MockCodec>,
              "int must NOT be codec_for_v with MockCodec");
static_assert(not codec_for_v<NotAsn1, MockCodec>,
              "NotAsn1 must NOT be codec_for_v with MockCodec");

// ============================================================================
// Static assertion tests: is_sequence_v, is_choice_v, is_set_v
// ============================================================================

static_assert(is_sequence_v<MockSequence>,
              "MockSequence must have is_sequence_v == true");
static_assert(!is_sequence_v<MockAsn1Type>,
              "MockAsn1Type (not a sequence) must have is_sequence_v == false");
static_assert(!is_sequence_v<int>,
              "int must have is_sequence_v == false");

static_assert(is_choice_v<MockChoice>,
              "MockChoice must have is_choice_v == true");
static_assert(!is_choice_v<MockAsn1Type>,
              "MockAsn1Type (not a choice) must have is_choice_v == false");
static_assert(!is_choice_v<int>,
              "int must have is_choice_v == false");

static_assert(is_set_v<MockSet>,
              "MockSet must have is_set_v == true");
static_assert(!is_set_v<MockAsn1Type>,
              "MockAsn1Type (not a set) must have is_set_v == false");
static_assert(!is_set_v<int>,
              "int must have is_set_v == false");

// ============================================================================
// Runtime tests: universal_tag enum values (X.680 Table 1)
// ============================================================================

TEST(TraitsTest, UniversalTagValues) {
    using UT = universal_tag;

    // Check a representative subset of X.680 universal class tag assignments
    EXPECT_EQ(static_cast<uint32_t>(UT::end_of_content),    0);
    EXPECT_EQ(static_cast<uint32_t>(UT::boolean),           1);
    EXPECT_EQ(static_cast<uint32_t>(UT::integer),           2);
    EXPECT_EQ(static_cast<uint32_t>(UT::bit_string),        3);
    EXPECT_EQ(static_cast<uint32_t>(UT::octet_string),      4);
    EXPECT_EQ(static_cast<uint32_t>(UT::null),              5);
    EXPECT_EQ(static_cast<uint32_t>(UT::object_identifier), 6);
    EXPECT_EQ(static_cast<uint32_t>(UT::object_descriptor), 7);
    EXPECT_EQ(static_cast<uint32_t>(UT::external),          8);
    EXPECT_EQ(static_cast<uint32_t>(UT::real),              9);
    EXPECT_EQ(static_cast<uint32_t>(UT::enumerated),       10);
    EXPECT_EQ(static_cast<uint32_t>(UT::embedded_pdv),     11);
    EXPECT_EQ(static_cast<uint32_t>(UT::utf8_string),      12);
    EXPECT_EQ(static_cast<uint32_t>(UT::relative_oid),     13);
    EXPECT_EQ(static_cast<uint32_t>(UT::sequence),         16);
    EXPECT_EQ(static_cast<uint32_t>(UT::set),              17);
    EXPECT_EQ(static_cast<uint32_t>(UT::numeric_string),   18);
    EXPECT_EQ(static_cast<uint32_t>(UT::printable_string), 19);
    EXPECT_EQ(static_cast<uint32_t>(UT::teletex_string),   20);
    EXPECT_EQ(static_cast<uint32_t>(UT::videotex_string),  21);
    EXPECT_EQ(static_cast<uint32_t>(UT::ia5_string),       22);
    EXPECT_EQ(static_cast<uint32_t>(UT::utc_time),         23);
    EXPECT_EQ(static_cast<uint32_t>(UT::generalized_time), 24);
    EXPECT_EQ(static_cast<uint32_t>(UT::graphic_string),   25);
    EXPECT_EQ(static_cast<uint32_t>(UT::visible_string),   26);
    EXPECT_EQ(static_cast<uint32_t>(UT::general_string),   27);
    EXPECT_EQ(static_cast<uint32_t>(UT::universal_string), 28);
    EXPECT_EQ(static_cast<uint32_t>(UT::character_string), 29);
    EXPECT_EQ(static_cast<uint32_t>(UT::bmp_string),       30);
}

// ============================================================================
// Runtime tests: tag struct construction and comparison
// ============================================================================

TEST(TraitsTest, TagConstruction) {
    tag t1{tag_class::universal, false, 2};
    EXPECT_EQ(t1.cls, tag_class::universal);
    EXPECT_FALSE(t1.constructed);
    EXPECT_EQ(t1.number, 2);

    tag t2{tag_class::context_specific, true, 42};
    EXPECT_EQ(t2.cls, tag_class::context_specific);
    EXPECT_TRUE(t2.constructed);
    EXPECT_EQ(t2.number, 42);

    // Defaulted operator==
    EXPECT_EQ(t1, t1);
    EXPECT_NE(t1, t2);
}

TEST(TraitsTest, MakeUniversalTag) {
    auto t = make_universal(universal_tag::integer);
    EXPECT_EQ(t.cls, tag_class::universal);
    EXPECT_EQ(t.number, 2);
    EXPECT_FALSE(t.constructed);

    auto t_seq = make_universal(universal_tag::sequence);
    EXPECT_EQ(t_seq.cls, tag_class::universal);
    EXPECT_EQ(t_seq.number, 16);
    EXPECT_FALSE(t_seq.constructed);

    auto t_seq_c = make_universal(universal_tag::sequence, true);
    EXPECT_TRUE(t_seq_c.constructed);
}

TEST(TraitsTest, MakeContextSpecificTag) {
    auto t = make_context_specific(5);
    EXPECT_EQ(t.cls, tag_class::context_specific);
    EXPECT_EQ(t.number, 5);
    EXPECT_FALSE(t.constructed);

    auto t_c = make_context_specific(10, true);
    EXPECT_EQ(t_c.cls, tag_class::context_specific);
    EXPECT_EQ(t_c.number, 10);
    EXPECT_TRUE(t_c.constructed);
}

// ============================================================================
// Runtime tests: is_primitive / is_constructed
// ============================================================================

TEST(TraitsTest, IsPrimitive) {
    // Primitive types per X.680
    EXPECT_TRUE(is_primitive(universal_tag::boolean));
    EXPECT_TRUE(is_primitive(universal_tag::integer));
    EXPECT_TRUE(is_primitive(universal_tag::bit_string));
    EXPECT_TRUE(is_primitive(universal_tag::octet_string));
    EXPECT_TRUE(is_primitive(universal_tag::null));
    EXPECT_TRUE(is_primitive(universal_tag::object_identifier));
    EXPECT_TRUE(is_primitive(universal_tag::real));
    EXPECT_TRUE(is_primitive(universal_tag::enumerated));
    EXPECT_TRUE(is_primitive(universal_tag::utf8_string));
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
    EXPECT_TRUE(is_primitive(universal_tag::object_descriptor));
}

TEST(TraitsTest, IsConstructed) {
    // Constructed types per X.680
    EXPECT_TRUE(is_constructed(universal_tag::sequence));
    EXPECT_TRUE(is_constructed(universal_tag::set));

    // These are NOT constructed
    EXPECT_FALSE(is_constructed(universal_tag::integer));
    EXPECT_FALSE(is_constructed(universal_tag::boolean));
    EXPECT_FALSE(is_constructed(universal_tag::null));
    EXPECT_FALSE(is_constructed(universal_tag::real));
    EXPECT_FALSE(is_constructed(universal_tag::enumerated));
}

// ============================================================================
// Runtime tests: universal_tag_name
// ============================================================================

TEST(TraitsTest, UniversalTagName) {
    EXPECT_STREQ(universal_tag_name(universal_tag::integer), "INTEGER");
    EXPECT_STREQ(universal_tag_name(universal_tag::boolean), "BOOLEAN");
    EXPECT_STREQ(universal_tag_name(universal_tag::sequence), "SEQUENCE");
    EXPECT_STREQ(universal_tag_name(universal_tag::set), "SET");
    EXPECT_STREQ(universal_tag_name(universal_tag::octet_string), "OCTET STRING");
    EXPECT_STREQ(universal_tag_name(universal_tag::null), "NULL");
    EXPECT_STREQ(universal_tag_name(universal_tag::object_identifier), "OBJECT IDENTIFIER");
    EXPECT_STREQ(universal_tag_name(universal_tag::bit_string), "BIT STRING");
    EXPECT_STREQ(universal_tag_name(universal_tag::real), "REAL");
    EXPECT_STREQ(universal_tag_name(universal_tag::enumerated), "ENUMERATED");
    EXPECT_STREQ(universal_tag_name(universal_tag::utf8_string), "UTF8String");
    EXPECT_STREQ(universal_tag_name(universal_tag::ia5_string), "IA5String");
}

// ============================================================================
// Runtime test: asn1_tag<T>::value match
// ============================================================================

TEST(TraitsTest, Asn1TagValueForMockInteger) {
    EXPECT_EQ(static_cast<uint32_t>(asn1_tag_v<MockAsn1Type>), 2);
    EXPECT_EQ(asn1_tag_v<MockAsn1Type>, universal_tag::integer);
}

TEST(TraitsTest, Asn1TagValueForSequence) {
    EXPECT_EQ(asn1_tag_v<MockSequence>, universal_tag::sequence);
}

TEST(TraitsTest, Asn1TagDefaultIsNull) {
    EXPECT_EQ(asn1_tag_v<int>, universal_tag::null);
    EXPECT_EQ(asn1_tag_v<NotAsn1>, universal_tag::null);
}

// ============================================================================
// Runtime tests: tag_class enumeration
// ============================================================================

TEST(TraitsTest, TagClassValues) {
    EXPECT_EQ(static_cast<uint8_t>(tag_class::universal),       0);
    EXPECT_EQ(static_cast<uint8_t>(tag_class::application),     1);
    EXPECT_EQ(static_cast<uint8_t>(tag_class::context_specific), 2);
    EXPECT_EQ(static_cast<uint8_t>(tag_class::private_class),   3);
}

// ============================================================================
// Runtime tests: has_tag_v
// ============================================================================

TEST(TraitsTest, HasTagV) {
    EXPECT_TRUE(has_tag_v<MockAsn1Type>);
    EXPECT_TRUE(has_tag_v<MockSequence>);
    EXPECT_FALSE(has_tag_v<NotAsn1>);
    EXPECT_FALSE(has_tag_v<int>);
}

// ============================================================================
// Runtime tests: encodable_v / decodable_v / codec_for_v
// ============================================================================

TEST(TraitsTest, EncodableV) {
    EXPECT_TRUE((encodable_v<MockAsn1Type, MockEncoder>));
    EXPECT_FALSE((encodable_v<int, MockEncoder>));
    EXPECT_FALSE((encodable_v<NotAsn1, MockEncoder>));
}

TEST(TraitsTest, DecodableV) {
    EXPECT_TRUE((decodable_v<MockAsn1Type, MockDecoder>));
    EXPECT_FALSE((decodable_v<int, MockDecoder>));
    EXPECT_FALSE((decodable_v<NotAsn1, MockDecoder>));
}

TEST(TraitsTest, CodecForV) {
    EXPECT_TRUE((codec_for_v<MockAsn1Type, MockCodec>));
    EXPECT_TRUE((codec_for_v<MockAsn1Type, MockEncoder>));
    EXPECT_TRUE((codec_for_v<MockAsn1Type, MockDecoder>));
    EXPECT_FALSE((codec_for_v<int, MockCodec>));
    EXPECT_FALSE((codec_for_v<NotAsn1, MockCodec>));
}

// ============================================================================
// Additional safety: non-null universal_tag_name for all enum values
// ============================================================================

TEST(TraitsTest, UniversalTagNameNotNullForAllTags) {
    // Every universal tag should have a human-readable name
    using UT = universal_tag;
    EXPECT_NE(universal_tag_name(UT::end_of_content), nullptr);
    EXPECT_NE(universal_tag_name(UT::boolean), nullptr);
    EXPECT_NE(universal_tag_name(UT::integer), nullptr);
    EXPECT_NE(universal_tag_name(UT::bit_string), nullptr);
    EXPECT_NE(universal_tag_name(UT::octet_string), nullptr);
    EXPECT_NE(universal_tag_name(UT::null), nullptr);
    EXPECT_NE(universal_tag_name(UT::object_identifier), nullptr);
    EXPECT_NE(universal_tag_name(UT::real), nullptr);
    EXPECT_NE(universal_tag_name(UT::enumerated), nullptr);
    EXPECT_NE(universal_tag_name(UT::utf8_string), nullptr);
    EXPECT_NE(universal_tag_name(UT::sequence), nullptr);
    EXPECT_NE(universal_tag_name(UT::set), nullptr);
    EXPECT_NE(universal_tag_name(UT::ia5_string), nullptr);
    EXPECT_NE(universal_tag_name(UT::utc_time), nullptr);
    EXPECT_NE(universal_tag_name(UT::generalized_time), nullptr);
    EXPECT_NE(universal_tag_name(UT::bmp_string), nullptr);
}
