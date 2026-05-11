#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>
#include <cstring>

#include "codec/result.hpp"
#include "codec/per/uper_encoder.hpp"
#include "codec/per/uper_decoder.hpp"

using namespace asn1pp;
using namespace asn1pp::per;

// ============================================================================
// PER Metadata structs (same as uper_encoder_test.cpp)
// ============================================================================

namespace {

// INTEGER (0..255)
struct uint8_range_meta {
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 255;
    static constexpr bool has_range_constraint = true;
    static constexpr bool is_extension_permitted = false;
};

// INTEGER (0..65535)
struct uint16_range_meta {
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 65535;
    static constexpr bool has_range_constraint = true;
    static constexpr bool is_extension_permitted = false;
};

// INTEGER (1..10)
struct small_range_meta {
    static constexpr int64_t min_value = 1;
    static constexpr int64_t max_value = 10;
    static constexpr bool has_range_constraint = true;
    static constexpr bool is_extension_permitted = false;
};

// INTEGER (0..1) — 1-bit range
struct bit_range_meta {
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 1;
    static constexpr bool has_range_constraint = true;
    static constexpr bool is_extension_permitted = false;
};

// INTEGER unconstrained
struct unconstrained_int_meta {
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 9223372036854775807LL;
    static constexpr bool has_range_constraint = false;
    static constexpr bool is_extension_permitted = false;
};

// OCTET STRING SIZE(4)
struct fixed_octets_meta {
    static constexpr size_t min_size = 4;
    static constexpr size_t max_size = 4;
    static constexpr bool has_size_constraint = true;
    static constexpr bool is_extension_permitted = false;
};

// OCTET STRING SIZE(0..255)
struct constrained_octets_meta {
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 255;
    static constexpr bool has_size_constraint = true;
    static constexpr bool is_extension_permitted = false;
};

// OCTET STRING unconstrained
struct unconstrained_octets_meta {
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 18446744073709551615ULL;
    static constexpr bool has_size_constraint = false;
    static constexpr bool is_extension_permitted = false;
};

// ENUMERATED { red(0), green(1), blue(2) }
struct color_enum_meta {
    static constexpr size_t normal_index_count = 3;
    static constexpr bool has_extension = false;
};

// ENUMERATED single value
struct single_val_enum_meta {
    static constexpr size_t normal_index_count = 1;
    static constexpr bool has_extension = false;
};

// SEQUENCE no optionals, no extension
struct seq_plain_meta {
    static constexpr size_t field_count = 2;
    static constexpr bool optional_bitmap[2] = {false, false};
    static constexpr bool has_extension = false;
};

// SEQUENCE with 1 optional
struct seq_one_optional_meta {
    static constexpr size_t field_count = 2;
    static constexpr bool optional_bitmap[2] = {false, true};
    static constexpr bool has_extension = false;
};

// SEQUENCE with extension + optional
struct seq_ext_optional_meta {
    static constexpr size_t field_count = 2;
    static constexpr bool optional_bitmap[2] = {false, true};
    static constexpr bool has_extension = true;
};

// SEQUENCE with extension, no optionals
struct seq_with_ext_meta {
    static constexpr size_t field_count = 1;
    static constexpr bool optional_bitmap[1] = {false};
    static constexpr bool has_extension = true;
};

// CHOICE with 4 alternatives
struct choice_4_meta {
    static constexpr size_t alternative_count = 4;
    static constexpr bool has_extension = false;
};

// CHOICE with extension
struct choice_ext_meta {
    static constexpr size_t alternative_count = 3;
    static constexpr bool has_extension = true;
};

// SEQUENCE OF with size constraint 0..255
struct seqof_256_meta {
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 255;
    static constexpr bool has_size_constraint = true;
};

// SEQUENCE OF unconstrained
struct seqof_unconstrained_meta {
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 18446744073709551615ULL;
    static constexpr bool has_size_constraint = false;
};

// INTEGER (0..100000) — large range
struct large_range_meta {
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 100000;
    static constexpr bool has_range_constraint = true;
    static constexpr bool is_extension_permitted = false;
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

} // namespace

// ============================================================================
// 1. bits_needed_for_range / bits_needed_unsigned
// ============================================================================

TEST(BitsNeededDecoderTest, Range0_255_UT_UPER_DEC_001) {
    EXPECT_EQ(uper_decoder::bits_needed_for_range(0, 255), 8);
}

TEST(BitsNeededDecoderTest, Range0_65535_UT_UPER_DEC_002) {
    EXPECT_EQ(uper_decoder::bits_needed_for_range(0, 65535), 16);
}

TEST(BitsNeededDecoderTest, Range1_10_UT_UPER_DEC_003) {
    EXPECT_EQ(uper_decoder::bits_needed_for_range(1, 10), 4);
}

TEST(BitsNeededDecoderTest, Range0_1_UT_UPER_DEC_004) {
    EXPECT_EQ(uper_decoder::bits_needed_for_range(0, 1), 1);
}

TEST(BitsNeededDecoderTest, Range0_0_UT_UPER_DEC_005) {
    EXPECT_EQ(uper_decoder::bits_needed_for_range(0, 0), 0);
}

TEST(BitsNeededDecoderTest, BitsNeededUnsigned_UT_UPER_DEC_006) {
    EXPECT_EQ(uper_decoder::bits_needed_unsigned(0), 0);
    EXPECT_EQ(uper_decoder::bits_needed_unsigned(1), 1);
    EXPECT_EQ(uper_decoder::bits_needed_unsigned(2), 2);
    EXPECT_EQ(uper_decoder::bits_needed_unsigned(3), 2);
    EXPECT_EQ(uper_decoder::bits_needed_unsigned(7), 3);
    EXPECT_EQ(uper_decoder::bits_needed_unsigned(255), 8);
    EXPECT_EQ(uper_decoder::bits_needed_unsigned(65535), 16);
}

// ============================================================================
// 2. Constrained whole number — unaligned (X.691 §12.2)
// ============================================================================

TEST(ConstrainedWholeNumberUnalignedDecodeTest, Range0_255_Value42_UT_UPER_DEC_010) {
    // Encode: range 0..255, value 42 = 0x2A
    auto buf_vec = make_mutable_buffer(8);
    auto enc_view = make_view(buf_vec);
    size_t w_off = 0;
    {
        auto r = uper_encoder::encode_constrained_whole_number_unaligned(42, 0, 255, enc_view, w_off);
        ASSERT_TRUE(r.is_ok());
    }

    // Decode at bit 0
    auto dec_view = make_view(buf_vec);
    size_t r_off = 0;
    auto dec_r = uper_decoder::decode_constrained_whole_number_unaligned(0, 255, dec_view, r_off);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 42);
    EXPECT_EQ(r_off, 8);
}

TEST(ConstrainedWholeNumberUnalignedDecodeTest, Range0_65535_Value60000_UT_UPER_DEC_011) {
    auto buf_vec = make_mutable_buffer(8);
    auto enc_view = make_view(buf_vec);
    size_t w_off = 0;
    {
        auto r = uper_encoder::encode_constrained_whole_number_unaligned(60000, 0, 65535, enc_view, w_off);
        ASSERT_TRUE(r.is_ok());
    }

    auto dec_view = make_view(buf_vec);
    size_t r_off = 0;
    auto dec_r = uper_decoder::decode_constrained_whole_number_unaligned(0, 65535, dec_view, r_off);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 60000);
    EXPECT_EQ(r_off, 16);
}

TEST(ConstrainedWholeNumberUnalignedDecodeTest, Range1_10_Value5_UT_UPER_DEC_012) {
    auto buf_vec = make_mutable_buffer(8);
    auto enc_view = make_view(buf_vec);
    size_t w_off = 0;
    {
        auto r = uper_encoder::encode_constrained_whole_number_unaligned(5, 1, 10, enc_view, w_off);
        ASSERT_TRUE(r.is_ok());
    }

    auto dec_view = make_view(buf_vec);
    size_t r_off = 0;
    auto dec_r = uper_decoder::decode_constrained_whole_number_unaligned(1, 10, dec_view, r_off);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 5);
}

TEST(ConstrainedWholeNumberUnalignedDecodeTest, Range0_1_Value0_UT_UPER_DEC_013) {
    auto buf_vec = make_mutable_buffer(8);
    auto enc_view = make_view(buf_vec);
    size_t w_off = 0;
    {
        auto r = uper_encoder::encode_constrained_whole_number_unaligned(0, 0, 1, enc_view, w_off);
        ASSERT_TRUE(r.is_ok());
    }

    auto dec_view = make_view(buf_vec);
    size_t r_off = 0;
    auto dec_r = uper_decoder::decode_constrained_whole_number_unaligned(0, 1, dec_view, r_off);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 0);
    EXPECT_EQ(r_off, 1);
}

TEST(ConstrainedWholeNumberUnalignedDecodeTest, Range0_1_Value1_UT_UPER_DEC_014) {
    auto buf_vec = make_mutable_buffer(8);
    auto enc_view = make_view(buf_vec);
    size_t w_off = 0;
    {
        auto r = uper_encoder::encode_constrained_whole_number_unaligned(1, 0, 1, enc_view, w_off);
        ASSERT_TRUE(r.is_ok());
    }

    auto dec_view = make_view(buf_vec);
    size_t r_off = 0;
    auto dec_r = uper_decoder::decode_constrained_whole_number_unaligned(0, 1, dec_view, r_off);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 1);
    EXPECT_EQ(r_off, 1);
}

TEST(ConstrainedWholeNumberUnalignedDecodeTest, NonZeroBitOffset_UT_UPER_DEC_015) {
    auto buf_vec = make_mutable_buffer(8);
    auto enc_view = make_view(buf_vec);
    size_t w_off = 3; // start at bit 3
    {
        auto r = uper_encoder::encode_constrained_whole_number_unaligned(42, 0, 255, enc_view, w_off);
        ASSERT_TRUE(r.is_ok());
    }

    auto dec_view = make_view(buf_vec);
    size_t r_off = 3;
    auto dec_r = uper_decoder::decode_constrained_whole_number_unaligned(0, 255, dec_view, r_off);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 42);
    EXPECT_EQ(r_off, 11);
}

TEST(ConstrainedWholeNumberUnalignedDecodeTest, LargeRange_0_100000_Value12345_UT_UPER_DEC_016) {
    auto buf_vec = make_mutable_buffer(8);
    auto enc_view = make_view(buf_vec);
    size_t w_off = 0;
    {
        auto r = uper_encoder::encode_constrained_whole_number_unaligned(12345, 0, 100000, enc_view, w_off);
        ASSERT_TRUE(r.is_ok());
    }

    auto dec_view = make_view(buf_vec);
    size_t r_off = 0;
    auto dec_r = uper_decoder::decode_constrained_whole_number_unaligned(0, 100000, dec_view, r_off);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 12345);
}

TEST(ConstrainedWholeNumberUnalignedDecodeTest, Range0_255_Value0_UT_UPER_DEC_017) {
    auto buf_vec = make_mutable_buffer(8);
    auto enc_view = make_view(buf_vec);
    size_t w_off = 0;
    {
        auto r = uper_encoder::encode_constrained_whole_number_unaligned(0, 0, 255, enc_view, w_off);
        ASSERT_TRUE(r.is_ok());
    }

    auto dec_view = make_view(buf_vec);
    size_t r_off = 0;
    auto dec_r = uper_decoder::decode_constrained_whole_number_unaligned(0, 255, dec_view, r_off);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 0);
}

TEST(ConstrainedWholeNumberUnalignedDecodeTest, BufferUnderflow_UT_UPER_DEC_018) {
    // Buffer too small
    auto buf_vec = make_mutable_buffer(0);
    auto dec_view = make_view(buf_vec);
    size_t r_off = 0;
    auto dec_r = uper_decoder::decode_constrained_whole_number_unaligned(0, 255, dec_view, r_off);
    ASSERT_TRUE(dec_r.is_err());
}

// ============================================================================
// 3. Length determinant — unaligned (X.691 §21)
// ============================================================================

TEST(LengthDeterminantUnalignedDecodeTest, Length0_UT_UPER_DEC_020) {
    auto buf_vec = make_mutable_buffer(8);
    auto enc_view = make_view(buf_vec);
    size_t w_off = 0;
    {
        auto r = uper_encoder::encode_length_determinant_unaligned(0, enc_view, w_off);
        ASSERT_TRUE(r.is_ok());
    }

    auto dec_view = make_view(buf_vec);
    size_t r_off = 0;
    auto dec_r = uper_decoder::decode_length_determinant_unaligned(dec_view, r_off);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 0u);
    EXPECT_EQ(r_off, 8);
}

TEST(LengthDeterminantUnalignedDecodeTest, Length127_UT_UPER_DEC_021) {
    auto buf_vec = make_mutable_buffer(8);
    auto enc_view = make_view(buf_vec);
    size_t w_off = 0;
    {
        auto r = uper_encoder::encode_length_determinant_unaligned(127, enc_view, w_off);
        ASSERT_TRUE(r.is_ok());
    }

    auto dec_view = make_view(buf_vec);
    size_t r_off = 0;
    auto dec_r = uper_decoder::decode_length_determinant_unaligned(dec_view, r_off);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 127u);
    EXPECT_EQ(r_off, 8);
}

TEST(LengthDeterminantUnalignedDecodeTest, Length128_UT_UPER_DEC_022) {
    auto buf_vec = make_mutable_buffer(8);
    auto enc_view = make_view(buf_vec);
    size_t w_off = 0;
    {
        auto r = uper_encoder::encode_length_determinant_unaligned(128, enc_view, w_off);
        ASSERT_TRUE(r.is_ok());
    }

    auto dec_view = make_view(buf_vec);
    size_t r_off = 0;
    auto dec_r = uper_decoder::decode_length_determinant_unaligned(dec_view, r_off);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 128u);
    EXPECT_EQ(r_off, 16);
}

TEST(LengthDeterminantUnalignedDecodeTest, Length16383_UT_UPER_DEC_023) {
    auto buf_vec = make_mutable_buffer(8);
    auto enc_view = make_view(buf_vec);
    size_t w_off = 0;
    {
        auto r = uper_encoder::encode_length_determinant_unaligned(16383, enc_view, w_off);
        ASSERT_TRUE(r.is_ok());
    }

    auto dec_view = make_view(buf_vec);
    size_t r_off = 0;
    auto dec_r = uper_decoder::decode_length_determinant_unaligned(dec_view, r_off);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 16383u);
    EXPECT_EQ(r_off, 16);
}

TEST(LengthDeterminantUnalignedDecodeTest, NonZeroBitOffset_UT_UPER_DEC_024) {
    auto buf_vec = make_mutable_buffer(8);
    auto enc_view = make_view(buf_vec);
    size_t w_off = 5;
    {
        auto r = uper_encoder::encode_length_determinant_unaligned(42, enc_view, w_off);
        ASSERT_TRUE(r.is_ok());
    }

    auto dec_view = make_view(buf_vec);
    size_t r_off = 5;
    auto dec_r = uper_decoder::decode_length_determinant_unaligned(dec_view, r_off);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 42u);
}

TEST(LengthDeterminantUnalignedDecodeTest, BufferUnderflow_UT_UPER_DEC_025) {
    auto buf_vec = make_mutable_buffer(0);
    auto dec_view = make_view(buf_vec);
    size_t r_off = 0;
    auto dec_r = uper_decoder::decode_length_determinant_unaligned(dec_view, r_off);
    ASSERT_TRUE(dec_r.is_err());
}

// ============================================================================
// 4. Round-trip: INTEGER
// ============================================================================

TEST(UperDecoderRoundTripTest, Integer_0_255_Value42_UT_UPER_DEC_030) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto r = enc.encode_integer<uint8_range_meta>(42, enc_view);
    ASSERT_TRUE(r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_integer<uint8_range_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 42);
}

TEST(UperDecoderRoundTripTest, Integer_0_65535_Value1000_UT_UPER_DEC_031) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto r = enc.encode_integer<uint16_range_meta>(1000, enc_view);
    ASSERT_TRUE(r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_integer<uint16_range_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 1000);
}

TEST(UperDecoderRoundTripTest, Integer_1_10_Value5_UT_UPER_DEC_032) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto r = enc.encode_integer<small_range_meta>(5, enc_view);
    ASSERT_TRUE(r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_integer<small_range_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 5);
}

TEST(UperDecoderRoundTripTest, Integer_0_1_Value1_UT_UPER_DEC_033) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto r = enc.encode_integer<bit_range_meta>(1, enc_view);
    ASSERT_TRUE(r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_integer<bit_range_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 1);
}

TEST(UperDecoderRoundTripTest, Integer_LargeRange_12345_UT_UPER_DEC_034) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto r = enc.encode_integer<large_range_meta>(12345, enc_view);
    ASSERT_TRUE(r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_integer<large_range_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 12345);
}

TEST(UperDecoderRoundTripTest, Integer_Unconstrained_Value100_UT_UPER_DEC_035) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto r = enc.encode_integer<unconstrained_int_meta>(100, enc_view);
    ASSERT_TRUE(r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_integer<unconstrained_int_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 100);
}

TEST(UperDecoderRoundTripTest, Integer_Constrained_Edge_0_UT_UPER_DEC_036) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto r = enc.encode_integer<uint8_range_meta>(0, enc_view);
    ASSERT_TRUE(r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_integer<uint8_range_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 0);
}

TEST(UperDecoderRoundTripTest, Integer_Constrained_Edge_255_UT_UPER_DEC_037) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto r = enc.encode_integer<uint8_range_meta>(255, enc_view);
    ASSERT_TRUE(r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_integer<uint8_range_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 255);
}

// ============================================================================
// 5. Round-trip: BOOLEAN
// ============================================================================

TEST(UperDecoderRoundTripTest, Boolean_True_UT_UPER_DEC_040) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto r = enc.encode_boolean(true, enc_view);
    ASSERT_TRUE(r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_boolean(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), true);
}

TEST(UperDecoderRoundTripTest, Boolean_False_UT_UPER_DEC_041) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto r = enc.encode_boolean(false, enc_view);
    ASSERT_TRUE(r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_boolean(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), false);
}

// ============================================================================
// 6. Round-trip: NULL
// ============================================================================

TEST(UperDecoderRoundTripTest, NullRoundTrip_UT_UPER_DEC_045) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto r = enc.encode_null(enc_view);
    ASSERT_TRUE(r.is_ok());

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_null(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
}

// ============================================================================
// 7. Round-trip: OCTET STRING
// ============================================================================

#ifndef ASN1PP_EMBEDDED
TEST(UperDecoderRoundTripTest, OctetString_Fixed4_UT_UPER_DEC_050) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    std::vector<uint8_t> data = {0xAA, 0xBB, 0xCC, 0xDD};
    auto enc_r = enc.encode_octet_string<fixed_octets_meta>(std::span<const uint8_t>(data), enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_octet_string<fixed_octets_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), data);
}

TEST(UperDecoderRoundTripTest, OctetString_Constrained_UT_UPER_DEC_051) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    auto enc_r = enc.encode_octet_string<constrained_octets_meta>(std::span<const uint8_t>(data), enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_octet_string<constrained_octets_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), data);
}

TEST(UperDecoderRoundTripTest, OctetString_Unconstrained_UT_UPER_DEC_052) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    std::vector<uint8_t> data = {0xDE, 0xAD};
    auto enc_r = enc.encode_octet_string<unconstrained_octets_meta>(std::span<const uint8_t>(data), enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_octet_string<unconstrained_octets_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), data);
}

// ============================================================================
// 8. Round-trip: BIT STRING
// ============================================================================

// BIT STRING SIZE(8) metadata
struct bit_string_fixed8_meta {
    static constexpr size_t min_size = 8;
    static constexpr size_t max_size = 8;
    static constexpr bool has_size_constraint = true;
    static constexpr bool is_extension_permitted = false;
};

TEST(UperDecoderRoundTripTest, BitString_Fixed8_UT_UPER_DEC_060) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    // BIT STRING SIZE(8): 1 data byte, 0 unused
    std::vector<uint8_t> data = {0xAA};
    auto enc_r = enc.encode_bit_string<bit_string_fixed8_meta>(std::span<const uint8_t>(data), 0, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_bit_string<bit_string_fixed8_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value().first, data);
    EXPECT_EQ(dec_r.value().second, 0);
}

// BIT STRING SIZE(0..255) metadata
struct bit_string_constrained_meta {
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 255;
    static constexpr bool has_size_constraint = true;
    static constexpr bool is_extension_permitted = false;
};

// BIT STRING unconstrained metadata
struct bit_string_unconstrained_meta {
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 18446744073709551615ULL;
    static constexpr bool has_size_constraint = false;
    static constexpr bool is_extension_permitted = false;
};

TEST(UperDecoderRoundTripTest, BitString_Unconstrained_UT_UPER_DEC_061) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    // 10 bits of data → 2 bytes, 6 unused
    std::vector<uint8_t> data = {0xA0, 0x00};
    auto enc_r = enc.encode_bit_string<bit_string_unconstrained_meta>(
        std::span<const uint8_t>(data), 6, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_bit_string<bit_string_unconstrained_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value().first, data);
    EXPECT_EQ(dec_r.value().second, 6);
}
#endif // ASN1PP_EMBEDDED

// ============================================================================
// 9. Round-trip: ENUMERATED
// ============================================================================

TEST(UperDecoderRoundTripTest, Enumerated_Index0_UT_UPER_DEC_070) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto enc_r = enc.encode_enumerated<color_enum_meta>(0, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_enumerated<color_enum_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 0);
}

TEST(UperDecoderRoundTripTest, Enumerated_Index2_UT_UPER_DEC_071) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto enc_r = enc.encode_enumerated<color_enum_meta>(2, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_enumerated<color_enum_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 2);
}

TEST(UperDecoderRoundTripTest, Enumerated_SingleVal_UT_UPER_DEC_072) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto enc_r = enc.encode_enumerated<single_val_enum_meta>(0, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_enumerated<single_val_enum_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 0);
}

// ============================================================================
// 10. Round-trip: SEQUENCE
// ============================================================================

TEST(UperDecoderRoundTripTest, Sequence_Plain_UT_UPER_DEC_080) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto enc_start = enc.encode_sequence_start<seq_plain_meta>(enc_view, nullptr, 0);
    ASSERT_TRUE(enc_start.is_ok());
    // No extension bit + no optional bitmap = 0 bits written

    auto enc_end = enc.encode_sequence_end(enc_view);
    ASSERT_TRUE(enc_end.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    bool optional_buf[2] = {false, false};
    auto dec_start = dec.decode_sequence_start<seq_plain_meta>(dec_view, optional_buf, 0);
    ASSERT_TRUE(dec_start.is_ok());
    EXPECT_EQ(dec_start.value(), false);

    dec.decode_sequence_end(dec_view);
}

TEST(UperDecoderRoundTripTest, Sequence_OneOptionalPresent_UT_UPER_DEC_081) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    const bool opts[1] = {true};
    auto enc_start = enc.encode_sequence_start<seq_one_optional_meta>(enc_view, opts, 1);
    ASSERT_TRUE(enc_start.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    bool read_opts[2] = {false, false};
    auto dec_start = dec.decode_sequence_start<seq_one_optional_meta>(dec_view, read_opts, 1);
    ASSERT_TRUE(dec_start.is_ok());
    EXPECT_TRUE(read_opts[0]);

    dec.decode_sequence_end(dec_view);
}

TEST(UperDecoderRoundTripTest, Sequence_OneOptionalAbsent_UT_UPER_DEC_082) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    const bool opts[1] = {false};
    auto enc_start = enc.encode_sequence_start<seq_one_optional_meta>(enc_view, opts, 1);
    ASSERT_TRUE(enc_start.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    bool read_opts[2] = {false, false};
    auto dec_start = dec.decode_sequence_start<seq_one_optional_meta>(dec_view, read_opts, 1);
    ASSERT_TRUE(dec_start.is_ok());
    EXPECT_FALSE(read_opts[0]);

    dec.decode_sequence_end(dec_view);
}

TEST(UperDecoderRoundTripTest, Sequence_WithExtension_UT_UPER_DEC_083) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    const bool opts[1] = {false};
    auto enc_start = enc.encode_sequence_start<seq_ext_optional_meta>(enc_view, opts, 1);
    ASSERT_TRUE(enc_start.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    bool read_opts[2] = {false, false};
    auto dec_start = dec.decode_sequence_start<seq_ext_optional_meta>(dec_view, read_opts, 1);
    ASSERT_TRUE(dec_start.is_ok());
    // Extension bit = 0, optional bitmap bit = 0
    EXPECT_EQ(dec_start.value(), false);
    EXPECT_FALSE(read_opts[0]);

    dec.decode_sequence_end(dec_view);
}

// ============================================================================
// 11. Round-trip: CHOICE
// ============================================================================

TEST(UperDecoderRoundTripTest, Choice_4Alternatives_Index2_UT_UPER_DEC_090) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto enc_r = enc.encode_choice_index<choice_4_meta>(2, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_choice_index<choice_4_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 2);
}

TEST(UperDecoderRoundTripTest, Choice_Index0_UT_UPER_DEC_091) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto enc_r = enc.encode_choice_index<choice_4_meta>(0, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_choice_index<choice_4_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 0);
}

TEST(UperDecoderRoundTripTest, Choice_Index3_UT_UPER_DEC_092) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto enc_r = enc.encode_choice_index<choice_4_meta>(3, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_choice_index<choice_4_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 3);
}

// ============================================================================
// 12. Round-trip: SEQUENCE OF length
// ============================================================================

TEST(UperDecoderRoundTripTest, SequenceOf_Length5_UT_UPER_DEC_100) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto enc_r = enc.encode_sequence_of_length<seqof_256_meta>(5, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_sequence_of_length<seqof_256_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 5u);
}

TEST(UperDecoderRoundTripTest, SequenceOf_Length0_UT_UPER_DEC_101) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto enc_r = enc.encode_sequence_of_length<seqof_256_meta>(0, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_sequence_of_length<seqof_256_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 0u);
}

TEST(UperDecoderRoundTripTest, SequenceOf_Edges_UT_UPER_DEC_102) {
    // Min and max edge values: 0..255 range
    for (size_t len : {0u, 255u}) {
        auto enc_buf = make_mutable_buffer(16);
        auto enc_view = make_view(enc_buf);
        uper_encoder enc;
        enc.set_bit_offset(0);

        auto enc_r = enc.encode_sequence_of_length<seqof_256_meta>(len, enc_view);
        ASSERT_TRUE(enc_r.is_ok()) << "Encode failed for len=" << len;
        enc.flush(enc_view);

        auto dec_view = make_view(enc_buf);
        uper_decoder dec;
        dec.set_bit_offset(0);

        auto dec_r = dec.decode_sequence_of_length<seqof_256_meta>(dec_view);
        ASSERT_TRUE(dec_r.is_ok()) << "Decode failed for len=" << len;
        EXPECT_EQ(dec_r.value(), len);
    }
}

// ============================================================================
// 13. flush — verify padding zeros
// ============================================================================

TEST(UperDecoderFlushTest, FlushAtOctetBoundary_UT_UPER_DEC_110) {
    auto enc_buf = make_mutable_buffer(8);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    enc.encode_boolean(true, enc_view);
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_boolean(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_TRUE(dec_r.value());

    auto flush_r = dec.flush(dec_view);
    ASSERT_TRUE(flush_r.is_ok());
}

// ============================================================================
// 14. Compact encoding comparison — decode matches known encoder output
// ============================================================================

TEST(UperDecoderCompactTest, Integer_0_255_Compact_UT_UPER_DEC_120) {
    // Pre-compute: INTEGER(0..255) value 42 = 0x2A = 00101010
    auto enc_buf = make_mutable_buffer(8);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    enc.encode_integer<uint8_range_meta>(42, enc_view);
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_integer<uint8_range_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 42);

    // Verify exact encoded byte
    EXPECT_EQ(enc_buf[0], 0x2A);
}

TEST(UperDecoderCompactTest, TwoSmallIntegersCompact_UT_UPER_DEC_121) {
    // Encode two consecutive small integers: 1..10 value 5 (4 bits) + 1..10 value 8 (4 bits)
    // This must fit into 1 byte (8 bits total), unlike aligned PER which would add padding
    auto enc_buf = make_mutable_buffer(4);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    auto r1 = enc.encode_integer<small_range_meta>(5, enc_view);
    ASSERT_TRUE(r1.is_ok());
    EXPECT_EQ(enc.bit_offset(), 4u);

    auto r2 = enc.encode_integer<small_range_meta>(8, enc_view);
    ASSERT_TRUE(r2.is_ok());
    EXPECT_EQ(enc.bit_offset(), 8u);

    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto d1 = dec.decode_integer<small_range_meta>(dec_view);
    ASSERT_TRUE(d1.is_ok());
    EXPECT_EQ(d1.value(), 5);

    auto d2 = dec.decode_integer<small_range_meta>(dec_view);
    ASSERT_TRUE(d2.is_ok());
    EXPECT_EQ(d2.value(), 8);
}

// ============================================================================
// 15. Error handling tests
// ============================================================================

TEST(UperDecoderErrorTest, BufferUnderflow_Integer_UT_UPER_DEC_130) {
    auto enc_buf = make_mutable_buffer(1);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    // Write only 3 bits for a value needing 8 bits for range
    enc.encode_boolean(true, enc_view);
    enc.encode_boolean(false, enc_view);
    enc.encode_boolean(true, enc_view);
    enc.flush(enc_view);

    // Now try to decode an INTEGER needing 8 bits from insufficient data
    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    // Skip booleans
    dec.decode_boolean(dec_view);
    dec.decode_boolean(dec_view);
    dec.decode_boolean(dec_view);

    // Now try to read 8 bits for INTEGER(0..255) — not enough bits remaining
    auto dec_r = dec.decode_integer<uint8_range_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_err());
}

TEST(UperDecoderErrorTest, ValueOutOfRange_Enumerated_UT_UPER_DEC_131) {
    // Manually construct buffer with out-of-range enum index
    // color_enum_meta has 3 root values, bits = 2
    // Write index 3 (binary 11), which is out of range
    auto buf_vec = make_mutable_buffer(4);
    auto enc_view = make_view(buf_vec);
    size_t bit_off = 0;

    // Write raw bits: index 3 (binary 11)
    write_bits(const_cast<uint8_t*>(enc_view.data()), bit_off, 3, 2);

    auto dec_view = make_view(buf_vec);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_enumerated<color_enum_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_err());
}

TEST(UperDecoderErrorTest, BufferUnderflow_OctetString_UT_UPER_DEC_132) {
#ifndef ASN1PP_EMBEDDED
    // Buffer too small for fixed-size octet string
    auto buf_vec = make_mutable_buffer(1);
    auto dec_view = make_view(buf_vec);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_octet_string<fixed_octets_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_err());
#endif // ASN1PP_EMBEDDED
}

// ============================================================================
// 16. OID round-trip
// ============================================================================

TEST(UperDecoderRoundTripTest, OidRoundTrip_UT_UPER_DEC_140) {
    auto enc_buf = make_mutable_buffer(16);
    auto enc_view = make_view(enc_buf);
    uper_encoder enc;
    enc.set_bit_offset(0);

    // Pre-encoded OID: 1.2.840.113549 → encoded subidentifiers
    std::vector<uint8_t> encoded_oid = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D};
    auto enc_r = enc.encode_oid(std::span<const uint8_t>(encoded_oid), enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    enc.flush(enc_view);

    auto dec_view = make_view(enc_buf);
    uper_decoder dec;
    dec.set_bit_offset(0);

    auto dec_r = dec.decode_oid(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), encoded_oid);
}
