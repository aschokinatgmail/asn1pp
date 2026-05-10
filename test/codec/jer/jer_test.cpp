#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <span>
#include <cstring>
#include <stdexcept>

#include "codec/result.hpp"
#include "codec/jer/encoder.hpp"
#include "codec/jer/decoder.hpp"

using namespace asn1pp;
using namespace asn1pp::jer;

// ============================================================================
// 1. INTEGER encoding / decoding
// ============================================================================

TEST(JerIntegerTest, Encode42_UT_JER_ENC_001) {
    jer_encoder enc;
    std::string out;
    auto r = enc.encode_integer(42, out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "42");
}

TEST(JerIntegerTest, EncodeZero_UT_JER_ENC_002) {
    jer_encoder enc;
    std::string out;
    auto r = enc.encode_integer(0, out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "0");
}

TEST(JerIntegerTest, EncodeNegative_UT_JER_ENC_003) {
    jer_encoder enc;
    std::string out;
    auto r = enc.encode_integer(-123, out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "-123");
}

TEST(JerIntegerTest, DecodeJsonNumber_UT_JER_DEC_001) {
    jer_decoder dec;
    auto r = dec.decode_integer("42");
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 42);
}

TEST(JerIntegerTest, DecodeNegativeNumber_UT_JER_DEC_002) {
    jer_decoder dec;
    auto r = dec.decode_integer("-456");
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), -456);
}

TEST(JerIntegerTest, DecodeZero_UT_JER_DEC_003) {
    jer_decoder dec;
    auto r = dec.decode_integer("0");
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0);
}

TEST(JerIntegerTest, DecodeInvalidErrors_UT_JER_DEC_004) {
    jer_decoder dec;
    auto r = dec.decode_integer("notanumber");
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::parse_error);
}

// ============================================================================
// 2. BOOLEAN encoding / decoding
// ============================================================================

TEST(JerBooleanTest, EncodeTrue_UT_JER_ENC_010) {
    jer_encoder enc;
    std::string out;
    auto r = enc.encode_boolean(true, out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "true");
}

TEST(JerBooleanTest, EncodeFalse_UT_JER_ENC_011) {
    jer_encoder enc;
    std::string out;
    auto r = enc.encode_boolean(false, out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "false");
}

TEST(JerBooleanTest, DecodeTrue_UT_JER_DEC_010) {
    jer_decoder dec;
    auto r = dec.decode_boolean("true");
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), true);
}

TEST(JerBooleanTest, DecodeFalse_UT_JER_DEC_011) {
    jer_decoder dec;
    auto r = dec.decode_boolean("false");
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), false);
}

TEST(JerBooleanTest, DecodeBadBoolean_UT_JER_DEC_012) {
    jer_decoder dec;
    auto r = dec.decode_boolean("yes");
    ASSERT_TRUE(r.is_err());
}

// ============================================================================
// 3. NULL encoding / decoding
// ============================================================================

TEST(JerNullTest, EncodeNull_UT_JER_ENC_020) {
    jer_encoder enc;
    std::string out;
    auto r = enc.encode_null(out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "null");
}

TEST(JerNullTest, DecodeNull_UT_JER_DEC_020) {
    jer_decoder dec;
    auto r = dec.decode_null("null");
    ASSERT_TRUE(r.is_ok());
}

TEST(JerNullTest, DecodeBadNull_UT_JER_DEC_021) {
    jer_decoder dec;
    auto r = dec.decode_null("notnull");
    ASSERT_TRUE(r.is_err());
}

// ============================================================================
// 4. OCTET STRING encoding / decoding (base64)
// ============================================================================

TEST(JerOctetStringTest, EncodeAQID_UT_JER_ENC_030) {
    jer_encoder enc;
    std::string out;
    const uint8_t data[] = {0x01, 0x02, 0x03};
    auto r = enc.encode_octet_string(std::span<const uint8_t>(data, 3), out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "\"AQID\"");
}

TEST(JerOctetStringTest, EncodeEmpty_UT_JER_ENC_031) {
    jer_encoder enc;
    std::string out;
    auto r = enc.encode_octet_string(std::span<const uint8_t>{}, out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "\"\"");
}

TEST(JerOctetStringTest, DecodeAQID_UT_JER_DEC_030) {
    jer_decoder dec;
    auto r = dec.decode_octet_string("\"AQID\"");
    ASSERT_TRUE(r.is_ok());
    const auto& vec = r.value();
    ASSERT_EQ(vec.size(), 3u);
    EXPECT_EQ(vec[0], 0x01);
    EXPECT_EQ(vec[1], 0x02);
    EXPECT_EQ(vec[2], 0x03);
}

TEST(JerOctetStringTest, DecodeEmpty_UT_JER_DEC_031) {
    jer_decoder dec;
    auto r = dec.decode_octet_string("\"\"");
    ASSERT_TRUE(r.is_ok());
    EXPECT_TRUE(r.value().empty());
}

// ============================================================================
// 5. ENUMERATED encoding / decoding
// ============================================================================

TEST(JerEnumeratedTest, EncodeRed_UT_JER_ENC_040) {
    jer_encoder enc;
    std::string out;
    const char* names[] = {"red", "green", "blue"};
    auto r = enc.encode_enumerated(0, names, out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "\"red\"");
}

TEST(JerEnumeratedTest, EncodeGreen_UT_JER_ENC_041) {
    jer_encoder enc;
    std::string out;
    const char* names[] = {"red", "green", "blue"};
    auto r = enc.encode_enumerated(1, names, out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "\"green\"");
}

TEST(JerEnumeratedTest, DecodeBlue_UT_JER_DEC_040) {
    jer_decoder dec;
    const char* names[] = {"red", "green", "blue"};
    auto r = dec.decode_enumerated("\"blue\"", names, 3);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 2);
}

TEST(JerEnumeratedTest, DecodeNotFound_UT_JER_DEC_041) {
    jer_decoder dec;
    const char* names[] = {"red", "green", "blue"};
    auto r = dec.decode_enumerated("\"yellow\"", names, 3);
    ASSERT_TRUE(r.is_err());
}

// ============================================================================
// 6. SEQUENCE object helpers (encoder)
// ============================================================================

TEST(JerSequenceTest, EncodeTwoFieldSequence_UT_JER_ENC_050) {
    jer_encoder enc;
    std::string out;

    // Manually build: {"a":42,"b":true}
    enc.open_object(out);
    enc.add_key("a", out);
    enc.encode_integer(42, out);
    enc.add_key("b", out);
    enc.encode_boolean(true, out);
    enc.close_object(out);

    EXPECT_EQ(out, "{\"a\":42,\"b\":true}");
}

TEST(JerSequenceTest, EncodeSequenceWithOptionalAbsent_UT_JER_ENC_051) {
    jer_encoder enc;
    std::string out;

    // Only mandatory fields present: {"mandatory":1}
    enc.open_object(out);
    enc.add_key("mandatory", out);
    enc.encode_integer(1, out);
    enc.close_object(out);

    EXPECT_EQ(out, "{\"mandatory\":1}");
}

TEST(JerSequenceTest, EncodeSequenceWithOctalString_UT_JER_ENC_052) {
    jer_encoder enc;
    std::string out;

    const uint8_t data[] = {0xAA, 0xBB};
    enc.open_object(out);
    enc.add_key("payload", out);
    enc.encode_octet_string(std::span<const uint8_t>(data, 2), out);
    enc.close_object(out);

    EXPECT_EQ(out, "{\"payload\":\"qrs=\"}");
}

// ============================================================================
// 7. CHOICE encoding
// ============================================================================

TEST(JerChoiceTest, EncodeChoiceAlt_UT_JER_ENC_060) {
    jer_encoder enc;
    std::string out;

    enc.open_object(out);
    enc.add_key("alt1", out);
    enc.encode_integer(42, out);
    enc.close_object(out);

    EXPECT_EQ(out, "{\"alt1\":42}");
}

// ============================================================================
// 8. SEQUENCE OF (arrays)
// ============================================================================

TEST(JerSequenceOfTest, EncodeArrayOfIntegers_UT_JER_ENC_070) {
    jer_encoder enc;
    std::string out;

    enc.open_array(out);
    enc.encode_integer(1, out);
    enc.encode_integer(2, out);
    enc.encode_integer(3, out);
    enc.close_array(out);

    EXPECT_EQ(out, "[1,2,3]");
}

TEST(JerSequenceOfTest, EncodeEmptyArray_UT_JER_ENC_071) {
    jer_encoder enc;
    std::string out;

    enc.open_array(out);
    enc.close_array(out);

    EXPECT_EQ(out, "[]");
}

// ============================================================================
// 9. BIT STRING (base64 + unused bits)
// ============================================================================

TEST(JerBitStringTest, EncodeBitString_UT_JER_ENC_080) {
    jer_encoder enc;
    std::string out;
    const uint8_t data[] = {0xA0}; // 1010 0000
    auto r = enc.encode_bit_string(std::span<const uint8_t>(data, 1), 4, out);
    ASSERT_TRUE(r.is_ok());
    // The base64 of {0xA0} is "oA=="
    EXPECT_EQ(out, "{\"value\":\"oA==\",\"unused\":4}");
}

// ============================================================================
// 10. OID encoding / decoding
// ============================================================================

TEST(JerOidTest, EncodeOID_UT_JER_ENC_090) {
    jer_encoder enc;
    std::string out;
    auto r = enc.encode_oid("1.2.840.113549", out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "\"1.2.840.113549\"");
}

TEST(JerOidTest, DecodeOID_UT_JER_DEC_090) {
    jer_decoder dec;
    auto r = dec.decode_oid("\"1.2.840.113549\"");
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), "1.2.840.113549");
}

// ============================================================================
// 11. JSON Parser: basic types
// ============================================================================

TEST(JerParserTest, ParseTrue_UT_JER_PARSE_001) {
    jer_json_parser parser("true");
    auto val = parser.parse_value();
    ASSERT_TRUE(val.is_ok());
    EXPECT_EQ(val.value().type, json_value_type::boolean);
    EXPECT_EQ(val.value().bool_val, true);
}

TEST(JerParserTest, ParseFalse_UT_JER_PARSE_002) {
    jer_json_parser parser("false");
    auto val = parser.parse_value();
    ASSERT_TRUE(val.is_ok());
    EXPECT_EQ(val.value().bool_val, false);
}

TEST(JerParserTest, ParseNull_UT_JER_PARSE_003) {
    jer_json_parser parser("null");
    auto val = parser.parse_value();
    ASSERT_TRUE(val.is_ok());
    EXPECT_EQ(val.value().type, json_value_type::nil);
}

TEST(JerParserTest, ParsePositiveNumber_UT_JER_PARSE_004) {
    jer_json_parser parser("42");
    auto val = parser.parse_value();
    ASSERT_TRUE(val.is_ok());
    EXPECT_EQ(val.value().type, json_value_type::number);
    EXPECT_EQ(val.value().number_val, 42);
}

TEST(JerParserTest, ParseNegativeNumber_UT_JER_PARSE_005) {
    jer_json_parser parser("-999");
    auto val = parser.parse_value();
    ASSERT_TRUE(val.is_ok());
    EXPECT_EQ(val.value().number_val, -999);
}

TEST(JerParserTest, ParseString_UT_JER_PARSE_006) {
    jer_json_parser parser("\"hello\"");
    auto val = parser.parse_value();
    ASSERT_TRUE(val.is_ok());
    EXPECT_EQ(val.value().type, json_value_type::string_val);
    EXPECT_EQ(val.value().string_val, "hello");
}

TEST(JerParserTest, ParseEmptyString_UT_JER_PARSE_007) {
    jer_json_parser parser("\"\"");
    auto val = parser.parse_value();
    ASSERT_TRUE(val.is_ok());
    EXPECT_EQ(val.value().string_val, "");
}

TEST(JerParserTest, ParseObject_UT_JER_PARSE_008) {
    jer_json_parser parser("{\"a\":42}");
    auto val = parser.parse_value();
    ASSERT_TRUE(val.is_ok());
    EXPECT_EQ(val.value().type, json_value_type::object);
    ASSERT_NE(val.value().object_val, nullptr);
    EXPECT_EQ(val.value().object_val->size(), 1u);
    EXPECT_EQ((*val.value().object_val)["a"].number_val, 42);
}

TEST(JerParserTest, ParseArray_UT_JER_PARSE_009) {
    jer_json_parser parser("[1,2,3]");
    auto val = parser.parse_value();
    ASSERT_TRUE(val.is_ok());
    EXPECT_EQ(val.value().type, json_value_type::array);
    ASSERT_NE(val.value().array_val, nullptr);
    EXPECT_EQ(val.value().array_val->size(), 3u);
    EXPECT_EQ((*val.value().array_val)[0].number_val, 1);
    EXPECT_EQ((*val.value().array_val)[1].number_val, 2);
    EXPECT_EQ((*val.value().array_val)[2].number_val, 3);
}

// ============================================================================
// 12. JSON Parser: object field extraction
// ============================================================================

TEST(JerParserTest, ParseFieldExtraction_UT_JER_PARSE_010) {
    // Parse {"field1":42,"field2":true}
    jer_json_parser parser("{\"field1\":42,\"field2\":true}");
    auto val = parser.parse_value();
    ASSERT_TRUE(val.is_ok());
    EXPECT_EQ(val.value().type, json_value_type::object);

    const auto& obj = *val.value().object_val;
    ASSERT_TRUE(obj.count("field1"));
    EXPECT_EQ(obj.at("field1").number_val, 42);

    ASSERT_TRUE(obj.count("field2"));
    EXPECT_EQ(obj.at("field2").bool_val, true);
}

// ============================================================================
// 13. Decoder: object-level decoding helpers
// ============================================================================

TEST(JerDecoderTest, DecodeObjectField_UT_JER_DEC_050) {
    jer_decoder dec;
    auto val = dec.parse_value("{\"a\":42}");
    ASSERT_TRUE(val.is_ok());
    auto field = dec.get_field(val.value(), "a");
    ASSERT_TRUE(field.is_ok());
    EXPECT_EQ(field.value().number_val, 42);
}

TEST(JerDecoderTest, DecodeMissingField_UT_JER_DEC_051) {
    jer_decoder dec;
    auto val = dec.parse_value("{\"a\":42}");
    ASSERT_TRUE(val.is_ok());
    auto field = dec.get_field(val.value(), "missing");
    ASSERT_TRUE(field.is_err());
}

// ============================================================================
// 14. Round-trip tests: encode → decode → compare
// ============================================================================

TEST(JerRoundTripTest, IntegerRoundTrip_UT_JER_RT_001) {
    jer_encoder enc;
    jer_decoder dec;

    std::string json;
    enc.encode_integer(12345, json);
    auto r = dec.decode_integer(json);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 12345);
}

TEST(JerRoundTripTest, NegativeIntegerRoundTrip_UT_JER_RT_002) {
    jer_encoder enc;
    jer_decoder dec;

    std::string json;
    enc.encode_integer(-9876, json);
    auto r = dec.decode_integer(json);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), -9876);
}

TEST(JerRoundTripTest, BooleanRoundTrip_UT_JER_RT_003) {
    jer_encoder enc;
    jer_decoder dec;

    std::string json;
    enc.encode_boolean(true, json);
    auto r = dec.decode_boolean(json);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), true);
}

TEST(JerRoundTripTest, OctetStringRoundTrip_UT_JER_RT_004) {
    jer_encoder enc;
    jer_decoder dec;

    const uint8_t original[] = {0xDE, 0xAD, 0xBE, 0xEF};
    std::string json;
    enc.encode_octet_string(std::span<const uint8_t>(original, 4), json);
    auto r = dec.decode_octet_string(json);
    ASSERT_TRUE(r.is_ok());
    const auto& decoded = r.value();
    ASSERT_EQ(decoded.size(), 4u);
    EXPECT_EQ(decoded[0], 0xDE);
    EXPECT_EQ(decoded[1], 0xAD);
    EXPECT_EQ(decoded[2], 0xBE);
    EXPECT_EQ(decoded[3], 0xEF);
}

TEST(JerRoundTripTest, OidRoundTrip_UT_JER_RT_005) {
    jer_encoder enc;
    jer_decoder dec;

    std::string json;
    enc.encode_oid("2.5.4.3", json);
    auto r = dec.decode_oid(json);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), "2.5.4.3");
}

TEST(JerRoundTripTest, SequenceRoundTrip_UT_JER_RT_006) {
    jer_encoder enc;
    jer_decoder dec;

    std::string json;
    enc.open_object(json);
    enc.add_key("id", json);
    enc.encode_integer(100, json);
    enc.add_key("active", json);
    enc.encode_boolean(true, json);
    enc.close_object(json);

    // Parse and verify
    auto val = dec.parse_value(json);
    ASSERT_TRUE(val.is_ok());
    EXPECT_EQ(val.value().type, json_value_type::object);

    auto id_field = dec.get_field(val.value(), "id");
    ASSERT_TRUE(id_field.is_ok());
    EXPECT_EQ(id_field.value().number_val, 100);

    auto active_field = dec.get_field(val.value(), "active");
    ASSERT_TRUE(active_field.is_ok());
    EXPECT_EQ(active_field.value().bool_val, true);
}

TEST(JerRoundTripTest, ArrayRoundTrip_UT_JER_RT_007) {
    jer_encoder enc;
    jer_decoder dec;

    std::string json;
    enc.open_array(json);
    enc.encode_integer(10, json);
    enc.encode_integer(20, json);
    enc.encode_integer(30, json);
    enc.close_array(json);

    auto val = dec.parse_value(json);
    ASSERT_TRUE(val.is_ok());
    EXPECT_EQ(val.value().type, json_value_type::array);
    ASSERT_NE(val.value().array_val, nullptr);
    EXPECT_EQ(val.value().array_val->size(), 3u);
    EXPECT_EQ((*val.value().array_val)[0].number_val, 10);
    EXPECT_EQ((*val.value().array_val)[1].number_val, 20);
    EXPECT_EQ((*val.value().array_val)[2].number_val, 30);
}

TEST(JerRoundTripTest, EnumeratedRoundTrip_UT_JER_RT_008) {
    jer_encoder enc;
    jer_decoder dec;

    const char* names[] = {"small", "medium", "large"};
    std::string json;
    enc.encode_enumerated(2, names, json);
    auto r = dec.decode_enumerated(json, names, 3);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 2);
}

// ============================================================================
// 15. Comma placement (no trailing comma in objects/arrays)
// ============================================================================

TEST(JerFormatTest, NoTrailingCommaObject_UT_JER_FMT_001) {
    jer_encoder enc;
    std::string out;

    enc.open_object(out);
    enc.add_key("x", out);
    enc.encode_integer(1, out);
    enc.close_object(out);

    // No trailing comma
    EXPECT_EQ(out, "{\"x\":1}");
}

TEST(JerFormatTest, NoTrailingCommaArray_UT_JER_FMT_002) {
    jer_encoder enc;
    std::string out;

    enc.open_array(out);
    enc.encode_integer(1, out);
    enc.close_array(out);

    EXPECT_EQ(out, "[1]");
}
