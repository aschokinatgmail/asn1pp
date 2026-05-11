#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <span>
#include <vector>

#include "codec/xer/encoder.hpp"
#include "codec/xer/decoder.hpp"

using namespace asn1pp::xer;

// ============================================================================
// XER Encoder tests
// ============================================================================

TEST(XerEncoder, EncodeInteger) {
    xer_encoder enc;
    std::string out;
    auto r = enc.encode_integer("value", 42, out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "<value>42</value>");
}

TEST(XerEncoder, EncodeIntegerNegative) {
    xer_encoder enc;
    std::string out;
    auto r = enc.encode_integer("value", -17, out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "<value>-17</value>");
}

TEST(XerEncoder, EncodeIntegerZero) {
    xer_encoder enc;
    std::string out;
    auto r = enc.encode_integer("value", 0, out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "<value>0</value>");
}

TEST(XerEncoder, EncodeBooleanTrue) {
    xer_encoder enc;
    std::string out;
    auto r = enc.encode_boolean("flag", true, out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "<flag><true/></flag>");
}

TEST(XerEncoder, EncodeBooleanFalse) {
    xer_encoder enc;
    std::string out;
    auto r = enc.encode_boolean("flag", false, out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "<flag><false/></flag>");
}

TEST(XerEncoder, EncodeNull) {
    xer_encoder enc;
    std::string out;
    auto r = enc.encode_null("empty", out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "<empty/>");
}

TEST(XerEncoder, EncodeOctetStringBase64) {
    xer_encoder enc;
    std::string out;
    const uint8_t data[] = {0x00, 0x01, 0x02, 0xFF};
    auto r = enc.encode_octet_string("data", std::span<const uint8_t>(data, 4), out);
    ASSERT_TRUE(r.is_ok());
    // base64 of [0x00, 0x01, 0x02, 0xFF] = "AAEC/w=="
    // Actually let's compute: 0x000102FF in binary:
    // 00000000 00000001 00000010 11111111
    // base64 groups of 6 bits:
    // 000000 000000 000100 001011 111111 00xxxx -> pad with ==
    // A A E L / w ==
    EXPECT_EQ(out, "<data>AAEC/w==</data>");
}

TEST(XerEncoder, EncodeBitStringBase64) {
    xer_encoder enc;
    std::string out;
    const uint8_t data[] = {0xAA, 0xBB};  // 10101010 10111011
    auto r = enc.encode_bit_string("bits", std::span<const uint8_t>(data, 2), out);
    ASSERT_TRUE(r.is_ok());
    // base64 of [0xAA, 0xBB]:
    // 10101010 10111011
    // 101010 101011 101100 -> in base64: q q s (pad with =)
    // Actually: 101010 -> 42 -> q, 101011 -> 43 -> r, 1011xx -> pad with one =
    // Wait, 3 bytes = 24 bits. 2 bytes = 16 bits needs padding.
    // 101010 101011 1011xx -> padding adds 00: 101100 -> 44 -> s
    // So: qrs=
    EXPECT_EQ(out, "<bits>qrs=</bits>");
}

TEST(XerEncoder, EncodeOctetStringEmpty) {
    xer_encoder enc;
    std::string out;
    auto r = enc.encode_octet_string("data", std::span<const uint8_t>{}, out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "<data></data>");
}

TEST(XerEncoder, EncodeEnumerated) {
    xer_encoder enc;
    std::string out;
    const char* names[] = {"red", "green", "blue", nullptr};
    auto r = enc.encode_enumerated("color", 1, names, out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "<color>green</color>");
}

TEST(XerEncoder, EncodeEnumeratedFirst) {
    xer_encoder enc;
    std::string out;
    const char* names[] = {"red", "green", "blue", nullptr};
    auto r = enc.encode_enumerated("color", 0, names, out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(out, "<color>red</color>");
}

TEST(XerEncoder, OpenCloseSequence) {
    xer_encoder enc;
    std::string out;
    enc.open_sequence("Seq", out);
    out += "<a>42</a>";
    enc.close_sequence("Seq", out);
    EXPECT_EQ(out, "<Seq><a>42</a></Seq>");
}

TEST(XerEncoder, SequenceWithTwoFields) {
    xer_encoder enc;
    std::string out;
    enc.open_sequence("MySeq", out);
    (void)enc.encode_integer("a", 42, out);
    (void)enc.encode_boolean("b", true, out);
    enc.close_sequence("MySeq", out);
    EXPECT_EQ(out, "<MySeq><a>42</a><b><true/></b></MySeq>");
}

TEST(XerEncoder, SequenceWithOptionalAbsent) {
    xer_encoder enc;
    std::string out;
    enc.open_sequence("Seq", out);
    // Field 'a' present, field 'b' optional and absent -> skip
    (void)enc.encode_integer("a", 10, out);
    enc.close_sequence("Seq", out);
    EXPECT_EQ(out, "<Seq><a>10</a></Seq>");
}

TEST(XerEncoder, OpenCloseChoice) {
    xer_encoder enc;
    std::string out;
    enc.open_choice("Choice", out);
    (void)enc.encode_integer("alt1", 42, out);
    enc.close_choice("Choice", out);
    EXPECT_EQ(out, "<Choice><alt1>42</alt1></Choice>");
}

TEST(XerEncoder, SequenceOf) {
    xer_encoder enc;
    std::string out;
    enc.open_sequence_of("List", out);
    (void)enc.encode_integer("item", 1, out);
    (void)enc.encode_integer("item", 2, out);
    (void)enc.encode_integer("item", 3, out);
    enc.close_sequence_of("List", out);
    EXPECT_EQ(out, "<List><item>1</item><item>2</item><item>3</item></List>");
}

TEST(XerEncoder, OpenRawElement) {
    xer_encoder enc;
    std::string out;
    enc.open_element("raw", out);
    out += "content";
    enc.close_element("raw", out);
    EXPECT_EQ(out, "<raw>content</raw>");
}

TEST(XerEncoder, EncodeRawTextElement) {
    xer_encoder enc;
    std::string out;
    enc.element_with_text("msg", "hello", out);
    EXPECT_EQ(out, "<msg>hello</msg>");
}

// ============================================================================
// XML Parser tests (hand-written, no external library)
// ============================================================================

TEST(XmlParser, ParseEmptyElement) {
    xml_parser parser;
    auto result = parser.parse("<tag/>");
    ASSERT_TRUE(result.is_ok());
    auto& elem = result.value();
    EXPECT_EQ(elem.name, "tag");
    EXPECT_TRUE(elem.is_empty);
    EXPECT_TRUE(elem.content.empty());
    EXPECT_TRUE(elem.children.empty());
}

TEST(XmlParser, ParseElementWithContent) {
    xml_parser parser;
    auto result = parser.parse("<tag>hello</tag>");
    ASSERT_TRUE(result.is_ok());
    auto& elem = result.value();
    EXPECT_EQ(elem.name, "tag");
    EXPECT_FALSE(elem.is_empty);
    EXPECT_EQ(elem.content, "hello");
}

TEST(XmlParser, ParseElementWithIntegerContent) {
    xml_parser parser;
    auto result = parser.parse("<value>42</value>");
    ASSERT_TRUE(result.is_ok());
    auto& elem = result.value();
    EXPECT_EQ(elem.name, "value");
    EXPECT_EQ(elem.content, "42");
}

TEST(XmlParser, ParseNestedElements) {
    xml_parser parser;
    auto result = parser.parse("<Seq><a>42</a><b><true/></b></Seq>");
    ASSERT_TRUE(result.is_ok());
    auto& elem = result.value();
    EXPECT_EQ(elem.name, "Seq");
    ASSERT_EQ(elem.children.size(), 2u);
    EXPECT_EQ(elem.children[0].name, "a");
    EXPECT_EQ(elem.children[0].content, "42");
    EXPECT_EQ(elem.children[1].name, "b");
    ASSERT_EQ(elem.children[1].children.size(), 1u);
    EXPECT_EQ(elem.children[1].children[0].name, "true");
    EXPECT_TRUE(elem.children[1].children[0].is_empty);
}

TEST(XmlParser, ParseDeeplyNested) {
    xml_parser parser;
    auto result = parser.parse("<A><B><C>deep</C></B></A>");
    ASSERT_TRUE(result.is_ok());
    auto& elem = result.value();
    EXPECT_EQ(elem.name, "A");
    ASSERT_EQ(elem.children.size(), 1u);
    EXPECT_EQ(elem.children[0].name, "B");
    ASSERT_EQ(elem.children[0].children.size(), 1u);
    EXPECT_EQ(elem.children[0].children[0].name, "C");
    EXPECT_EQ(elem.children[0].children[0].content, "deep");
}

TEST(XmlParser, ParseContentWithSpecialChars) {
    xml_parser parser;
    auto result = parser.parse("<tag>a&lt;b&gt;c</tag>");
    ASSERT_TRUE(result.is_ok());
    auto& elem = result.value();
    EXPECT_EQ(elem.content, "a<b>c");
}

TEST(XmlParser, ParseMultipleTopLevel) {
    xml_parser parser;
    auto result = parser.parse("<List><item>1</item><item>2</item></List>");
    ASSERT_TRUE(result.is_ok());
    auto& elem = result.value();
    EXPECT_EQ(elem.name, "List");
    ASSERT_EQ(elem.children.size(), 2u);
    EXPECT_EQ(elem.children[0].content, "1");
    EXPECT_EQ(elem.children[1].content, "2");
}

TEST(XmlParser, ParseErrorUnclosed) {
    xml_parser parser;
    auto result = parser.parse("<tag>unclosed");
    EXPECT_TRUE(result.is_err());
}

TEST(XmlParser, ParseErrorMismatchedTag) {
    xml_parser parser;
    auto result = parser.parse("<tag>content</other>");
    EXPECT_TRUE(result.is_err());
}

TEST(XmlParser, ParseElementNoContent) {
    xml_parser parser;
    auto result = parser.parse("<tag></tag>");
    ASSERT_TRUE(result.is_ok());
    auto& elem = result.value();
    EXPECT_EQ(elem.name, "tag");
    EXPECT_FALSE(elem.is_empty);
    EXPECT_TRUE(elem.content.empty());
}

// ============================================================================
// XER Decoder tests
// ============================================================================

TEST(XerDecoder, DecodeInteger) {
    xer_decoder dec;
    auto result = dec.decode_integer("<value>42</value>");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 42);
}

TEST(XerDecoder, DecodeIntegerNegative) {
    xer_decoder dec;
    auto result = dec.decode_integer("<value>-17</value>");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), -17);
}

TEST(XerDecoder, DecodeBooleanTrue) {
    xer_decoder dec;
    auto result = dec.decode_boolean("<flag><true/></flag>");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), true);
}

TEST(XerDecoder, DecodeBooleanFalse) {
    xer_decoder dec;
    auto result = dec.decode_boolean("<flag><false/></flag>");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), false);
}

TEST(XerDecoder, DecodeOctetString) {
#ifndef ASN1PP_EMBEDDED
    xer_decoder dec;
    auto result = dec.decode_octet_string("<data>AAEC/w==</data>");
    ASSERT_TRUE(result.is_ok());
    auto& vec = result.value();
    ASSERT_EQ(vec.size(), 4u);
    EXPECT_EQ(vec[0], 0x00);
    EXPECT_EQ(vec[1], 0x01);
    EXPECT_EQ(vec[2], 0x02);
    EXPECT_EQ(vec[3], 0xFF);
#endif // ASN1PP_EMBEDDED
}

TEST(XerDecoder, DecodeOctetStringEmpty) {
#ifndef ASN1PP_EMBEDDED
    xer_decoder dec;
    auto result = dec.decode_octet_string("<data></data>");
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value().empty());
#endif // ASN1PP_EMBEDDED
}

TEST(XerDecoder, DecodeEnumerated) {
    xer_decoder dec;
    const char* names[] = {"red", "green", "blue", nullptr};
    auto result = dec.decode_enumerated("<color>green</color>", names);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 1);
}

TEST(XerDecoder, DecodeEnumeratedFirst) {
    xer_decoder dec;
    const char* names[] = {"red", "green", "blue", nullptr};
    auto result = dec.decode_enumerated("<color>red</color>", names);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 0);
}

TEST(XerDecoder, GetFieldContentFromSequence) {
    xer_decoder dec;
    auto result = dec.decode_sequence_field_content(
        "<Seq><a>42</a><b>hello</b></Seq>", "a");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), "42");
}

TEST(XerDecoder, GetFieldContentSecondField) {
    xer_decoder dec;
    auto result = dec.decode_sequence_field_content(
        "<Seq><a>42</a><b>hello</b></Seq>", "b");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), "hello");
}

TEST(XerDecoder, SequenceFieldIsPresent) {
    xer_decoder dec;
    // Parse then check
    auto open_r = dec.open_sequence("Seq", "<Seq><a>42</a></Seq>");
    ASSERT_TRUE(open_r.is_ok());
    EXPECT_TRUE(dec.field_is_present("a"));
    EXPECT_FALSE(dec.field_is_present("b"));
}

TEST(XerDecoder, ChoiceAlternative) {
    xer_decoder dec;
    auto result = dec.decode_choice_alternative("<Choice><alt1>42</alt1></Choice>");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), "alt1");
}

TEST(XerDecoder, ChoiceAlternativeSecond) {
    xer_decoder dec;
    auto result = dec.decode_choice_alternative("<Choice><alt2>hello</alt2></Choice>");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), "alt2");
}

TEST(XerDecoder, ChildContentForChoice) {
    xer_decoder dec;
    auto result = dec.decode_choice_child_content("<Choice><alt1>42</alt1></Choice>");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), "42");
}

TEST(XerDecoder, DecodeBitString) {
#ifndef ASN1PP_EMBEDDED
    xer_decoder dec;
    auto result = dec.decode_bit_string("<bits>qrs=</bits>");
    ASSERT_TRUE(result.is_ok());
    auto& vec = result.value();
    ASSERT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec[0], 0xAA);
    EXPECT_EQ(vec[1], 0xBB);
#endif // ASN1PP_EMBEDDED
}

// ============================================================================
// Round-trip tests: encode -> decode -> verify
// ============================================================================

TEST(XerRoundTrip, Integer) {
    xer_encoder enc;
    xer_decoder dec;
    std::string xml;
    (void)enc.encode_integer("value", 12345, xml);
    auto result = dec.decode_integer(xml);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 12345);
}

TEST(XerRoundTrip, IntegerNegative) {
    xer_encoder enc;
    xer_decoder dec;
    std::string xml;
    (void)enc.encode_integer("value", -999, xml);
    auto result = dec.decode_integer(xml);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), -999);
}

TEST(XerRoundTrip, BooleanTrue) {
    xer_encoder enc;
    xer_decoder dec;
    std::string xml;
    (void)enc.encode_boolean("flag", true, xml);
    auto result = dec.decode_boolean(xml);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), true);
}

TEST(XerRoundTrip, BooleanFalse) {
    xer_encoder enc;
    xer_decoder dec;
    std::string xml;
    (void)enc.encode_boolean("flag", false, xml);
    auto result = dec.decode_boolean(xml);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), false);
}

TEST(XerRoundTrip, OctetString) {
#ifndef ASN1PP_EMBEDDED
    xer_encoder enc;
    xer_decoder dec;
    std::string xml;
    const uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    (void)enc.encode_octet_string("data", std::span<const uint8_t>(data, 4), xml);
    auto result = dec.decode_octet_string(xml);
    ASSERT_TRUE(result.is_ok());
    auto& vec = result.value();
    ASSERT_EQ(vec.size(), 4u);
    EXPECT_EQ(vec[0], 0xDE);
    EXPECT_EQ(vec[1], 0xAD);
    EXPECT_EQ(vec[2], 0xBE);
    EXPECT_EQ(vec[3], 0xEF);
#endif // ASN1PP_EMBEDDED
}

TEST(XerRoundTrip, OctetStringEmpty) {
#ifndef ASN1PP_EMBEDDED
    xer_encoder enc;
    xer_decoder dec;
    std::string xml;
    (void)enc.encode_octet_string("data", std::span<const uint8_t>{}, xml);
    auto result = dec.decode_octet_string(xml);
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value().empty());
#endif // ASN1PP_EMBEDDED
}

TEST(XerRoundTrip, BitString) {
#ifndef ASN1PP_EMBEDDED
    xer_encoder enc;
    xer_decoder dec;
    std::string xml;
    const uint8_t data[] = {0x12, 0x34, 0x56};
    (void)enc.encode_bit_string("bits", std::span<const uint8_t>(data, 3), xml);
    auto result = dec.decode_bit_string(xml);
    ASSERT_TRUE(result.is_ok());
    auto& vec = result.value();
    ASSERT_EQ(vec.size(), 3u);
    EXPECT_EQ(vec[0], 0x12);
    EXPECT_EQ(vec[1], 0x34);
    EXPECT_EQ(vec[2], 0x56);
#endif // ASN1PP_EMBEDDED
}

TEST(XerRoundTrip, Enumerated) {
    xer_encoder enc;
    xer_decoder dec;
    const char* names[] = {"alpha", "beta", "gamma", nullptr};
    std::string xml;
    (void)enc.encode_enumerated("letter", 2, names, xml);
    auto result = dec.decode_enumerated(xml, names);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 2);
}

TEST(XerRoundTrip, SequenceRoundTrip) {
    xer_encoder enc;
    xer_decoder dec;
    std::string xml;
    enc.open_sequence("Seq", xml);
    (void)enc.encode_integer("id", 100, xml);
    (void)enc.encode_boolean("active", true, xml);
    enc.close_sequence("Seq", xml);

    // Decode individual fields back
    auto id_result = dec.decode_sequence_field_content(xml, "id");
    ASSERT_TRUE(id_result.is_ok());
    EXPECT_EQ(dec.decode_integer_from_string(id_result.value()), 100);

    auto flag_xml = std::string("<flag>") + std::string(dec.decode_sequence_field_xml(xml, "active").value()) + std::string("</flag>");
    // Actually, let's test using field content + boolean decode
    auto active_content = dec.decode_sequence_field_content(xml, "active");
    ASSERT_TRUE(active_content.is_ok());
    // active_content contains the child XML like "<true/>"
    EXPECT_TRUE(active_content.value().find("<true/>") != std::string_view::npos ||
                active_content.value().find("true") != std::string_view::npos);
}

// ============================================================================
// Base64 utility tests
// ============================================================================

TEST(Base64, EncodeDecodeRoundTrip) {
    std::vector<uint8_t> data = {0x00, 0x01, 0x02, 0xFF, 0xAA, 0x55};
    std::string encoded = base64_encode(data.data(), data.size());
    auto decoded = base64_decode(encoded);
    EXPECT_EQ(decoded, data);
}

TEST(Base64, EncodeEmpty) {
    std::string encoded = base64_encode(nullptr, 0);
    EXPECT_EQ(encoded, "");
}

TEST(Base64, EncodeKnownVector) {
    // "Man" -> "TWFu"
    const uint8_t man[] = {'M', 'a', 'n'};
    std::string encoded = base64_encode(man, 3);
    EXPECT_EQ(encoded, "TWFu");
}

TEST(Base64, DecodeKnownVector) {
    auto decoded = base64_decode("TWFu");
    ASSERT_TRUE(decoded.size() == 3);
    EXPECT_EQ(decoded[0], 'M');
    EXPECT_EQ(decoded[1], 'a');
    EXPECT_EQ(decoded[2], 'n');
}
