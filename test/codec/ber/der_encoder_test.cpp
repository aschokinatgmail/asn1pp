#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>

#include "codec/ber/der_encoder.hpp"

using namespace asn1pp;
using namespace asn1pp::ber;

namespace {

buffer_view make_mutable_view(std::vector<uint8_t>& vec) {
    return buffer_view(vec);
}

std::vector<uint8_t> remaining_bytes(const std::vector<uint8_t>& buf, size_t encoded_size) {
    return std::vector<uint8_t>(buf.begin(), buf.begin() + static_cast<long>(encoded_size));
}

} // namespace

class BooleanDerTest : public ::testing::Test {
protected:
    der_encoder encoder_;
};

TEST_F(BooleanDerTest, EncodeTrue_UT_DER_001) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_boolean(true, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x01, 0x01, 0xFF}));
}

TEST_F(BooleanDerTest, EncodeFalse_UT_DER_002) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_boolean(false, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x01, 0x01, 0x00}));
}

TEST_F(BooleanDerTest, ValidateBooleanDer_0xFF_OK_UT_DER_003) {
    auto r = encoder_.validate_boolean_der(std::span<const uint8_t>({0xFF}));
    EXPECT_TRUE(r.is_ok());
}

TEST_F(BooleanDerTest, ValidateBooleanDer_0x00_OK_UT_DER_004) {
    auto r = encoder_.validate_boolean_der(std::span<const uint8_t>({0x00}));
    EXPECT_TRUE(r.is_ok());
}

TEST_F(BooleanDerTest, ValidateBooleanDer_0x42_Error_UT_DER_005) {
    auto r = encoder_.validate_boolean_der(std::span<const uint8_t>({0x42}));
    EXPECT_TRUE(r.is_err());
}

TEST_F(BooleanDerTest, ValidateBooleanDer_0x01_Error_UT_DER_006) {
    auto r = encoder_.validate_boolean_der(std::span<const uint8_t>({0x01}));
    EXPECT_TRUE(r.is_err());
}

TEST_F(BooleanDerTest, ValidateBooleanDer_0x7F_Error_UT_DER_007) {
    auto r = encoder_.validate_boolean_der(std::span<const uint8_t>({0x7F}));
    EXPECT_TRUE(r.is_err());
}

TEST_F(BooleanDerTest, ValidateBooleanDer_MultipleBytes_Error_UT_DER_008) {
    auto r = encoder_.validate_boolean_der(std::span<const uint8_t>({0xFF, 0x00}));
    EXPECT_TRUE(r.is_err());
}

TEST_F(BooleanDerTest, ValidateBooleanDer_Empty_Error_UT_DER_009) {
    auto r = encoder_.validate_boolean_der(std::span<const uint8_t>({}));
    EXPECT_TRUE(r.is_err());
}

class IntegerDerTest : public ::testing::Test {
protected:
    der_encoder encoder_;
};

TEST_F(IntegerDerTest, EncodeZero_UT_DER_010) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x02, 0x01, 0x00}));
}

TEST_F(IntegerDerTest, Encode127_UT_DER_011) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(127, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x02, 0x01, 0x7F}));
}

TEST_F(IntegerDerTest, Encode128_UT_DER_012) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(128, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 4), std::vector<uint8_t>({0x02, 0x02, 0x00, 0x80}));
}

TEST_F(IntegerDerTest, EncodeNegativeOne_UT_DER_013) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(-1, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x02, 0x01, 0xFF}));
}

TEST_F(IntegerDerTest, EncodeNegative128_UT_DER_014) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(-128, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x02, 0x01, 0x80}));
}

TEST_F(IntegerDerTest, EncodeNegative129_UT_DER_015) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(-129, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 4), std::vector<uint8_t>({0x02, 0x02, 0xFF, 0x7F}));
}

TEST_F(IntegerDerTest, ValidateIntegerDer_SingleByte_OK_UT_DER_016) {
    auto r = encoder_.validate_integer_der(std::span<const uint8_t>({0x00}));
    EXPECT_TRUE(r.is_ok());
}

TEST_F(IntegerDerTest, ValidateIntegerDer_TwoBytesPositive_OK_UT_DER_017) {
    auto r = encoder_.validate_integer_der(std::span<const uint8_t>({0x00, 0x80}));
    EXPECT_TRUE(r.is_ok());
}

TEST_F(IntegerDerTest, ValidateIntegerDer_UnnecessaryLeadingZero_Error_UT_DER_018) {
    auto r = encoder_.validate_integer_der(std::span<const uint8_t>({0x00, 0x00}));
    EXPECT_TRUE(r.is_err());
}

TEST_F(IntegerDerTest, ValidateIntegerDer_MultipleLeadingZeros_Error_UT_DER_019) {
    auto r = encoder_.validate_integer_der(std::span<const uint8_t>({0x00, 0x00, 0x7F}));
    EXPECT_TRUE(r.is_err());
}

TEST_F(IntegerDerTest, ValidateIntegerDer_LeadingFFNecessaryForNegative_OK_UT_DER_020) {
    // 0xFF, 0x7F is minimal encoding of -129: the 0xFF is needed because 0x7F has MSB clear
    auto r = encoder_.validate_integer_der(std::span<const uint8_t>({0xFF, 0x7F}));
    EXPECT_TRUE(r.is_ok());
}

TEST_F(IntegerDerTest, ValidateIntegerDer_MultipleLeadingFF_Error_UT_DER_021) {
    auto r = encoder_.validate_integer_der(std::span<const uint8_t>({0xFF, 0xFF, 0x80}));
    EXPECT_TRUE(r.is_err());
}

TEST_F(IntegerDerTest, ValidateIntegerDer_RequiredLeadingFF_OK_UT_DER_022) {
    auto r = encoder_.validate_integer_der(std::span<const uint8_t>({0xFF, 0x00}));
    EXPECT_TRUE(r.is_ok());
}

TEST_F(IntegerDerTest, ValidateIntegerDer_RequiredLeadingZero_OK_UT_DER_023) {
    auto r = encoder_.validate_integer_der(std::span<const uint8_t>({0x00, 0x80}));
    EXPECT_TRUE(r.is_ok());
}

TEST_F(IntegerDerTest, ValidateIntegerDer_Empty_Error_UT_DER_024) {
    auto r = encoder_.validate_integer_der(std::span<const uint8_t>({}));
    EXPECT_TRUE(r.is_err());
}

class LengthDerTest : public ::testing::Test {
protected:
    der_encoder encoder_;
};

TEST_F(LengthDerTest, ValidateDefiniteLength_OK_UT_DER_025) {
    std::vector<uint8_t> tlv = {0x02, 0x01, 0x00};
    auto r = encoder_.validate_length_definite(tlv);
    EXPECT_TRUE(r.is_ok());
}

TEST_F(LengthDerTest, ValidateDefiniteLength_MultiOctet_OK_UT_DER_026) {
    std::vector<uint8_t> tlv = {0x02, 0x82, 0x01, 0x00};
    auto r = encoder_.validate_length_definite(tlv);
    EXPECT_TRUE(r.is_ok());
}

TEST_F(LengthDerTest, ValidateIndefiniteLength_Error_UT_DER_027) {
    std::vector<uint8_t> tlv = {0x02, 0x80};
    auto r = encoder_.validate_length_definite(tlv);
    EXPECT_TRUE(r.is_err());
}

TEST_F(LengthDerTest, ValidateEmpty_Error_UT_DER_028) {
    std::vector<uint8_t> tlv = {};
    auto r = encoder_.validate_length_definite(tlv);
    EXPECT_TRUE(r.is_err());
}

class BitStringDerTest : public ::testing::Test {
protected:
    der_encoder encoder_;
};

TEST_F(BitStringDerTest, EncodeBitString_UT_DER_029) {
    std::vector<uint8_t> data = {0x80};
    std::vector<uint8_t> buf(32);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_bit_string(data, 0, view);
    ASSERT_TRUE(r.is_ok());
}

TEST_F(BitStringDerTest, EncodeBitString_UnusedBits_OK_UT_DER_030) {
    std::vector<uint8_t> data = {0xF0};
    std::vector<uint8_t> buf(32);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_bit_string(data, 4, view);
    ASSERT_TRUE(r.is_ok());
}

TEST_F(BitStringDerTest, ValidateUnusedBits_Zero_OK_UT_DER_031) {
    const uint8_t data[] = {0x00, 0xF0};
    std::span<const uint8_t> content(data, 2);
    auto r = encoder_.validate_bit_string_unused_bits(content);
    EXPECT_TRUE(r.is_ok());
}

TEST_F(BitStringDerTest, ValidateUnusedBits_NonZeroInUnusedBits_Error_UT_DER_032) {
    const uint8_t data[] = {0x03, 0xF8};
    std::span<const uint8_t> content(data, 2);
    auto r = encoder_.validate_bit_string_unused_bits(content);
    EXPECT_TRUE(r.is_err());
}

TEST_F(BitStringDerTest, ValidateUnusedBits_InvalidUnusedBitsValue_Error_UT_DER_033) {
    const uint8_t data[] = {0x08, 0x00};
    std::span<const uint8_t> content(data, 2);
    auto r = encoder_.validate_bit_string_unused_bits(content);
    EXPECT_TRUE(r.is_err());
}

TEST_F(BitStringDerTest, ValidateUnusedBits_EmptyContent_Error_UT_DER_034) {
    std::span<const uint8_t> content = {};
    auto r = encoder_.validate_bit_string_unused_bits(content);
    EXPECT_TRUE(r.is_err());
}

class OctetStringDerTest : public ::testing::Test {
protected:
    der_encoder encoder_;
};

TEST_F(OctetStringDerTest, EncodeOctetString_UT_DER_035) {
    std::vector<uint8_t> data = {'h', 'e', 'l', 'l', 'o'};
    std::vector<uint8_t> buf(32);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_octet_string(data, view);
    ASSERT_TRUE(r.is_ok());
}

TEST_F(OctetStringDerTest, ValidatePrimitiveString_OK_UT_DER_036) {
    std::vector<uint8_t> tlv = {0x04, 0x05, 'h', 'e', 'l', 'l', 'o'};
    auto r = encoder_.validate_primitive_string(tlv);
    EXPECT_TRUE(r.is_ok());
}

TEST_F(OctetStringDerTest, ValidatePrimitiveString_Constructed_Error_UT_DER_037) {
    std::vector<uint8_t> tlv = {0x24, 0x80};
    auto r = encoder_.validate_primitive_string(tlv);
    EXPECT_TRUE(r.is_err());
}

class NullDerTest : public ::testing::Test {
protected:
    der_encoder encoder_;
};

TEST_F(NullDerTest, EncodeNull_UT_DER_038) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_null(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 2), std::vector<uint8_t>({0x05, 0x00}));
}

class EnumeratedDerTest : public ::testing::Test {
protected:
    der_encoder encoder_;
};

TEST_F(EnumeratedDerTest, EncodeEnumerated_UT_DER_039) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_enumerated(0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x0A, 0x01, 0x00}));
}

TEST_F(EnumeratedDerTest, EncodeEnumerated_Negative_UT_DER_040) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_enumerated(-1, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x0A, 0x01, 0xFF}));
}

class ValidateFullTlvTest : public ::testing::Test {
protected:
    der_encoder encoder_;
};

TEST_F(ValidateFullTlvTest, ValidateValidDer_UT_DER_041) {
    std::vector<uint8_t> der = {0x02, 0x01, 0x00};
    auto r = encoder_.validate(der);
    EXPECT_TRUE(r.is_ok());
}

TEST_F(ValidateFullTlvTest, ValidateIndefiniteLength_Error_UT_DER_042) {
    std::vector<uint8_t> der = {0x02, 0x80};
    auto r = encoder_.validate(der);
    EXPECT_TRUE(r.is_err());
}

TEST_F(ValidateFullTlvTest, ValidateNonUniversalTag_Skipped_UT_DER_043) {
    std::vector<uint8_t> der = {0xA0, 0x01, 0x00};
    auto r = encoder_.validate(der);
    EXPECT_TRUE(r.is_ok());
}

TEST_F(ValidateFullTlvTest, ValidateOctetStringPrimitive_OK_UT_DER_044) {
    std::vector<uint8_t> der = {0x04, 0x03, 0x01, 0x02, 0x03};
    auto r = encoder_.validate(der);
    EXPECT_TRUE(r.is_ok());
}

TEST_F(ValidateFullTlvTest, ValidateOctetStringConstructed_Error_UT_DER_045) {
    std::vector<uint8_t> der = {0x24, 0x80};
    auto r = encoder_.validate(der);
    EXPECT_TRUE(r.is_err());
}

class RoundTripDerTest : public ::testing::Test {
protected:
    der_encoder encoder_;
    ber_decoder decoder_;
};

TEST_F(RoundTripDerTest, RoundTripBoolean_UT_DER_046) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    ASSERT_TRUE(encoder_.encode_boolean(true, view).is_ok());

    size_t encoded = 16 - view.size();
    std::vector<uint8_t> encoded_data(buf.begin(), buf.begin() + static_cast<long>(encoded));
    ASSERT_TRUE(encoder_.validate(encoded_data).is_ok());

    buffer_view decode_view(encoded_data);
    auto r = decoder_.decode_boolean(decode_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_TRUE(r.value());
}

TEST_F(RoundTripDerTest, RoundTripInteger_UT_DER_047) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    ASSERT_TRUE(encoder_.encode_integer(42, view).is_ok());

    size_t encoded = 16 - view.size();
    std::vector<uint8_t> encoded_data(buf.begin(), buf.begin() + static_cast<long>(encoded));
    ASSERT_TRUE(encoder_.validate(encoded_data).is_ok());

    buffer_view decode_view(encoded_data);
    auto r = decoder_.decode_integer(decode_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 42);
}

TEST_F(RoundTripDerTest, RoundTripNegativeInteger_UT_DER_048) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    ASSERT_TRUE(encoder_.encode_integer(-128, view).is_ok());

    size_t encoded = 16 - view.size();
    std::vector<uint8_t> encoded_data(buf.begin(), buf.begin() + static_cast<long>(encoded));
    ASSERT_TRUE(encoder_.validate(encoded_data).is_ok());

    buffer_view decode_view(encoded_data);
    auto r = decoder_.decode_integer(decode_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), -128);
}