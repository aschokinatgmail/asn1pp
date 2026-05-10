#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>
#include <cstring>

#include "codec/result.hpp"
#include "codec/per/uper_encoder.hpp"

using namespace asn1pp;
using namespace asn1pp::per;

// ============================================================================
// PER Metadata structs (same pattern as per_encoder_test.cpp)
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

std::vector<uint8_t> used_bytes(const std::vector<uint8_t>& buf, size_t bit_count) {
    size_t byte_count = (bit_count + 7) / 8;
    return std::vector<uint8_t>(buf.begin(), buf.begin() + static_cast<long>(byte_count));
}

} // namespace

// ============================================================================
// 1. bits_needed_for_range / bits_needed_unsigned (same as aligned)
// ============================================================================

TEST(BitsNeededTest, Range0_255_UT_UPER_ENC_001) {
    EXPECT_EQ(uper_encoder::bits_needed_for_range(0, 255), 8);
}

TEST(BitsNeededTest, Range0_65535_UT_UPER_ENC_002) {
    EXPECT_EQ(uper_encoder::bits_needed_for_range(0, 65535), 16);
}

TEST(BitsNeededTest, Range1_10_UT_UPER_ENC_003) {
    EXPECT_EQ(uper_encoder::bits_needed_for_range(1, 10), 4);
}

TEST(BitsNeededTest, Range0_1_UT_UPER_ENC_004) {
    EXPECT_EQ(uper_encoder::bits_needed_for_range(0, 1), 1);
}

TEST(BitsNeededTest, Range0_0_UT_UPER_ENC_005) {
    EXPECT_EQ(uper_encoder::bits_needed_for_range(0, 0), 0);
}

TEST(BitsNeededTest, BitsNeededUnsigned_UT_UPER_ENC_006) {
    EXPECT_EQ(uper_encoder::bits_needed_unsigned(0), 0);
    EXPECT_EQ(uper_encoder::bits_needed_unsigned(1), 1);
    EXPECT_EQ(uper_encoder::bits_needed_unsigned(2), 2);
    EXPECT_EQ(uper_encoder::bits_needed_unsigned(3), 2);
    EXPECT_EQ(uper_encoder::bits_needed_unsigned(7), 3);
    EXPECT_EQ(uper_encoder::bits_needed_unsigned(255), 8);
    EXPECT_EQ(uper_encoder::bits_needed_unsigned(65535), 16);
}

// ============================================================================
// 2. flush_to_octet — should pad to next octet boundary
// ============================================================================

TEST(FlushToOctetTest, AlreadyAligned_UT_UPER_ENC_010) {
    size_t bit_off = 0;
    uper_encoder::flush_to_octet(bit_off);
    EXPECT_EQ(bit_off, 0);
}

TEST(FlushToOctetTest, AlreadyAligned8_UT_UPER_ENC_011) {
    size_t bit_off = 8;
    uper_encoder::flush_to_octet(bit_off);
    EXPECT_EQ(bit_off, 8);
}

TEST(FlushToOctetTest, NotAligned_UT_UPER_ENC_012) {
    size_t bit_off = 1;
    uper_encoder::flush_to_octet(bit_off);
    EXPECT_EQ(bit_off, 8);
}

TEST(FlushToOctetTest, NotAlignedMid_UT_UPER_ENC_013) {
    size_t bit_off = 10;
    uper_encoder::flush_to_octet(bit_off);
    EXPECT_EQ(bit_off, 16);
}

TEST(FlushToOctetTest, NotAlignedSeven_UT_UPER_ENC_014) {
    size_t bit_off = 7;
    uper_encoder::flush_to_octet(bit_off);
    EXPECT_EQ(bit_off, 8);
}

// ============================================================================
// 3. Constrained whole number — unaligned
// ============================================================================

TEST(ConstrainedWholeNumberUnalignedTest, Range0_255_Value42_UT_UPER_ENC_020) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 0;

    auto r = uper_encoder::encode_constrained_whole_number_unaligned(42, 0, 255, view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(bit_off, 8);
    EXPECT_EQ(used_bytes(buf_vec, bit_off), std::vector<uint8_t>({0x2A}));
}

TEST(ConstrainedWholeNumberUnalignedTest, Range0_255_Value255_UT_UPER_ENC_021) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 0;

    auto r = uper_encoder::encode_constrained_whole_number_unaligned(255, 0, 255, view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(bit_off, 8);
    EXPECT_EQ(used_bytes(buf_vec, bit_off), std::vector<uint8_t>({0xFF}));
}

TEST(ConstrainedWholeNumberUnalignedTest, Range1_10_Value5_UT_UPER_ENC_022) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 0;

    auto r = uper_encoder::encode_constrained_whole_number_unaligned(5, 1, 10, view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(bit_off, 4);
    EXPECT_EQ(buf_vec[0] & 0xF0, 0x40);
}

TEST(ConstrainedWholeNumberUnalignedTest, Range0_1_Value0_UT_UPER_ENC_023) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 0;

    auto r = uper_encoder::encode_constrained_whole_number_unaligned(0, 0, 1, view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(bit_off, 1);
    EXPECT_EQ(buf_vec[0] & 0x80, 0x00);
}

TEST(ConstrainedWholeNumberUnalignedTest, Range0_1_Value1_UT_UPER_ENC_024) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 0;

    auto r = uper_encoder::encode_constrained_whole_number_unaligned(1, 0, 1, view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(bit_off, 1);
    EXPECT_EQ(buf_vec[0] & 0x80, 0x80);
}

TEST(ConstrainedWholeNumberUnalignedTest, Range0_65535_Value1024_UT_UPER_ENC_025) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 0;

    auto r = uper_encoder::encode_constrained_whole_number_unaligned(1024, 0, 65535, view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(bit_off, 16);
    EXPECT_EQ(used_bytes(buf_vec, bit_off), std::vector<uint8_t>({0x04, 0x00}));
}

TEST(ConstrainedWholeNumberUnalignedTest, NonZeroBitOffset_UT_UPER_ENC_026) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 3;

    auto r = uper_encoder::encode_constrained_whole_number_unaligned(42, 0, 255, view, bit_off);
    ASSERT_TRUE(r.is_ok());

    // 42 = 0x2A = 00101010, MSB first, from bit 3:
    // Bits 3-7 of byte 0 = first 5 bits: 00101 → byte0 & 0x1F = 0x05(00101)
    EXPECT_EQ(buf_vec[0] & 0x1F, 0x05U);
    // Bits 0-2 of byte 1 = last 3 bits: 010 → byte1 & 0xE0 = 0x40
    EXPECT_EQ(buf_vec[1] & 0xE0, 0x40);
    EXPECT_EQ(bit_off, 11);
}

TEST(ConstrainedWholeNumberUnalignedTest, ValueOutOfRange_UT_UPER_ENC_027) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 0;

    auto r = uper_encoder::encode_constrained_whole_number_unaligned(256, 0, 255, view, bit_off);
    ASSERT_TRUE(r.is_err());
}

TEST(ConstrainedWholeNumberUnalignedTest, LargeRange_0_100000_Value12345_UT_UPER_ENC_028) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 0;

    auto r = uper_encoder::encode_constrained_whole_number_unaligned(12345, 0, 100000, view, bit_off);
    ASSERT_TRUE(r.is_ok());

    // 12345 = 0x3039, needs 2 bytes
    // length det (unaligned) = 1 byte (value 2 = 0x02)
    // + 2 data bytes = 0x30, 0x39
    // total = 1 byte (8 bits) + 2 bytes (16 bits) = 24 bits = 3 bytes
    EXPECT_EQ(bit_off, 24);
    EXPECT_EQ(used_bytes(buf_vec, bit_off), std::vector<uint8_t>({0x02, 0x30, 0x39}));
}

// ============================================================================
// 4. Length determinant — unaligned
// ============================================================================

TEST(LengthDeterminantUnalignedTest, Length0_UT_UPER_ENC_030) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 0;

    auto r = uper_encoder::encode_length_determinant_unaligned(0, view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(bit_off, 8);
    EXPECT_EQ(buf_vec[0], 0x00);
}

TEST(LengthDeterminantUnalignedTest, Length127_UT_UPER_ENC_031) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 0;

    auto r = uper_encoder::encode_length_determinant_unaligned(127, view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(bit_off, 8);
    EXPECT_EQ(buf_vec[0], 0x7F);
}

TEST(LengthDeterminantUnalignedTest, Length128_UT_UPER_ENC_032) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 0;

    auto r = uper_encoder::encode_length_determinant_unaligned(128, view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(bit_off, 16);
    EXPECT_EQ(used_bytes(buf_vec, bit_off), std::vector<uint8_t>({0x80, 0x80}));
}

TEST(LengthDeterminantUnalignedTest, Length16383_UT_UPER_ENC_033) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 0;

    auto r = uper_encoder::encode_length_determinant_unaligned(16383, view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(bit_off, 16);
    EXPECT_EQ(used_bytes(buf_vec, bit_off), std::vector<uint8_t>({0xBF, 0xFF}));
}

TEST(LengthDeterminantUnalignedTest, NonZeroBitOffset_UT_UPER_ENC_034) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 5;

    auto r = uper_encoder::encode_length_determinant_unaligned(42, view, bit_off);
    ASSERT_TRUE(r.is_ok());

    // 42 = 0x2A = 00101010, MSB first, from bit 5:
    // 8 bits span: byte 0 bits 5-7 (3 bits: 001) + byte 1 bits 7-4 (5 bits: 01010)
    // byte 0: 001xxxx → byte0 & 0x07 = 0x01(001)
    EXPECT_EQ(buf_vec[0] & 0x07, 0x01U);
    // byte 1: 01010xxx → byte1 & 0xF8 = 0x50
    EXPECT_EQ(buf_vec[1] & 0xF8, 0x50);
}

// ============================================================================
// 5. BOOLEAN — 1 bit, no alignment after
// ============================================================================

TEST(BooleanTest, True_UT_UPER_ENC_040) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_boolean(true, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 1);
    EXPECT_EQ(buf_vec[0] & 0x80, 0x80);
}

TEST(BooleanTest, False_UT_UPER_ENC_041) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_boolean(false, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 1);
    EXPECT_EQ(buf_vec[0] & 0x80, 0x00);
}

TEST(BooleanTest, TrueThenFalse_NoAlignment_UT_UPER_ENC_042) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r1 = enc.encode_boolean(true, view);
    ASSERT_TRUE(r1.is_ok());
    EXPECT_EQ(enc.bit_offset(), 1);

    auto r2 = enc.encode_boolean(false, view);
    ASSERT_TRUE(r2.is_ok());
    EXPECT_EQ(enc.bit_offset(), 2);

    // bit 0 = 1, bit 1 = 0 → 10xxxxxx = 0x80
    EXPECT_EQ(buf_vec[0] & 0xC0, 0x80);
}

// ============================================================================
// 6. NULL — 0 bits
// ============================================================================

TEST(NullTest, EncodeNull_UT_UPER_ENC_050) {
    uper_encoder enc;
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);

    auto r = enc.encode_null(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 0);
}

// ============================================================================
// 7. INTEGER encoding via template methods
// ============================================================================

TEST(IntegerTest, Constrained0_255_Value42_UT_UPER_ENC_060) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_integer<uint8_range_meta>(42, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 8);
    EXPECT_EQ(used_bytes(buf_vec, enc.bit_offset()), std::vector<uint8_t>({0x2A}));
}

TEST(IntegerTest, Constrained1_10_Value5_UT_UPER_ENC_061) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_integer<small_range_meta>(5, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 4);
    EXPECT_EQ(buf_vec[0] & 0xF0, 0x40);
}

TEST(IntegerTest, Constrained0_1_Value1_UT_UPER_ENC_062) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_integer<bit_range_meta>(1, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 1);
    EXPECT_EQ(buf_vec[0] & 0x80, 0x80);
}

// ============================================================================
// 8. OCTET STRING
// ============================================================================

TEST(OctetStringTest, FixedSize4_UT_UPER_ENC_070) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    const uint8_t data[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    auto r = enc.encode_octet_string<fixed_octets_meta>(std::span{data}, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 32);
    EXPECT_EQ(used_bytes(buf_vec, enc.bit_offset()),
              std::vector<uint8_t>({0xDE, 0xAD, 0xBE, 0xEF}));
}

TEST(OctetStringTest, FixedSize4_WrongSize_UT_UPER_ENC_071) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    const uint8_t data[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
    auto r = enc.encode_octet_string<fixed_octets_meta>(std::span{data}, view);
    ASSERT_TRUE(r.is_err());
}

TEST(OctetStringTest, FixedSizeAfterBool_NonOctetStart_UT_UPER_ENC_072) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r1 = enc.encode_boolean(true, view);
    ASSERT_TRUE(r1.is_ok());
    EXPECT_EQ(enc.bit_offset(), 1);

    const uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};
    auto r2 = enc.encode_octet_string<fixed_octets_meta>(std::span{data}, view);
    ASSERT_TRUE(r2.is_ok());

    // 1 (bool) + 32 (data) = 33 bits
    EXPECT_EQ(enc.bit_offset(), 33);
    // byte 0: bit 7 = 1(bool), bits 6..0 = first 7 bits of 0x01 = 0000000
    // → 10000000 = 0x80
    EXPECT_EQ(buf_vec[0], 0x80);
}

TEST(OctetStringTest, ConstrainedSize3_UT_UPER_ENC_073) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    const uint8_t data[3] = {0xAA, 0xBB, 0xCC};
    auto r = enc.encode_octet_string<constrained_octets_meta>(std::span{data}, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 32);
    EXPECT_EQ(used_bytes(buf_vec, enc.bit_offset()),
              std::vector<uint8_t>({0x03, 0xAA, 0xBB, 0xCC}));
}

TEST(OctetStringTest, UnconstrainedSize2_UT_UPER_ENC_074) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    const uint8_t data[2] = {0xAA, 0xBB};
    auto r = enc.encode_octet_string<unconstrained_octets_meta>(std::span{data}, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 24);
    EXPECT_EQ(used_bytes(buf_vec, enc.bit_offset()),
              std::vector<uint8_t>({0x02, 0xAA, 0xBB}));
}

// ============================================================================
// 9. ENUMERATED
// ============================================================================

TEST(EnumeratedTest, Index0_UT_UPER_ENC_080) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_enumerated<color_enum_meta>(0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 2);
    EXPECT_EQ(buf_vec[0] & 0xC0, 0x00);
}

TEST(EnumeratedTest, Index1_UT_UPER_ENC_081) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_enumerated<color_enum_meta>(1, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 2);
    EXPECT_EQ(buf_vec[0] & 0xC0, 0x40);
}

TEST(EnumeratedTest, Index2_UT_UPER_ENC_082) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_enumerated<color_enum_meta>(2, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 2);
    EXPECT_EQ(buf_vec[0] & 0xC0, 0x80);
}

TEST(EnumeratedTest, SingleValue_Needs0Bits_UT_UPER_ENC_083) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_enumerated<single_val_enum_meta>(0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 0);
}

TEST(EnumeratedTest, InvalidIndex_UT_UPER_ENC_084) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_enumerated<color_enum_meta>(3, view);
    ASSERT_TRUE(r.is_err());
}

// ============================================================================
// 10. SEQUENCE
// ============================================================================

TEST(SequenceTest, Plain_NoExtension_NoOptionals_UT_UPER_ENC_090) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_sequence_start<seq_plain_meta>(view, nullptr, 0);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 0);
}

TEST(SequenceTest, ExtensionButNoOptionals_UT_UPER_ENC_091) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_sequence_start<seq_with_ext_meta>(view, nullptr, 0);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 1);
    EXPECT_EQ(buf_vec[0] & 0x80, 0x00);
}

TEST(SequenceTest, OneOptional_BothAbsent_UT_UPER_ENC_092) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    bool present[1] = {false};
    auto r = enc.encode_sequence_start<seq_one_optional_meta>(view, present, 1);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 1);
    EXPECT_EQ(buf_vec[0] & 0x80, 0x00);
}

TEST(SequenceTest, OneOptional_BPresent_UT_UPER_ENC_093) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    bool present[1] = {true};
    auto r = enc.encode_sequence_start<seq_one_optional_meta>(view, present, 1);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 1);
    EXPECT_EQ(buf_vec[0] & 0x80, 0x80);
}

TEST(SequenceTest, ExtensionWithOptional_UT_UPER_ENC_094) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    bool present[1] = {true};
    auto r = enc.encode_sequence_start<seq_ext_optional_meta>(view, present, 1);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 2);
    EXPECT_EQ(buf_vec[0] & 0xC0, 0x40);
}

TEST(SequenceEndTest, Noop_UT_UPER_ENC_095) {
    uper_encoder enc;
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);

    auto r = enc.encode_sequence_end(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 0);
}

// ============================================================================
// 11. CHOICE
// ============================================================================

TEST(ChoiceTest, Index0_UT_UPER_ENC_100) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_choice_index<choice_4_meta>(0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 2);
    EXPECT_EQ(buf_vec[0] & 0xC0, 0x00);
}

TEST(ChoiceTest, Index3_UT_UPER_ENC_101) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_choice_index<choice_4_meta>(3, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 2);
    EXPECT_EQ(buf_vec[0] & 0xC0, 0xC0);
}

TEST(ChoiceTest, InvalidIndex_UT_UPER_ENC_102) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_choice_index<choice_4_meta>(4, view);
    ASSERT_TRUE(r.is_err());
}

TEST(ChoiceTest, WithExtension_Index0_UT_UPER_ENC_103) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    // The encode_choice_index method writes only the index bits
    // (extension bit is handled separately by the caller/codegen).
    // For choice_ext_meta with 3 alternatives: 2 bits for index.
    auto r = enc.encode_choice_index<choice_ext_meta>(0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 2);
    EXPECT_EQ(buf_vec[0] & 0xC0, 0x00);
}

// ============================================================================
// 12. SEQUENCE OF length
// ============================================================================

TEST(SequenceOfLengthTest, Constrained256_Length10_UT_UPER_ENC_110) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_sequence_of_length<seqof_256_meta>(10, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 8);
    EXPECT_EQ(buf_vec[0], 0x0A);
}

TEST(SequenceOfLengthTest, Unconstrained_Length42_UT_UPER_ENC_111) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_sequence_of_length<seqof_unconstrained_meta>(42, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 8);
    EXPECT_EQ(buf_vec[0], 0x2A);
}

TEST(SequenceOfLengthTest, OutOfRange_UT_UPER_ENC_112) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.encode_sequence_of_length<seqof_256_meta>(256, view);
    ASSERT_TRUE(r.is_err());
}

// ============================================================================
// 13. flush() — final PDU padding
// ============================================================================

TEST(FlushTest, AlreadyByteAligned_UT_UPER_ENC_120) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    enc.encode_integer<uint8_range_meta>(42, view);
    ASSERT_EQ(enc.bit_offset(), 8);

    auto r = enc.flush(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 8);
}

TEST(FlushTest, NotByteAligned_UT_UPER_ENC_121) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    enc.encode_boolean(true, view);
    enc.encode_boolean(false, view);
    enc.encode_boolean(true, view);
    ASSERT_EQ(enc.bit_offset(), 3);

    auto r = enc.flush(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 8);
}

TEST(FlushTest, SequenceOfTwoBools_FlushTo1Byte_UT_UPER_ENC_122) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    enc.encode_boolean(true, view);
    enc.encode_boolean(false, view);
    ASSERT_EQ(enc.bit_offset(), 2);

    auto r = enc.flush(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 8);
    EXPECT_EQ(used_bytes(buf_vec, enc.bit_offset()), std::vector<uint8_t>({0x80}));
}

// ============================================================================
// 14. Multi-field UPER compactness
// ============================================================================

TEST(UperCompactnessTest, SequenceOfTwoBools_1BytePdu_UT_UPER_ENC_130) {
    uper_encoder enc;
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);

    enc.encode_sequence_start<seq_plain_meta>(view, nullptr, 0);
    enc.encode_boolean(true, view);
    enc.encode_boolean(false, view);
    enc.encode_sequence_end(view);
    enc.flush(view);

    EXPECT_EQ(enc.bit_offset(), 8);
    EXPECT_EQ(used_bytes(buf_vec, enc.bit_offset()), std::vector<uint8_t>({0x80}));
}

TEST(UperCompactnessTest, Int0to7_ThenBool_1Byte_UT_UPER_ENC_131) {
    uper_encoder enc;
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);

    enc.encode_sequence_start<seq_plain_meta>(view, nullptr, 0);

    // INTEGER(0..7, value 5): 3 bits (101)
    size_t tmp_bit = 0;
    auto r1 = uper_encoder::encode_constrained_whole_number_unaligned(5, 0, 7, view, tmp_bit);
    ASSERT_TRUE(r1.is_ok());
    enc.set_bit_offset(tmp_bit);
    EXPECT_EQ(tmp_bit, 3);

    // BOOLEAN(true): 1 bit at bit 3
    enc.encode_boolean(true, view);
    EXPECT_EQ(enc.bit_offset(), 4);

    enc.encode_sequence_end(view);
    enc.flush(view);

    EXPECT_EQ(enc.bit_offset(), 8);
    EXPECT_EQ(used_bytes(buf_vec, enc.bit_offset()), std::vector<uint8_t>({0xB0}));
}

TEST(UperCompactnessTest, ThreeSmallFields_1Byte_UT_UPER_ENC_132) {
    uper_encoder enc;
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);

    enc.encode_sequence_start<seq_plain_meta>(view, nullptr, 0);

    // Field 1: INTEGER(0..7) = 2 (010) → 3 bits
    size_t tmp = 0;
    uper_encoder::encode_constrained_whole_number_unaligned(2, 0, 7, view, tmp);
    enc.set_bit_offset(tmp);

    // Field 2: BOOLEAN(false) → 1 bit at bit 3
    enc.encode_boolean(false, view);

    // Field 3: value in 0..3, val=1 → 2 bits (01), starting at bit 4
    tmp = enc.bit_offset();
    uper_encoder::encode_constrained_whole_number_unaligned(1, 0, 3, view, tmp);
    enc.set_bit_offset(tmp);

    enc.encode_sequence_end(view);
    enc.flush(view);

    EXPECT_EQ(enc.bit_offset(), 8);
    EXPECT_EQ(used_bytes(buf_vec, enc.bit_offset()), std::vector<uint8_t>({0x44}));
}

// ============================================================================
// 15. Large range INT
// ============================================================================

TEST(LargeRangeTest, Range0_100000_Value42_UT_UPER_ENC_140) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 0;

    auto r = uper_encoder::encode_constrained_whole_number_unaligned(42, 0, 100000, view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(bit_off, 16);
    EXPECT_EQ(used_bytes(buf_vec, bit_off), std::vector<uint8_t>({0x01, 0x2A}));
}

// ============================================================================
// 16. bit_offset() / set_bit_offset()
// ============================================================================

TEST(BitOffsetAccessorTest, DefaultZero_UT_UPER_ENC_150) {
    uper_encoder enc;
    EXPECT_EQ(enc.bit_offset(), 0);
}

TEST(BitOffsetAccessorTest, SetAndGet_UT_UPER_ENC_151) {
    uper_encoder enc;
    enc.set_bit_offset(42);
    EXPECT_EQ(enc.bit_offset(), 42);
}

// ============================================================================
// 17. Empty PDU flush
// ============================================================================

TEST(EmptyPduFlushTest, NothingEncoded_FlushNoOp_UT_UPER_ENC_160) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    uper_encoder enc;

    auto r = enc.flush(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(enc.bit_offset(), 0);
}

// ============================================================================
// 18. Bool + Int + Bool compound
// ============================================================================

TEST(CompoundTest, BoolThenIntThenBool_UT_UPER_ENC_170) {
    uper_encoder enc;
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);

    enc.encode_sequence_start<seq_plain_meta>(view, nullptr, 0);

    enc.encode_boolean(true, view);
    EXPECT_EQ(enc.bit_offset(), 1);

    enc.encode_integer<uint8_range_meta>(42, view);
    EXPECT_EQ(enc.bit_offset(), 9);

    enc.encode_boolean(false, view);
    EXPECT_EQ(enc.bit_offset(), 10);

    enc.encode_sequence_end(view);
    enc.flush(view);

    EXPECT_EQ(enc.bit_offset(), 16);
    EXPECT_EQ(buf_vec[0], 0x95);
    EXPECT_EQ(buf_vec[1], 0x00);
}

// ============================================================================
// 19. Single value range
// ============================================================================

TEST(SingleRangeTest, Range0_0_Value0_UT_UPER_ENC_180) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 0;

    auto r = uper_encoder::encode_constrained_whole_number_unaligned(0, 0, 0, view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(bit_off, 0);
}

// ============================================================================
// 20. Length determinant — 4-byte form (16384 ≤ 0x3FFFFFFF)
// ============================================================================

TEST(LengthDeterminantUnalignedTest, Length16384_FourByteForm_UT_UPER_ENC_190) {
    auto buf_vec = make_mutable_buffer(8);
    auto view = make_view(buf_vec);
    size_t bit_off = 0;

    // 16384 → > 16383, ≤ 0x3FFFFFFF → 4-byte form: 0xC0004000
    auto r = uper_encoder::encode_length_determinant_unaligned(16384, view, bit_off);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(bit_off, 32);
    EXPECT_EQ(used_bytes(buf_vec, bit_off),
              std::vector<uint8_t>({0xC0, 0x00, 0x40, 0x00}));
}
