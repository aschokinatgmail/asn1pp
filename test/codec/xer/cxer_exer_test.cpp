#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "codec/xer/cxer_validator.hpp"
#include "codec/xer/exer_encoder.hpp"

using namespace asn1pp::xer;

TEST(CxerValidator, ValidXmlNoExtraWhitespace) {
    std::string xml = "<value>42</value>";
    auto r = cxer_validator::validate(xml);
    EXPECT_TRUE(r.is_ok());
}

TEST(CxerValidator, ValidXmlWithSingleSpace) {
    std::string xml = "<Sequence><Integer>42</Integer></Sequence>";
    auto r = cxer_validator::validate(xml);
    EXPECT_TRUE(r.is_ok());
}

TEST(CxerValidator, RejectExtraWhitespaceBetweenTags) {
    std::string xml = "<value>42</value>   <other>100</other>";
    auto r = cxer_validator::validate(xml);
    EXPECT_TRUE(r.is_err());
}

TEST(CxerValidator, RejectWhitespaceInsideElement) {
    std::string xml = "<value>  42  </value>";
    auto r = cxer_validator::validate(xml);
    EXPECT_TRUE(r.is_err());
}

TEST(CxerValidator, RejectNewlineBetweenElements) {
    std::string xml = "<elem1>test</elem1>\n<elem2>other</elem2>";
    auto r = cxer_validator::validate(xml);
    EXPECT_TRUE(r.is_err());
}

TEST(CxerValidator, ValidNestedStructure) {
    std::string xml = "<Sequence><Integer>42</Integer><Boolean>true</Boolean></Sequence>";
    auto r = cxer_validator::validate(xml);
    EXPECT_TRUE(r.is_ok());
}

TEST(CxerValidator, ValidEmptyElements) {
    std::string xml = "<Empty/><Null/>";
    auto r = cxer_validator::validate(xml);
    EXPECT_TRUE(r.is_ok());
}

TEST(ExerEncoder, EncodeSequenceStartWithNamespace) {
    std::string out;
    auto r = exer_encoder::encode_sequence_start("Sequence", out);
    ASSERT_TRUE(r.is_ok());
    EXPECT_TRUE(out.find("xmlns:asn1=") != std::string::npos);
    EXPECT_TRUE(out.find("asn1:type=") != std::string::npos);
}

TEST(ExerEncoder, AddAsn1Namespace) {
    std::string out = "<Sequence";
    exer_encoder::add_asn1_namespace(out);
    out += ">";
    EXPECT_TRUE(out.find("xmlns:asn1=\"urn:asn1pp:asn1\"") != std::string::npos);
}

TEST(ExerEncoder, AddTypeAttribute) {
    std::string out;
    exer_encoder::add_type_attribute("INTEGER", out);
    EXPECT_TRUE(out.find("asn1:type=\"INTEGER\"") != std::string::npos);
}

TEST(ExerEncoder, FullSequenceWithAttributes) {
    std::string out;
    exer_encoder::encode_sequence_start("Sequence", out);
    out += "<Integer>42</Integer>";
    out += "<Boolean>true</Boolean>";
    xer_encoder enc;
    enc.close_element("Sequence", out);

    EXPECT_TRUE(out.find("xmlns:asn1") != std::string::npos);
    EXPECT_TRUE(out.find("asn1:type=\"Sequence\"") != std::string::npos);
    EXPECT_TRUE(out.find("<Integer>42</Integer>") != std::string::npos);
}

TEST(ExerEncoder, EncodeElementWithType) {
    std::string out;
    exer_encoder::encode_element_with_type("data", "OCTET-STRING", "AQID", out);
    EXPECT_TRUE(out.find("asn1:type=\"OCTET-STRING\"") != std::string::npos);
    EXPECT_TRUE(out.find("<data") != std::string::npos);
    EXPECT_TRUE(out.find("</data>") != std::string::npos);
}