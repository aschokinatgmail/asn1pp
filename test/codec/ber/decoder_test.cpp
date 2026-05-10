#include <gtest/gtest.h>

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>
#include <utility>

#include "codec/traits.hpp"
#include "codec/result.hpp"
#include "codec/ber/tlv.hpp"
#include "codec/ber/decoder.hpp"
#include "codec/ber/encoder.hpp"

using namespace asn1pp;
using namespace asn1pp::ber;

namespace {

buffer_view make_const_view(const std::vector<uint8_t>& vec) {
    return buffer_view(vec.data(), vec.size());
}

} // namespace

// ============================================================================
// 1. INTEGER decoding
// ============================================================================

class IntegerDecodingTest : public ::testing::Test {
protected:
    ber_decoder decoder_;
};

TEST_F(IntegerDecodingTest, DecodeZero_UT_BER_DEC_001) {
    std::vector<uint8_t> data = {0x02, 0x01, 0x00};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0);
}

TEST_F(IntegerDecodingTest, Decode127_UT_BER_DEC_002) {
    std::vector<uint8_t> data = {0x02, 0x01, 0x7F};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 127);
}

TEST_F(IntegerDecodingTest, Decode128_UT_BER_DEC_003) {
    std::vector<uint8_t> data = {0x02, 0x02, 0x00, 0x80};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 128);
}

TEST_F(IntegerDecodingTest, DecodeNegative1_UT_BER_DEC_004) {
    std::vector<uint8_t> data = {0x02, 0x01, 0xFF};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), -1);
}

TEST_F(IntegerDecodingTest, DecodeNegative128_UT_BER_DEC_005) {
    std::vector<uint8_t> data = {0x02, 0x01, 0x80};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), -128);
}

TEST_F(IntegerDecodingTest, DecodeNegative129_UT_BER_DEC_006) {
    std::vector<uint8_t> data = {0x02, 0x02, 0xFF, 0x7F};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), -129);
}

TEST_F(IntegerDecodingTest, Decode256_UT_BER_DEC_007) {
    std::vector<uint8_t> data = {0x02, 0x02, 0x01, 0x00};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 256);
}

TEST_F(IntegerDecodingTest, Decode255_UT_BER_DEC_008) {
    std::vector<uint8_t> data = {0x02, 0x02, 0x00, 0xFF};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 255);
}

TEST_F(IntegerDecodingTest, DecodeMaxInt32_UT_BER_DEC_009) {
    std::vector<uint8_t> data = {0x02, 0x04, 0x7F, 0xFF, 0xFF, 0xFF};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0x7FFFFFFF);
}

TEST_F(IntegerDecodingTest, DecodeMinInt32_UT_BER_DEC_010) {
    std::vector<uint8_t> data = {0x02, 0x04, 0x80, 0x00, 0x00, 0x00};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), static_cast<int64_t>(static_cast<int32_t>(0x80000000)));
}

TEST_F(IntegerDecodingTest, DecodeInt64Max_UT_BER_DEC_011) {
    std::vector<uint8_t> data = {0x02, 0x08, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), INT64_MAX);
}

TEST_F(IntegerDecodingTest, DecodeInt64Min_UT_BER_DEC_012) {
    std::vector<uint8_t> data = {0x02, 0x08, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), INT64_MIN);
}

TEST_F(IntegerDecodingTest, DecodeLargePositive_UT_BER_DEC_013) {
    std::vector<uint8_t> data = {0x02, 0x03, 0x01, 0x23, 0x45};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0x012345);
}

TEST_F(IntegerDecodingTest, DecodeLargeNegative_UT_BER_DEC_014) {
    std::vector<uint8_t> data = {0x02, 0x03, 0xFE, 0xDC, 0xBB};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), -74565);
}

// ============================================================================
// 2. BOOLEAN decoding
// ============================================================================

class BooleanDecodingTest : public ::testing::Test {
protected:
    ber_decoder decoder_;
};

TEST_F(BooleanDecodingTest, DecodeTrue_UT_BER_DEC_015) {
    std::vector<uint8_t> data = {0x01, 0x01, 0xFF};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_boolean(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), true);
}

TEST_F(BooleanDecodingTest, DecodeFalse_UT_BER_DEC_016) {
    std::vector<uint8_t> data = {0x01, 0x01, 0x00};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_boolean(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), false);
}

TEST_F(BooleanDecodingTest, DecodeNonZeroAsTrue_UT_BER_DEC_017) {
    std::vector<uint8_t> data = {0x01, 0x01, 0x42};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_boolean(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), true);
}

// ============================================================================
// 3. NULL decoding
// ============================================================================

class NullDecodingTest : public ::testing::Test {
protected:
    ber_decoder decoder_;
};

TEST_F(NullDecodingTest, DecodeNull_UT_BER_DEC_018) {
    std::vector<uint8_t> data = {0x05, 0x00};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_null(view);
    EXPECT_TRUE(r.is_ok());
}

// ============================================================================
// 4. OCTET STRING decoding
// ============================================================================

class OctetStringDecodingTest : public ::testing::Test {
protected:
    ber_decoder decoder_;
};

TEST_F(OctetStringDecodingTest, DecodeEmpty_UT_BER_DEC_019) {
    std::vector<uint8_t> data = {0x04, 0x00};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_octet_string(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_TRUE(r.value().empty());
}

TEST_F(OctetStringDecodingTest, DecodeTestBytes_UT_BER_DEC_020) {
    std::vector<uint8_t> data = {0x04, 0x04, 0x74, 0x65, 0x73, 0x74};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_octet_string(view);
    ASSERT_TRUE(r.is_ok());
    std::vector<uint8_t> expected = {'t', 'e', 's', 't'};
    EXPECT_EQ(r.value(), expected);
}

TEST_F(OctetStringDecodingTest, DecodeLongFormLength_UT_BER_DEC_021) {
    const size_t N = 200;
    std::vector<uint8_t> data;
    data.push_back(0x04);
    data.push_back(0x81);
    data.push_back(0xC8);
    for (size_t i = 0; i < N; ++i)
        data.push_back(static_cast<uint8_t>(i & 0xFF));

    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_octet_string(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().size(), N);
    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ(r.value()[i], static_cast<uint8_t>(i & 0xFF));
}

// ============================================================================
// 5. BIT STRING decoding
// ============================================================================

class BitStringDecodingTest : public ::testing::Test {
protected:
    ber_decoder decoder_;
};

TEST_F(BitStringDecodingTest, DecodeWithUnusedBits_UT_BER_DEC_022) {
    std::vector<uint8_t> data = {0x03, 0x02, 0x07, 0x80};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_bit_string(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().second, 7);
    ASSERT_EQ(r.value().first.size(), 1u);
    EXPECT_EQ(r.value().first[0], 0x80);
}

TEST_F(BitStringDecodingTest, DecodeFullByteNoUnused_UT_BER_DEC_023) {
    std::vector<uint8_t> data = {0x03, 0x02, 0x00, 0xFF};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_bit_string(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().second, 0);
    ASSERT_EQ(r.value().first.size(), 1u);
    EXPECT_EQ(r.value().first[0], 0xFF);
}

TEST_F(BitStringDecodingTest, DecodeEmpty_UT_BER_DEC_024) {
    std::vector<uint8_t> data = {0x03, 0x01, 0x00};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_bit_string(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().second, 0);
    EXPECT_TRUE(r.value().first.empty());
}

TEST_F(BitStringDecodingTest, DecodeMultiByteWithUnused_UT_BER_DEC_025) {
    std::vector<uint8_t> data = {0x03, 0x03, 0x04, 0x0A, 0x3B};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_bit_string(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().second, 4);
    ASSERT_EQ(r.value().first.size(), 2u);
    EXPECT_EQ(r.value().first[0], 0x0A);
    EXPECT_EQ(r.value().first[1], 0x3B);
}

// ============================================================================
// 6. ENUMERATED decoding
// ============================================================================

class EnumeratedDecodingTest : public ::testing::Test {
protected:
    ber_decoder decoder_;
};

TEST_F(EnumeratedDecodingTest, DecodeZero_UT_BER_DEC_026) {
    std::vector<uint8_t> data = {0x0A, 0x01, 0x00};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_enumerated(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0);
}

TEST_F(EnumeratedDecodingTest, Decode42_UT_BER_DEC_027) {
    std::vector<uint8_t> data = {0x0A, 0x01, 0x2A};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_enumerated(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 42);
}

TEST_F(EnumeratedDecodingTest, DecodeLarge_UT_BER_DEC_028) {
    std::vector<uint8_t> data = {0x0A, 0x02, 0x03, 0xE8};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_enumerated(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 1000);
}

// ============================================================================
// 7. SEQUENCE decoding
// ============================================================================

class SequenceDecodingTest : public ::testing::Test {
protected:
    ber_decoder decoder_;
};

TEST_F(SequenceDecodingTest, DecodeEmptySequence_UT_BER_DEC_029) {
    std::vector<uint8_t> data = {0x30, 0x00};
    buffer_view view = make_const_view(data);
    size_t content_length = 0;
    auto r = decoder_.decode_sequence_header(view, content_length);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(content_length, 0u);
    EXPECT_EQ(r.value(), make_universal(universal_tag::sequence, true));
}

TEST_F(SequenceDecodingTest, DecodeSequenceWithOneInteger_UT_BER_DEC_030) {
    std::vector<uint8_t> data = {0x30, 0x03, 0x02, 0x01, 0x2A};
    buffer_view view = make_const_view(data);
    size_t content_length = 0;
    auto r = decoder_.decode_sequence_header(view, content_length);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(content_length, 3u);

    auto r_int = decoder_.decode_integer(view);
    ASSERT_TRUE(r_int.is_ok());
    EXPECT_EQ(r_int.value(), 42);
}

TEST_F(SequenceDecodingTest, DecodeSequenceWithBoolAndInt_UT_BER_DEC_031) {
    std::vector<uint8_t> data = {
        0x30, 0x06,
        0x01, 0x01, 0xFF,
        0x02, 0x01, 0x05
    };
    buffer_view view = make_const_view(data);
    size_t content_length = 0;
    auto r = decoder_.decode_sequence_header(view, content_length);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(content_length, 6u);

    auto r_bool = decoder_.decode_boolean(view);
    ASSERT_TRUE(r_bool.is_ok());
    EXPECT_EQ(r_bool.value(), true);

    auto r_int = decoder_.decode_integer(view);
    ASSERT_TRUE(r_int.is_ok());
    EXPECT_EQ(r_int.value(), 5);
}

TEST_F(SequenceDecodingTest, DecodeSequenceWithOctetString_UT_BER_DEC_032) {
    std::vector<uint8_t> data = {
        0x30, 0x04,
        0x04, 0x02, 'A', 'B'
    };
    buffer_view view = make_const_view(data);
    size_t content_length = 0;
    auto r = decoder_.decode_sequence_header(view, content_length);
    ASSERT_TRUE(r.is_ok());

    auto r_oct = decoder_.decode_octet_string(view);
    ASSERT_TRUE(r_oct.is_ok());
    std::vector<uint8_t> expected = {'A', 'B'};
    EXPECT_EQ(r_oct.value(), expected);
}

TEST_F(SequenceDecodingTest, DecodeNestedSequence_UT_BER_DEC_033) {
    std::vector<uint8_t> data = {
        0x30, 0x05,
        0x30, 0x03,
        0x02, 0x01, 0x07
    };
    buffer_view view = make_const_view(data);
    size_t outer_len = 0;
    auto r_outer = decoder_.decode_sequence_header(view, outer_len);
    ASSERT_TRUE(r_outer.is_ok());

    size_t inner_len = 0;
    auto r_inner = decoder_.decode_sequence_header(view, inner_len);
    ASSERT_TRUE(r_inner.is_ok());

    auto r_int = decoder_.decode_integer(view);
    ASSERT_TRUE(r_int.is_ok());
    EXPECT_EQ(r_int.value(), 7);
}

// ============================================================================
// 8. OID decoding
// ============================================================================

class OidDecodingTest : public ::testing::Test {
protected:
    ber_decoder decoder_;
};

TEST_F(OidDecodingTest, DecodePreencodedOid_UT_BER_DEC_034) {
    std::vector<uint8_t> data = {0x06, 0x06, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_oid(view);
    ASSERT_TRUE(r.is_ok());
    std::vector<uint8_t> expected = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D};
    EXPECT_EQ(r.value(), expected);
}

TEST_F(OidDecodingTest, DecodeEmptyOid_UT_BER_DEC_035) {
    std::vector<uint8_t> data = {0x06, 0x00};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_oid(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_TRUE(r.value().empty());
}

// ============================================================================
// 9. Error handling
// ============================================================================

class DecoderErrorTest : public ::testing::Test {
protected:
    ber_decoder decoder_;
};

TEST_F(DecoderErrorTest, TruncatedInputInteger_UT_BER_DEC_036) {
    std::vector<uint8_t> data = {0x02};  // tag only, no length or value
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::buffer_underflow);
}

TEST_F(DecoderErrorTest, TruncatedInputBoolean_UT_BER_DEC_037) {
    std::vector<uint8_t> data = {0x01, 0x01};  // tag + length, no value
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_boolean(view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::buffer_underflow);
}

TEST_F(DecoderErrorTest, WrongTagForInteger_UT_BER_DEC_038) {
    std::vector<uint8_t> data = {0x01, 0x01, 0xFF};  // boolean tag instead of integer
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::invalid_tag);
}

TEST_F(DecoderErrorTest, WrongTagForBoolean_UT_BER_DEC_039) {
    std::vector<uint8_t> data = {0x02, 0x01, 0x00};  // integer tag instead of boolean
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_boolean(view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::invalid_tag);
}

TEST_F(DecoderErrorTest, WrongTagForNull_UT_BER_DEC_040) {
    std::vector<uint8_t> data = {0x02, 0x01, 0x00};  // integer tag instead of null
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_null(view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::invalid_tag);
}

TEST_F(DecoderErrorTest, WrongTagForOctetString_UT_BER_DEC_041) {
    std::vector<uint8_t> data = {0x02, 0x01, 0x00};  // integer tag instead of octet string
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_octet_string(view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::invalid_tag);
}

TEST_F(DecoderErrorTest, IntegerValueTooLarge_UT_BER_DEC_042) {
    std::vector<uint8_t> data = {0x02, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::value_out_of_range);
}

TEST_F(DecoderErrorTest, NonConstructedSequence_UT_BER_DEC_043) {
    std::vector<uint8_t> data = {0x10, 0x00};  // tag 16 but not constructed
    buffer_view view = make_const_view(data);
    size_t content_length = 0;
    auto r = decoder_.decode_sequence_header(view, content_length);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::invalid_tag);
}

TEST_F(DecoderErrorTest, BufferOverflowSequence_UT_BER_DEC_044) {
    std::vector<uint8_t> data = {0x30, 0x10, 0x01, 0x01, 0xFF};  // claims 16 bytes but only 3 follow
    buffer_view view = make_const_view(data);
    size_t content_length = 0;
    auto r = decoder_.decode_sequence_header(view, content_length);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::buffer_underflow);
}

// ============================================================================
// 10. Generic TLV decode
// ============================================================================

class GenericTlvDecodeTest : public ::testing::Test {
protected:
    ber_decoder decoder_;
};

TEST_F(GenericTlvDecodeTest, DecodeTlvInteger_UT_BER_DEC_045) {
    std::vector<uint8_t> data = {0x02, 0x01, 0x2A};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_tlv(view);
    ASSERT_TRUE(r.is_ok());
    const auto& tlv = r.value();
    EXPECT_EQ(tlv.t, make_universal(universal_tag::integer));
    EXPECT_EQ(tlv.length, 1u);
    ASSERT_EQ(tlv.value.size(), 1u);
    EXPECT_EQ(tlv.value[0], 0x2A);
}

TEST_F(GenericTlvDecodeTest, DecodeTlvContextSpecific_UT_BER_DEC_046) {
    std::vector<uint8_t> data = {0xA1, 0x02, 0xAA, 0xBB};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_tlv(view);
    ASSERT_TRUE(r.is_ok());
    const auto& tlv = r.value();
    EXPECT_EQ(tlv.t, make_context_specific(1, true));
    EXPECT_EQ(tlv.length, 2u);
    ASSERT_EQ(tlv.value.size(), 2u);
    EXPECT_EQ(tlv.value[0], 0xAA);
    EXPECT_EQ(tlv.value[1], 0xBB);
}

// ============================================================================
// 11. Skip TLV
// ============================================================================

class SkipTlvTest : public ::testing::Test {
protected:
    ber_decoder decoder_;
};

TEST_F(SkipTlvTest, SkipInteger_UT_BER_DEC_047) {
    std::vector<uint8_t> data = {0x02, 0x01, 0x2A,  0x02, 0x01, 0x07};
    buffer_view view = make_const_view(data);
    auto r = decoder_.skip_tlv(view);
    ASSERT_TRUE(r.is_ok());
    auto r_int = decoder_.decode_integer(view);
    ASSERT_TRUE(r_int.is_ok());
    EXPECT_EQ(r_int.value(), 7);
}

TEST_F(SkipTlvTest, SkipOctetString_UT_BER_DEC_048) {
    std::vector<uint8_t> data = {0x04, 0x04, 0x74, 0x65, 0x73, 0x74,  0x05, 0x00};
    buffer_view view = make_const_view(data);
    auto r = decoder_.skip_tlv(view);
    ASSERT_TRUE(r.is_ok());
    auto r_null = decoder_.decode_null(view);
    EXPECT_TRUE(r_null.is_ok());
}

// ============================================================================
// 12. Peek tag
// ============================================================================

class PeekTagTest : public ::testing::Test {
protected:
    ber_decoder decoder_;
};

TEST_F(PeekTagTest, PeekIntegerTag_UT_BER_DEC_049) {
    std::vector<uint8_t> data = {0x02, 0x01, 0x2A};
    buffer_view view = make_const_view(data);
    auto r = decoder_.peek_tag(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), make_universal(universal_tag::integer));
    EXPECT_EQ(view.size(), 3u);  // view not advanced
}

TEST_F(PeekTagTest, PeekThenDecode_UT_BER_DEC_050) {
    std::vector<uint8_t> data = {0x01, 0x01, 0xFF};
    buffer_view view = make_const_view(data);
    auto peek = decoder_.peek_tag(view);
    ASSERT_TRUE(peek.is_ok());
    EXPECT_EQ(peek.value(), make_universal(universal_tag::boolean));

    auto decode = decoder_.decode_boolean(view);
    ASSERT_TRUE(decode.is_ok());
    EXPECT_EQ(decode.value(), true);
}

// ============================================================================
// 13. End-of-content decoding
// ============================================================================

class EndOfContentTest : public ::testing::Test {
protected:
    ber_decoder decoder_;
};

TEST_F(EndOfContentTest, DecodeValidEoc_UT_BER_DEC_051) {
    std::vector<uint8_t> data = {0x00, 0x00};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_sequence_end(view);
    EXPECT_TRUE(r.is_ok());
    EXPECT_TRUE(view.empty());
}

TEST_F(EndOfContentTest, DecodeNonEocBytes_UT_BER_DEC_052) {
    std::vector<uint8_t> data = {0x01, 0x01};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_sequence_end(view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::invalid_tag);
}

TEST_F(EndOfContentTest, DecodeTruncatedEoc_UT_BER_DEC_053) {
    std::vector<uint8_t> data = {0x00};
    buffer_view view = make_const_view(data);
    auto r = decoder_.decode_sequence_end(view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::buffer_underflow);
}

// ============================================================================
// 14. Round-trip tests (encode → decode → original value)
// ============================================================================

class RoundTripTest : public ::testing::Test {
protected:
    ber_encoder encoder_;
    ber_decoder decoder_;
};

TEST_F(RoundTripTest, IntegerRoundTrip_Zero_UT_BER_DEC_054) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_integer(0, enc_view).is_ok());
    const size_t used = 16 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_integer(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0);
}

TEST_F(RoundTripTest, IntegerRoundTrip_127_UT_BER_DEC_055) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_integer(127, enc_view).is_ok());
    const size_t used = 16 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_integer(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 127);
}

TEST_F(RoundTripTest, IntegerRoundTrip_128_UT_BER_DEC_056) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_integer(128, enc_view).is_ok());
    const size_t used = 16 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_integer(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 128);
}

TEST_F(RoundTripTest, IntegerRoundTrip_Neg1_UT_BER_DEC_057) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_integer(-1, enc_view).is_ok());
    const size_t used = 16 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_integer(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), -1);
}

TEST_F(RoundTripTest, IntegerRoundTrip_Neg128_UT_BER_DEC_058) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_integer(-128, enc_view).is_ok());
    const size_t used = 16 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_integer(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), -128);
}

TEST_F(RoundTripTest, IntegerRoundTrip_Neg129_UT_BER_DEC_059) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_integer(-129, enc_view).is_ok());
    const size_t used = 16 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_integer(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), -129);
}

TEST_F(RoundTripTest, IntegerRoundTrip_256_UT_BER_DEC_060) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_integer(256, enc_view).is_ok());
    const size_t used = 16 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_integer(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 256);
}

TEST_F(RoundTripTest, IntegerRoundTrip_255_UT_BER_DEC_061) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_integer(255, enc_view).is_ok());
    const size_t used = 16 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_integer(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 255);
}

TEST_F(RoundTripTest, IntegerRoundTrip_MaxInt32_UT_BER_DEC_062) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_integer(0x7FFFFFFF, enc_view).is_ok());
    const size_t used = 16 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_integer(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0x7FFFFFFF);
}

TEST_F(RoundTripTest, IntegerRoundTrip_MinInt32_UT_BER_DEC_063) {
    const auto min_int32 = static_cast<int64_t>(static_cast<int32_t>(0x80000000));
    std::vector<uint8_t> buf(16);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_integer(min_int32, enc_view).is_ok());
    const size_t used = 16 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_integer(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), min_int32);
}

TEST_F(RoundTripTest, IntegerRoundTrip_Int64Max_UT_BER_DEC_064) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_integer(INT64_MAX, enc_view).is_ok());
    const size_t used = 16 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_integer(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), INT64_MAX);
}

TEST_F(RoundTripTest, IntegerRoundTrip_Int64Min_UT_BER_DEC_065) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_integer(INT64_MIN, enc_view).is_ok());
    const size_t used = 16 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_integer(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), INT64_MIN);
}

TEST_F(RoundTripTest, BooleanRoundTrip_True_UT_BER_DEC_066) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_boolean(true, enc_view).is_ok());
    const size_t used = 16 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_boolean(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), true);
}

TEST_F(RoundTripTest, BooleanRoundTrip_False_UT_BER_DEC_067) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_boolean(false, enc_view).is_ok());
    const size_t used = 16 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_boolean(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), false);
}

TEST_F(RoundTripTest, NullRoundTrip_UT_BER_DEC_068) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_null(enc_view).is_ok());
    const size_t used = 16 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_null(dec_view);
    EXPECT_TRUE(r.is_ok());
}

TEST_F(RoundTripTest, OctetStringRoundTrip_UT_BER_DEC_069) {
    const std::vector<uint8_t> original = {'h', 'e', 'l', 'l', 'o'};
    std::vector<uint8_t> buf(32);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_octet_string(original, enc_view).is_ok());
    const size_t used = 32 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_octet_string(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), original);
}

TEST_F(RoundTripTest, OidRoundTrip_UT_BER_DEC_070) {
    const std::vector<uint8_t> original = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D};
    std::vector<uint8_t> buf(32);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_oid(original, enc_view).is_ok());
    const size_t used = 32 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_oid(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), original);
}

TEST_F(RoundTripTest, EnumeratedRoundTrip_42_UT_BER_DEC_071) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view = buffer_view(buf);
    ASSERT_TRUE(encoder_.encode_enumerated(42, enc_view).is_ok());
    const size_t used = 16 - enc_view.size();

    buffer_view dec_view(buf.data(), used);
    auto r = decoder_.decode_enumerated(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 42);
}
