#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>
#include <cstring>

#include "codec/result.hpp"
#include "codec/per/encoder.hpp"
#include "codec/per/decoder.hpp"

using namespace asn1pp;
using namespace asn1pp::per;

// ============================================================================
// PER Metadata structs (same as encoder tests)
// ============================================================================

namespace {

struct uint8_range_meta {
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 255;
    static constexpr bool has_range_constraint = true;
    static constexpr bool is_extension_permitted = false;
};

struct uint16_range_meta {
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 65535;
    static constexpr bool has_range_constraint = true;
    static constexpr bool is_extension_permitted = false;
};

struct small_range_meta {
    static constexpr int64_t min_value = 1;
    static constexpr int64_t max_value = 10;
    static constexpr bool has_range_constraint = true;
    static constexpr bool is_extension_permitted = false;
};

struct bit_range_meta {
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 1;
    static constexpr bool has_range_constraint = true;
    static constexpr bool is_extension_permitted = false;
};

struct unconstrained_int_meta {
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 9223372036854775807LL;
    static constexpr bool has_range_constraint = false;
    static constexpr bool is_extension_permitted = false;
};

struct fixed_octets_meta {
    static constexpr size_t min_size = 4;
    static constexpr size_t max_size = 4;
    static constexpr bool has_size_constraint = true;
    static constexpr bool is_extension_permitted = false;
};

struct constrained_octets_meta {
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 255;
    static constexpr bool has_size_constraint = true;
    static constexpr bool is_extension_permitted = false;
};

struct unconstrained_octets_meta {
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 18446744073709551615ULL;
    static constexpr bool has_size_constraint = false;
    static constexpr bool is_extension_permitted = false;
};

struct color_enum_meta {
    static constexpr size_t normal_index_count = 3;
    static constexpr bool has_extension = false;
};

struct single_val_enum_meta {
    static constexpr size_t normal_index_count = 1;
    static constexpr bool has_extension = false;
};

struct seq_one_optional_meta {
    static constexpr size_t field_count = 2;
    static constexpr bool optional_bitmap[2] = {false, true};
    static constexpr bool has_extension = false;
};

struct seq_with_ext_meta {
    static constexpr size_t field_count = 1;
    static constexpr bool optional_bitmap[1] = {false};
    static constexpr bool has_extension = true;
};

struct seq_ext_optional_meta {
    static constexpr size_t field_count = 2;
    static constexpr bool optional_bitmap[2] = {false, true};
    static constexpr bool has_extension = true;
};

struct seq_plain_meta {
    static constexpr size_t field_count = 2;
    static constexpr bool optional_bitmap[2] = {false, false};
    static constexpr bool has_extension = false;
};

struct choice_4_meta {
    static constexpr size_t alternative_count = 4;
    static constexpr bool has_extension = false;
};

struct seqof_256_meta {
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 255;
    static constexpr bool has_size_constraint = true;
};

struct seqof_64k_meta {
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 65535;
    static constexpr bool has_size_constraint = true;
};

struct seqof_unconstrained_meta {
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 18446744073709551615ULL;
    static constexpr bool has_size_constraint = false;
};

struct three_field_two_optional_meta {
    static constexpr size_t field_count = 3;
    static constexpr bool optional_bitmap[3] = {false, true, true};
    static constexpr bool has_extension = false;
};

struct fixed_bit8_meta {
    static constexpr size_t min_size = 8;
    static constexpr size_t max_size = 8;
    static constexpr bool has_size_constraint = true;
};

struct fixed_bit10_meta {
    static constexpr size_t min_size = 10;
    static constexpr size_t max_size = 10;
    static constexpr bool has_size_constraint = true;
};

struct constrained_bit_meta {
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 255;
    static constexpr bool has_size_constraint = true;
};

struct large_range_meta {
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 100000;
    static constexpr bool has_range_constraint = true;
};

// ============================================================================
// Helpers
// ============================================================================

std::vector<uint8_t> make_mutable_buffer(size_t cap) {
    return std::vector<uint8_t>(cap, 0);
}

buffer_view make_view(std::vector<uint8_t>& vec) {
    return buffer_view(vec);
}

buffer_view make_const_view(const std::vector<uint8_t>& vec) {
    return buffer_view(vec);
}

} // namespace

// ============================================================================
// 1. bits_needed_for_range / bits_needed_unsigned
// ============================================================================

TEST(DecoderBitsNeededTest, Range0_255_UT_PER_DEC_001) {
    EXPECT_EQ(per_aligned_decoder::bits_needed_for_range(0, 255), 8);
}

TEST(DecoderBitsNeededTest, Range0_65535_UT_PER_DEC_002) {
    EXPECT_EQ(per_aligned_decoder::bits_needed_for_range(0, 65535), 16);
}

TEST(DecoderBitsNeededTest, Range1_10_UT_PER_DEC_003) {
    EXPECT_EQ(per_aligned_decoder::bits_needed_for_range(1, 10), 4);
}

TEST(DecoderBitsNeededTest, Range0_1_UT_PER_DEC_004) {
    EXPECT_EQ(per_aligned_decoder::bits_needed_for_range(0, 1), 1);
}

TEST(DecoderBitsNeededTest, Range0_0_UT_PER_DEC_005) {
    EXPECT_EQ(per_aligned_decoder::bits_needed_for_range(0, 0), 0);
}

TEST(DecoderBitsNeededTest, BitsNeededUnsigned_UT_PER_DEC_006) {
    EXPECT_EQ(per_aligned_decoder::bits_needed_unsigned(0), 0);
    EXPECT_EQ(per_aligned_decoder::bits_needed_unsigned(1), 1);
    EXPECT_EQ(per_aligned_decoder::bits_needed_unsigned(2), 2);
    EXPECT_EQ(per_aligned_decoder::bits_needed_unsigned(3), 2);
    EXPECT_EQ(per_aligned_decoder::bits_needed_unsigned(7), 3);
}

// ============================================================================
// 2. Align helper
// ============================================================================

TEST(DecoderAlignTest, AlreadyAligned_UT_PER_DEC_010) {
    size_t bit_off = 0;
    per_aligned_decoder::align(bit_off);
    EXPECT_EQ(bit_off, 0);
}

TEST(DecoderAlignTest, NotAligned_UT_PER_DEC_011) {
    size_t bit_off = 1;
    per_aligned_decoder::align(bit_off);
    EXPECT_EQ(bit_off, 8);
}

TEST(DecoderAlignTest, NotAlignedMid_UT_PER_DEC_012) {
    size_t bit_off = 10;
    per_aligned_decoder::align(bit_off);
    EXPECT_EQ(bit_off, 16);
}

// ============================================================================
// 3. Constrained whole number (round-trip with encoder)
// ============================================================================

TEST(ConstrainedWholeNumberRoundTrip, Range0_255_Value42_UT_PER_DEC_020) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t enc_bit_off = 0;

    per_aligned_encoder::encode_constrained_whole_number(42, 0, 255, view, enc_bit_off);

    size_t dec_bit_off = 0;
    auto dec_view = make_const_view(buf_vec);
    auto r = per_aligned_decoder::decode_constrained_whole_number(0, 255, dec_view, dec_bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 42);
    EXPECT_EQ(dec_bit_off, enc_bit_off);
}

TEST(ConstrainedWholeNumberRoundTrip, Range0_65535_Value1000_UT_PER_DEC_021) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t enc_bit_off = 0;

    per_aligned_encoder::encode_constrained_whole_number(1000, 0, 65535, view, enc_bit_off);

    size_t dec_bit_off = 0;
    auto dec_view = make_const_view(buf_vec);
    auto r = per_aligned_decoder::decode_constrained_whole_number(0, 65535, dec_view, dec_bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 1000);
}

TEST(ConstrainedWholeNumberRoundTrip, Range1_10_Value5_UT_PER_DEC_022) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t enc_bit_off = 0;

    per_aligned_encoder::encode_constrained_whole_number(5, 1, 10, view, enc_bit_off);

    size_t dec_bit_off = 0;
    auto dec_view = make_const_view(buf_vec);
    auto r = per_aligned_decoder::decode_constrained_whole_number(1, 10, dec_view, dec_bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 5);
}

TEST(ConstrainedWholeNumberRoundTrip, Range0_1_Value0_UT_PER_DEC_023) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t enc_bit_off = 0;

    per_aligned_encoder::encode_constrained_whole_number(0, 0, 1, view, enc_bit_off);

    size_t dec_bit_off = 0;
    auto dec_view = make_const_view(buf_vec);
    auto r = per_aligned_decoder::decode_constrained_whole_number(0, 1, dec_view, dec_bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0);
}

TEST(ConstrainedWholeNumberRoundTrip, Range0_1_Value1_UT_PER_DEC_024) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t enc_bit_off = 0;

    per_aligned_encoder::encode_constrained_whole_number(1, 0, 1, view, enc_bit_off);

    size_t dec_bit_off = 0;
    auto dec_view = make_const_view(buf_vec);
    auto r = per_aligned_decoder::decode_constrained_whole_number(0, 1, dec_view, dec_bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 1);
}

TEST(ConstrainedWholeNumberRoundTrip, Range0_0_SingleValue_UT_PER_DEC_025) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t enc_bit_off = 0;

    per_aligned_encoder::encode_constrained_whole_number(0, 0, 0, view, enc_bit_off);

    size_t dec_bit_off = 0;
    auto dec_view = make_const_view(buf_vec);
    auto r = per_aligned_decoder::decode_constrained_whole_number(0, 0, dec_view, dec_bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0);
}

// Direct decode from known byte patterns
TEST(ConstrainedWholeNumberDecode, FromKnownBytes_42_UT_PER_DEC_026) {
    std::vector<uint8_t> buf = {0x2A};  // 42 in 1 byte (range 0..255)
    auto view = make_const_view(buf);
    size_t bit_off = 0;
    auto r = per_aligned_decoder::decode_constrained_whole_number(0, 255, view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 42);
}

TEST(ConstrainedWholeNumberDecode, FromKnownBytes_1000_UT_PER_DEC_027) {
    std::vector<uint8_t> buf = {0x03, 0xE8};  // 1000 = 0x03E8 (range 0..65535)
    auto view = make_const_view(buf);
    size_t bit_off = 0;
    auto r = per_aligned_decoder::decode_constrained_whole_number(0, 65535, view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 1000);
}

TEST(ConstrainedWholeNumberDecode, BufferUnderflow_UT_PER_DEC_028) {
    std::vector<uint8_t> buf = {0x00};  // only 1 byte, need 2 for uint16 range
    auto view = make_const_view(buf);
    size_t bit_off = 0;
    auto r = per_aligned_decoder::decode_constrained_whole_number(0, 65535, view, bit_off);
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::buffer_underflow);
}

// ============================================================================
// 4. Length determinant (round-trip with encoder)
// ============================================================================

TEST(LengthDeterminantRoundTrip, Len0_UT_PER_DEC_030) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t enc_bit_off = 0;

    per_aligned_encoder::encode_length_determinant(0, view, enc_bit_off);

    size_t dec_bit_off = 0;
    auto dec_view = make_const_view(buf_vec);
    auto r = per_aligned_decoder::decode_length_determinant(dec_view, dec_bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0u);
}

TEST(LengthDeterminantRoundTrip, Len127_UT_PER_DEC_031) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t enc_bit_off = 0;

    per_aligned_encoder::encode_length_determinant(127, view, enc_bit_off);

    size_t dec_bit_off = 0;
    auto dec_view = make_const_view(buf_vec);
    auto r = per_aligned_decoder::decode_length_determinant(dec_view, dec_bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 127u);
}

TEST(LengthDeterminantRoundTrip, Len128_UT_PER_DEC_032) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t enc_bit_off = 0;

    per_aligned_encoder::encode_length_determinant(128, view, enc_bit_off);

    size_t dec_bit_off = 0;
    auto dec_view = make_const_view(buf_vec);
    auto r = per_aligned_decoder::decode_length_determinant(dec_view, dec_bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 128u);
}

TEST(LengthDeterminantRoundTrip, Len16383_UT_PER_DEC_033) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t enc_bit_off = 0;

    per_aligned_encoder::encode_length_determinant(16383, view, enc_bit_off);

    size_t dec_bit_off = 0;
    auto dec_view = make_const_view(buf_vec);
    auto r = per_aligned_decoder::decode_length_determinant(dec_view, dec_bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 16383u);
}

TEST(LengthDeterminantRoundTrip, Len16384_UT_PER_DEC_034) {
    auto buf_vec = make_mutable_buffer(16);
    auto view = make_view(buf_vec);
    size_t enc_bit_off = 0;

    per_aligned_encoder::encode_length_determinant(16384, view, enc_bit_off);

    size_t dec_bit_off = 0;
    auto dec_view = make_const_view(buf_vec);
    auto r = per_aligned_decoder::decode_length_determinant(dec_view, dec_bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 16384u);
}

TEST(LengthDeterminantDirect, SingleByte0_UT_PER_DEC_035) {
    std::vector<uint8_t> buf = {0x00};
    auto view = make_const_view(buf);
    size_t bit_off = 0;
    auto r = per_aligned_decoder::decode_length_determinant(view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0u);
}

TEST(LengthDeterminantDirect, SingleByte127_UT_PER_DEC_036) {
    std::vector<uint8_t> buf = {0x7F};
    auto view = make_const_view(buf);
    size_t bit_off = 0;
    auto r = per_aligned_decoder::decode_length_determinant(view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 127u);
}

TEST(LengthDeterminantDirect, TwoByte_128_UT_PER_DEC_037) {
    // 128 = 0x0080, two-byte form: 10_000000 + 1_0000000 = 0x80 0x80
    // Per encoder: 10xxxxxx xxxxxxxx → byte1=0x80|((128>>8)&0x3F)=0x80, byte2=0x00
    // Actually: 128>>8 = 0, so byte1 = 0x80, byte2 = 128&0xFF = 0x80? No.
    // Let me look at encoder code: out[byte_off] = 0x80 | ((length >> 8) & 0x3F)
    // 128 >> 8 = 0, so byte1 = 0x80. byte2 = 128 & 0xFF = 128 = 0x80
    // Wait no: byte2 = length & 0xFF = 128 & 255 = 128 = 0x80
    // So two-byte encoding of 128 is 0x80 0x80
    std::vector<uint8_t> buf = {0x80, 0x80};
    auto view = make_const_view(buf);
    size_t bit_off = 0;
    auto r = per_aligned_decoder::decode_length_determinant(view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 128u);
}

// ============================================================================
// 5. INTEGER round-trip via template API
// ============================================================================

class IntegerDecoderRoundTripTest : public ::testing::Test {
protected:
    per_aligned_encoder encoder_;
    per_aligned_decoder decoder_;
};

TEST_F(IntegerDecoderRoundTripTest, Uint8_42_UT_PER_DEC_040) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_integer<uint8_range_meta>(42, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_integer<uint8_range_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 42);
}

TEST_F(IntegerDecoderRoundTripTest, Uint16_1000_UT_PER_DEC_041) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_integer<uint16_range_meta>(1000, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_integer<uint16_range_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 1000);
}

TEST_F(IntegerDecoderRoundTripTest, SmallRange_5_UT_PER_DEC_042) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_integer<small_range_meta>(5, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_integer<small_range_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 5);
}

TEST_F(IntegerDecoderRoundTripTest, BitRange_0_UT_PER_DEC_043) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_integer<bit_range_meta>(0, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_integer<bit_range_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0);
}

TEST_F(IntegerDecoderRoundTripTest, BitRange_1_UT_PER_DEC_044) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_integer<bit_range_meta>(1, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_integer<bit_range_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 1);
}

TEST_F(IntegerDecoderRoundTripTest, MinValue_UT_PER_DEC_045) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_integer<uint8_range_meta>(0, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_integer<uint8_range_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0);
}

TEST_F(IntegerDecoderRoundTripTest, MaxValue_UT_PER_DEC_046) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_integer<uint8_range_meta>(255, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_integer<uint8_range_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 255);
}

TEST(IntegerDecoderErrorTest, BufferUnderflow_UT_PER_DEC_047) {
    per_aligned_decoder dec;
    std::vector<uint8_t> buf = {};
    auto view = make_const_view(buf);
    auto r = dec.decode_integer<uint8_range_meta>(view);
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::buffer_underflow);
}

// ============================================================================
// 6. BOOLEAN round-trip
// ============================================================================

class BooleanDecoderRoundTripTest : public ::testing::Test {
protected:
    per_aligned_encoder encoder_;
    per_aligned_decoder decoder_;
};

TEST_F(BooleanDecoderRoundTripTest, True_UT_PER_DEC_050) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_boolean(true, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_boolean(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), true);
}

TEST_F(BooleanDecoderRoundTripTest, False_UT_PER_DEC_051) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_boolean(false, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_boolean(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), false);
}

TEST(BooleanDecoderDirectTest, KnownByteTrue_UT_PER_DEC_052) {
    std::vector<uint8_t> buf = {0x80};  // 1 bit = 1, padded to byte
    auto view = make_const_view(buf);
    per_aligned_decoder dec;
    auto r = dec.decode_boolean(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_TRUE(r.value());
}

TEST(BooleanDecoderDirectTest, KnownByteFalse_UT_PER_DEC_053) {
    std::vector<uint8_t> buf = {0x00};  // 1 bit = 0, padded to byte
    auto view = make_const_view(buf);
    per_aligned_decoder dec;
    auto r = dec.decode_boolean(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_FALSE(r.value());
}

// ============================================================================
// 7. NULL round-trip
// ============================================================================

class NullDecoderRoundTripTest : public ::testing::Test {
protected:
    per_aligned_encoder encoder_;
    per_aligned_decoder decoder_;
};

TEST_F(NullDecoderRoundTripTest, Null_UT_PER_DEC_060) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_null(view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_null(dec_view);
    ASSERT_TRUE(r.is_ok());
}

// ============================================================================
// 8. OCTET STRING round-trip
// ============================================================================

class OctetStringDecoderRoundTripTest : public ::testing::Test {
protected:
    per_aligned_encoder encoder_;
    per_aligned_decoder decoder_;
};

TEST_F(OctetStringDecoderRoundTripTest, FixedSize4_UT_PER_DEC_070) {
    auto buf_vec = make_mutable_buffer(16);
    auto view = make_view(buf_vec);
    const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    encoder_.encode_octet_string<fixed_octets_meta>(std::span<const uint8_t>(data, 4), view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_octet_string<fixed_octets_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    std::vector<uint8_t> expected = {0x01, 0x02, 0x03, 0x04};
    EXPECT_EQ(r.value(), expected);
}

TEST_F(OctetStringDecoderRoundTripTest, Constrained_3Bytes_UT_PER_DEC_071) {
    auto buf_vec = make_mutable_buffer(16);
    auto view = make_view(buf_vec);
    const uint8_t data[] = {0xAA, 0xBB, 0xCC};
    encoder_.encode_octet_string<constrained_octets_meta>(std::span<const uint8_t>(data, 3), view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_octet_string<constrained_octets_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    std::vector<uint8_t> expected = {0xAA, 0xBB, 0xCC};
    EXPECT_EQ(r.value(), expected);
}

TEST_F(OctetStringDecoderRoundTripTest, Constrained_0Bytes_UT_PER_DEC_072) {
    auto buf_vec = make_mutable_buffer(16);
    auto view = make_view(buf_vec);
    encoder_.encode_octet_string<constrained_octets_meta>(std::span<const uint8_t>(), view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_octet_string<constrained_octets_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_TRUE(r.value().empty());
}

TEST_F(OctetStringDecoderRoundTripTest, Unconstrained_UT_PER_DEC_073) {
    auto buf_vec = make_mutable_buffer(16);
    auto view = make_view(buf_vec);
    const uint8_t data[] = {0xDE, 0xAD};
    encoder_.encode_octet_string<unconstrained_octets_meta>(std::span<const uint8_t>(data, 2), view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_octet_string<unconstrained_octets_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    std::vector<uint8_t> expected = {0xDE, 0xAD};
    EXPECT_EQ(r.value(), expected);
}

// ============================================================================
// 9. BIT STRING round-trip
// ============================================================================

class BitStringDecoderRoundTripTest : public ::testing::Test {
protected:
    per_aligned_encoder encoder_;
    per_aligned_decoder decoder_;
};

TEST_F(BitStringDecoderRoundTripTest, Fixed8_UT_PER_DEC_080) {
    auto buf_vec = make_mutable_buffer(16);
    auto view = make_view(buf_vec);
    // 0xA5 = 10100101, 8 bits, no unused
    const uint8_t data[] = {0xA5};
    encoder_.encode_bit_string<fixed_bit8_meta>(std::span<const uint8_t>(data, 1), 0, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_bit_string<fixed_bit8_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().first, std::vector<uint8_t>({0xA5}));
    EXPECT_EQ(r.value().second, 0);  // 8 bits → 0 unused in 1 byte
}

TEST_F(BitStringDecoderRoundTripTest, Fixed10_UT_PER_DEC_081) {
    auto buf_vec = make_mutable_buffer(16);
    auto view = make_view(buf_vec);
    // 10 bits: 1010101011 = 0x2AB in 2 bytes, 6 bits unused
    const uint8_t data[] = {0xAA, 0xC0};  // 10101010 11000000
    encoder_.encode_bit_string<fixed_bit10_meta>(std::span<const uint8_t>(data, 2), 6, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_bit_string<fixed_bit10_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().first, std::vector<uint8_t>({0xAA, 0xC0}));
}

TEST_F(BitStringDecoderRoundTripTest, Constrained_UT_PER_DEC_082) {
    auto buf_vec = make_mutable_buffer(16);
    auto view = make_view(buf_vec);
    const uint8_t data[] = {0xFF};
    encoder_.encode_bit_string<constrained_bit_meta>(std::span<const uint8_t>(data, 1), 0, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_bit_string<constrained_bit_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().first, std::vector<uint8_t>({0xFF}));
}

// ============================================================================
// 10. ENUMERATED round-trip
// ============================================================================

class EnumeratedDecoderRoundTripTest : public ::testing::Test {
protected:
    per_aligned_encoder encoder_;
    per_aligned_decoder decoder_;
};

TEST_F(EnumeratedDecoderRoundTripTest, Index0_UT_PER_DEC_090) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_enumerated<color_enum_meta>(0, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_enumerated<color_enum_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0);
}

TEST_F(EnumeratedDecoderRoundTripTest, Index1_UT_PER_DEC_091) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_enumerated<color_enum_meta>(1, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_enumerated<color_enum_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 1);
}

TEST_F(EnumeratedDecoderRoundTripTest, Index2_UT_PER_DEC_092) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_enumerated<color_enum_meta>(2, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_enumerated<color_enum_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 2);
}

TEST_F(EnumeratedDecoderRoundTripTest, SingleValueEnum_UT_PER_DEC_093) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_enumerated<single_val_enum_meta>(0, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_enumerated<single_val_enum_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0);
}

TEST(EnumeratedDecoderErrorTest, OutOfRange_UT_PER_DEC_094) {
    std::vector<uint8_t> buf = {0xC0};  // index 3 in 2 bits = 11, but only 3 values (0..2)
    auto view = make_const_view(buf);
    per_aligned_decoder dec;
    auto r = dec.decode_enumerated<color_enum_meta>(view);
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::value_out_of_range);
}

// ============================================================================
// 11. SEQUENCE round-trip
// ============================================================================

class SequenceDecoderRoundTripTest : public ::testing::Test {
protected:
    per_aligned_encoder encoder_;
    per_aligned_decoder decoder_;
};

TEST_F(SequenceDecoderRoundTripTest, OneOptionalPresent_UT_PER_DEC_100) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    const bool present[] = {true};
    encoder_.encode_sequence_start<seq_one_optional_meta>(view, present, 1);
    encoder_.encode_sequence_end(view);

    auto dec_view = make_const_view(buf_vec);
    bool out_present[1] = {false};
    auto r = decoder_.decode_sequence_start<seq_one_optional_meta>(dec_view, out_present, 1);
    ASSERT_TRUE(r.is_ok());
    EXPECT_FALSE(r.value());  // no extension chosen
    EXPECT_TRUE(out_present[0]);
}

TEST_F(SequenceDecoderRoundTripTest, OneOptionalAbsent_UT_PER_DEC_101) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    const bool present[] = {false};
    encoder_.encode_sequence_start<seq_one_optional_meta>(view, present, 1);
    encoder_.encode_sequence_end(view);

    auto dec_view = make_const_view(buf_vec);
    bool out_present[1] = {true};
    auto r = decoder_.decode_sequence_start<seq_one_optional_meta>(dec_view, out_present, 1);
    ASSERT_TRUE(r.is_ok());
    EXPECT_FALSE(r.value());
    EXPECT_FALSE(out_present[0]);
}

TEST_F(SequenceDecoderRoundTripTest, NoOptionals_UT_PER_DEC_102) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_sequence_start<seq_plain_meta>(view, nullptr, 0);
    encoder_.encode_sequence_end(view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_sequence_start<seq_plain_meta>(dec_view, nullptr, 0);
    ASSERT_TRUE(r.is_ok());
    EXPECT_FALSE(r.value());
}

TEST_F(SequenceDecoderRoundTripTest, ExtensionBitNoExt_UT_PER_DEC_103) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_sequence_start<seq_with_ext_meta>(view, nullptr, 0);
    encoder_.encode_sequence_end(view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_sequence_start<seq_with_ext_meta>(dec_view, nullptr, 0);
    ASSERT_TRUE(r.is_ok());
    EXPECT_FALSE(r.value());  // extension bit = 0
}

TEST_F(SequenceDecoderRoundTripTest, ExtensionWithOptional_UT_PER_DEC_104) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    const bool present[] = {true};
    encoder_.encode_sequence_start<seq_ext_optional_meta>(view, present, 1);
    encoder_.encode_sequence_end(view);

    auto dec_view = make_const_view(buf_vec);
    bool out_present[1] = {false};
    auto r = decoder_.decode_sequence_start<seq_ext_optional_meta>(dec_view, out_present, 1);
    ASSERT_TRUE(r.is_ok());
    EXPECT_FALSE(r.value());   // extension bit = 0
    EXPECT_TRUE(out_present[0]);  // optional present
}

// ============================================================================
// 12. CHOICE round-trip
// ============================================================================

class ChoiceDecoderRoundTripTest : public ::testing::Test {
protected:
    per_aligned_encoder encoder_;
    per_aligned_decoder decoder_;
};

TEST_F(ChoiceDecoderRoundTripTest, Index0_UT_PER_DEC_110) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_choice_index<choice_4_meta>(0, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_choice_index<choice_4_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0);
}

TEST_F(ChoiceDecoderRoundTripTest, Index2_UT_PER_DEC_111) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_choice_index<choice_4_meta>(2, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_choice_index<choice_4_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 2);
}

TEST_F(ChoiceDecoderRoundTripTest, Index3_UT_PER_DEC_112) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_choice_index<choice_4_meta>(3, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_choice_index<choice_4_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 3);
}

// ============================================================================
// 13. SEQUENCE OF length round-trip
// ============================================================================

class SeqOfLenDecoderRoundTripTest : public ::testing::Test {
protected:
    per_aligned_encoder encoder_;
    per_aligned_decoder decoder_;
};

TEST_F(SeqOfLenDecoderRoundTripTest, Constrained256_Len5_UT_PER_DEC_120) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_sequence_of_length<seqof_256_meta>(5, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_sequence_of_length<seqof_256_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 5u);
}

TEST_F(SeqOfLenDecoderRoundTripTest, Constrained64k_Len5000_UT_PER_DEC_121) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_sequence_of_length<seqof_64k_meta>(5000, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_sequence_of_length<seqof_64k_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 5000u);
}

TEST_F(SeqOfLenDecoderRoundTripTest, Unconstrained_Len5_UT_PER_DEC_122) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    encoder_.encode_sequence_of_length<seqof_unconstrained_meta>(5, view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_sequence_of_length<seqof_unconstrained_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 5u);
}

// ============================================================================
// 14. OID round-trip
// ============================================================================

class OidDecoderRoundTripTest : public ::testing::Test {
protected:
    per_aligned_encoder encoder_;
    per_aligned_decoder decoder_;
};

TEST_F(OidDecoderRoundTripTest, EncodeOidBasic_UT_PER_DEC_130) {
    auto buf_vec = make_mutable_buffer(16);
    auto view = make_view(buf_vec);
    const uint8_t oid_bytes[] = {0x55, 0x04, 0x03};
    encoder_.encode_oid(std::span<const uint8_t>(oid_bytes, 3), view);

    auto dec_view = make_const_view(buf_vec);
    auto r = decoder_.decode_oid(dec_view);
    ASSERT_TRUE(r.is_ok());
    std::vector<uint8_t> expected = {0x55, 0x04, 0x03};
    EXPECT_EQ(r.value(), expected);
}

// ============================================================================
// 15. Compound round-trip scenarios
// ============================================================================

TEST(CompoundDecoderRoundTripTest, SequenceWithIntegers_UT_PER_DEC_140) {
    per_aligned_encoder enc;
    auto buf_vec = make_mutable_buffer(64);
    auto view = make_view(buf_vec);

    // Encode SEQUENCE with ext + optional, then two integers
    const bool present[] = {true};
    enc.encode_sequence_start<seq_ext_optional_meta>(view, present, 1);
    enc.encode_integer<uint8_range_meta>(42, view);
    enc.encode_integer<uint16_range_meta>(1000, view);
    enc.encode_sequence_end(view);

    // Decode
    per_aligned_decoder dec;
    auto dec_view = make_const_view(buf_vec);

    bool out_present[1] = {false};
    auto r_seq = dec.decode_sequence_start<seq_ext_optional_meta>(dec_view, out_present, 1);
    ASSERT_TRUE(r_seq.is_ok());
    EXPECT_FALSE(r_seq.value());
    EXPECT_TRUE(out_present[0]);

    auto r_int1 = dec.decode_integer<uint8_range_meta>(dec_view);
    ASSERT_TRUE(r_int1.is_ok());
    EXPECT_EQ(r_int1.value(), 42);

    auto r_int2 = dec.decode_integer<uint16_range_meta>(dec_view);
    ASSERT_TRUE(r_int2.is_ok());
    EXPECT_EQ(r_int2.value(), 1000);
}

TEST(CompoundDecoderRoundTripTest, AllNullEncoding_UT_PER_DEC_141) {
    per_aligned_encoder enc;
    auto buf_vec = make_mutable_buffer(64);
    auto view = make_view(buf_vec);

    enc.encode_integer<uint8_range_meta>(0, view);
    enc.encode_boolean(false, view);
    enc.encode_null(view);
    const uint8_t zeros[] = {0x00, 0x00, 0x00, 0x00};
    enc.encode_octet_string<fixed_octets_meta>(std::span<const uint8_t>(zeros, 4), view);

    // Decode
    per_aligned_decoder dec;
    auto dec_view = make_const_view(buf_vec);

    auto r_int = dec.decode_integer<uint8_range_meta>(dec_view);
    ASSERT_TRUE(r_int.is_ok());
    EXPECT_EQ(r_int.value(), 0);

    auto r_bool = dec.decode_boolean(dec_view);
    ASSERT_TRUE(r_bool.is_ok());
    EXPECT_FALSE(r_bool.value());

    auto r_null = dec.decode_null(dec_view);
    ASSERT_TRUE(r_null.is_ok());

    auto r_oct = dec.decode_octet_string<fixed_octets_meta>(dec_view);
    ASSERT_TRUE(r_oct.is_ok());
    std::vector<uint8_t> expected_oct = {0x00, 0x00, 0x00, 0x00};
    EXPECT_EQ(r_oct.value(), expected_oct);
}

TEST(CompoundDecoderRoundTripTest, MultiFieldSequenceBitmap_UT_PER_DEC_142) {
    per_aligned_encoder enc;
    auto buf_vec = make_mutable_buffer(64);
    auto view = make_view(buf_vec);

    const bool present[] = {true, false};
    enc.encode_sequence_start<three_field_two_optional_meta>(view, present, 2);
    enc.encode_integer<uint8_range_meta>(1, view);
    enc.encode_integer<uint8_range_meta>(2, view);
    // third field absent, skip
    enc.encode_sequence_end(view);

    // Decode
    per_aligned_decoder dec;
    auto dec_view = make_const_view(buf_vec);

    bool out_present[2] = {false, false};
    auto r_seq = dec.decode_sequence_start<three_field_two_optional_meta>(dec_view, out_present, 2);
    ASSERT_TRUE(r_seq.is_ok());
    EXPECT_FALSE(r_seq.value());
    EXPECT_TRUE(out_present[0]);
    EXPECT_FALSE(out_present[1]);

    auto r1 = dec.decode_integer<uint8_range_meta>(dec_view);
    ASSERT_TRUE(r1.is_ok());
    EXPECT_EQ(r1.value(), 1);

    auto r2 = dec.decode_integer<uint8_range_meta>(dec_view);
    ASSERT_TRUE(r2.is_ok());
    EXPECT_EQ(r2.value(), 2);
}

// ============================================================================
// 16. Boundary / edge case tests
// ============================================================================

TEST(EdgeCaseDecoderTest, BitOffsetTrackingAcrossCalls_UT_PER_DEC_150) {
    per_aligned_encoder enc;
    auto buf_vec = make_mutable_buffer(64);
    auto view = make_view(buf_vec);

    enc.encode_boolean(true, view);
    enc.encode_integer<uint8_range_meta>(42, view);
    enc.encode_boolean(false, view);

    per_aligned_decoder dec;
    auto dec_view = make_const_view(buf_vec);

    auto r1 = dec.decode_boolean(dec_view);
    ASSERT_TRUE(r1.is_ok());
    EXPECT_TRUE(r1.value());
    EXPECT_EQ(dec.bit_offset(), 8);

    auto r2 = dec.decode_integer<uint8_range_meta>(dec_view);
    ASSERT_TRUE(r2.is_ok());
    EXPECT_EQ(r2.value(), 42);
    EXPECT_EQ(dec.bit_offset(), 16);

    auto r3 = dec.decode_boolean(dec_view);
    ASSERT_TRUE(r3.is_ok());
    EXPECT_FALSE(r3.value());
    EXPECT_EQ(dec.bit_offset(), 24);
}

TEST(EdgeCaseDecoderTest, IntegerMinMaxInSequence_UT_PER_DEC_151) {
    per_aligned_encoder enc;
    auto buf_vec = make_mutable_buffer(64);
    auto view = make_view(buf_vec);

    enc.encode_integer<uint8_range_meta>(0, view);
    enc.encode_integer<uint8_range_meta>(255, view);

    per_aligned_decoder dec;
    auto dec_view = make_const_view(buf_vec);

    auto r1 = dec.decode_integer<uint8_range_meta>(dec_view);
    ASSERT_TRUE(r1.is_ok());
    EXPECT_EQ(r1.value(), 0);

    auto r2 = dec.decode_integer<uint8_range_meta>(dec_view);
    ASSERT_TRUE(r2.is_ok());
    EXPECT_EQ(r2.value(), 255);
}

TEST(EdgeCaseDecoderTest, ConstrainedOctetStringMax_UT_PER_DEC_152) {
    per_aligned_encoder enc;
    auto buf_vec = make_mutable_buffer(512);
    auto view = make_view(buf_vec);

    // SIZE(0..255): encode 255 bytes
    std::vector<uint8_t> data(255, 0xAB);
    enc.encode_octet_string<constrained_octets_meta>(std::span<const uint8_t>(data), view);

    per_aligned_decoder dec;
    auto dec_view = make_const_view(buf_vec);
    auto r = dec.decode_octet_string<constrained_octets_meta>(dec_view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().size(), 255u);
    EXPECT_EQ(r.value()[0], 0xAB);
    EXPECT_EQ(r.value()[254], 0xAB);
}

TEST(EdgeCaseDecoderTest, LargeConstrainedInteger_UT_PER_DEC_153) {
    auto buf_vec = make_mutable_buffer(32);
    auto view = make_view(buf_vec);
    size_t enc_bit_off = 0;

    per_aligned_encoder::encode_constrained_whole_number(50000, 0, 100000, view, enc_bit_off);

    size_t dec_bit_off = 0;
    auto dec_view = make_const_view(buf_vec);
    auto r = per_aligned_decoder::decode_constrained_whole_number(0, 100000, dec_view, dec_bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 50000);
}
