#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>
#include <cstring>

#include "codec/traits.hpp"
#include "codec/result.hpp"
#include "codec/oer/encoder.hpp"
#include "codec/oer/decoder.hpp"

using namespace asn1pp;
using namespace asn1pp::oer;

// ============================================================================
// OER Meta structs for testing
// ============================================================================

// INTEGER meta
struct uint8_int_meta {
    static constexpr bool has_range_constraint = true;
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 255;
};

struct uint16_int_meta {
    static constexpr bool has_range_constraint = true;
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 65535;
};

struct uint32_int_meta {
    static constexpr bool has_range_constraint = true;
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 4294967295;
};

struct uint64_int_meta {
    static constexpr bool has_range_constraint = true;
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = INT64_MAX;
};

struct signed_8bit_meta {
    static constexpr bool has_range_constraint = true;
    static constexpr int64_t min_value = -128;
    static constexpr int64_t max_value = 127;
};

struct unconstrained_int_meta {
    static constexpr bool has_range_constraint = false;
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = INT64_MAX;
};

// OCTET STRING meta
struct fixed_octets_meta {
    static constexpr bool has_size_constraint = true;
    static constexpr size_t min_size = 4;
    static constexpr size_t max_size = 4;
};

struct constrained_octets_meta {
    static constexpr bool has_size_constraint = true;
    static constexpr size_t min_size = 1;
    static constexpr size_t max_size = 10;
};

struct unconstrained_octets_meta {
    static constexpr bool has_size_constraint = false;
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = SIZE_MAX;
};

// BIT STRING meta
struct fixed_bits_meta {
    static constexpr bool has_size_constraint = true;
    static constexpr size_t min_size = 4;
    static constexpr size_t max_size = 4;
};

struct unconstrained_bits_meta {
    static constexpr bool has_size_constraint = false;
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = SIZE_MAX;
};

// ENUMERATED meta
struct small_enum_meta {
    static constexpr size_t root_count = 3;
    static constexpr bool has_extension = false;
};

struct large_enum_meta {
    static constexpr size_t root_count = 300;
    static constexpr bool has_extension = false;
};

struct extension_enum_meta {
    static constexpr size_t root_count = 5;
    static constexpr bool has_extension = true;
};

// SEQUENCE meta
struct seq_no_ext_no_opt_meta {
    static constexpr bool has_extension = false;
    static constexpr size_t optional_count = 0;
};

struct seq_one_opt_meta {
    static constexpr bool has_extension = false;
    static constexpr size_t optional_count = 1;
};

struct seq_multi_opt_meta {
    static constexpr bool has_extension = false;
    static constexpr size_t optional_count = 5;
};

struct seq_ext_opt_meta {
    static constexpr bool has_extension = true;
    static constexpr size_t optional_count = 2;
};

struct seq_16_opt_meta {
    static constexpr bool has_extension = false;
    static constexpr size_t optional_count = 16;
};

// CHOICE meta
struct choice_small_meta {
    static constexpr size_t alternative_count = 3;
    static constexpr bool has_extension = false;
};

struct choice_large_meta {
    static constexpr size_t alternative_count = 300;
    static constexpr bool has_extension = false;
};

// SEQUENCE OF meta
struct seqof_meta {
    static constexpr bool has_size_constraint = false;
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = SIZE_MAX;
};

// ============================================================================
// Helpers
// ============================================================================

namespace {

std::vector<uint8_t> encoded_bytes(const std::vector<uint8_t>& buf, size_t count) {
    return std::vector<uint8_t>(buf.begin(), buf.begin() + static_cast<long>(count));
}

} // namespace

// ============================================================================
// 1. INTEGER encoding (X.696 7.2)
// ============================================================================

class OerIntegerEncodeTest : public ::testing::Test {
protected:
    oer_encoder encoder_;
};

TEST_F(OerIntegerEncodeTest, Uint8_Value42_UT_OER_ENC_INT_001) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_integer<uint8_int_meta>(42, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x2A}));
}

TEST_F(OerIntegerEncodeTest, Uint8_Value0_UT_OER_ENC_INT_002) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_integer<uint8_int_meta>(0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x00}));
}

TEST_F(OerIntegerEncodeTest, Uint8_Value255_UT_OER_ENC_INT_003) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_integer<uint8_int_meta>(255, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0xFF}));
}

TEST_F(OerIntegerEncodeTest, Uint16_Value1000_UT_OER_ENC_INT_004) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_integer<uint16_int_meta>(1000, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 2), std::vector<uint8_t>({0x03, 0xE8}));
}

TEST_F(OerIntegerEncodeTest, Uint16_Value65535_UT_OER_ENC_INT_005) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_integer<uint16_int_meta>(65535, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 2), std::vector<uint8_t>({0xFF, 0xFF}));
}

TEST_F(OerIntegerEncodeTest, Uint32_Value100000_UT_OER_ENC_INT_006) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_integer<uint32_int_meta>(100000, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 4), std::vector<uint8_t>({0x00, 0x01, 0x86, 0xA0}));
}

TEST_F(OerIntegerEncodeTest, Uint64_ValueLarge_UT_OER_ENC_INT_007) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_integer<uint64_int_meta>(INT64_MAX, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 8),
        std::vector<uint8_t>({0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}));
}

TEST_F(OerIntegerEncodeTest, Signed8bit_NegativeOne_UT_OER_ENC_INT_008) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_integer<signed_8bit_meta>(-1, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0xFF}));
}

TEST_F(OerIntegerEncodeTest, Signed8bit_Negative128_UT_OER_ENC_INT_009) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_integer<signed_8bit_meta>(-128, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x80}));
}

TEST_F(OerIntegerEncodeTest, Unconstrained_Value42_UT_OER_ENC_INT_010) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_integer<unconstrained_int_meta>(42, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 2), std::vector<uint8_t>({0x01, 0x2A}));
}

TEST_F(OerIntegerEncodeTest, Unconstrained_ValueNegative1_UT_OER_ENC_INT_011) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_integer<unconstrained_int_meta>(-1, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 2), std::vector<uint8_t>({0x01, 0xFF}));
}

TEST_F(OerIntegerEncodeTest, Unconstrained_Value0_UT_OER_ENC_INT_012) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_integer<unconstrained_int_meta>(0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 2), std::vector<uint8_t>({0x01, 0x00}));
}

TEST_F(OerIntegerEncodeTest, Unconstrained_Value256_UT_OER_ENC_INT_013) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_integer<unconstrained_int_meta>(256, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 3), std::vector<uint8_t>({0x02, 0x01, 0x00}));
}

TEST_F(OerIntegerEncodeTest, Constrained_ValueOutOfRange_UT_OER_ENC_INT_014) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_integer<uint8_int_meta>(256, view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::value_out_of_range);
}

TEST_F(OerIntegerEncodeTest, BufferOverflow_UT_OER_ENC_INT_015) {
    std::vector<uint8_t> buf(0);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_integer<uint8_int_meta>(42, view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::buffer_overflow);
}

// ============================================================================
// 2. BOOLEAN encoding (X.696 7.3)
// ============================================================================

class OerBooleanEncodeTest : public ::testing::Test {
protected:
    oer_encoder encoder_;
};

TEST_F(OerBooleanEncodeTest, True_UT_OER_ENC_BOOL_001) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_boolean(true, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0xFF}));
}

TEST_F(OerBooleanEncodeTest, False_UT_OER_ENC_BOOL_002) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_boolean(false, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x00}));
}

TEST_F(OerBooleanEncodeTest, BufferOverflow_UT_OER_ENC_BOOL_003) {
    std::vector<uint8_t> buf(0);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_boolean(true, view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::buffer_overflow);
}

// ============================================================================
// 3. NULL encoding (X.696 7.4)
// ============================================================================

class OerNullEncodeTest : public ::testing::Test {
protected:
    oer_encoder encoder_;
};

TEST_F(OerNullEncodeTest, Null_ZeroOctets_UT_OER_ENC_NULL_001) {
    std::vector<uint8_t> buf(8, 0);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_null(view);
    ASSERT_TRUE(r.is_ok());
}

// ============================================================================
// 4. OCTET STRING encoding (X.696 16)
// ============================================================================

class OerOctetStringEncodeTest : public ::testing::Test {
protected:
    oer_encoder encoder_;
};

TEST_F(OerOctetStringEncodeTest, FixedSize4_UT_OER_ENC_OS_001) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    uint8_t data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    auto r = encoder_.encode_octet_string<fixed_octets_meta>(
        std::span<const uint8_t>(data, 4), view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 4),
        std::vector<uint8_t>({0xAA, 0xBB, 0xCC, 0xDD}));
}

TEST_F(OerOctetStringEncodeTest, FixedSize_WrongSize_UT_OER_ENC_OS_002) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    uint8_t data[] = {0xAA, 0xBB, 0xCC};
    auto r = encoder_.encode_octet_string<fixed_octets_meta>(
        std::span<const uint8_t>(data, 3), view);
    ASSERT_TRUE(r.is_err());
}

TEST_F(OerOctetStringEncodeTest, Constrained_3bytes_UT_OER_ENC_OS_003) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    uint8_t data[] = {0x01, 0x02, 0x03};
    auto r = encoder_.encode_octet_string<constrained_octets_meta>(
        std::span<const uint8_t>(data, 3), view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 4),
        std::vector<uint8_t>({0x03, 0x01, 0x02, 0x03}));
}

TEST_F(OerOctetStringEncodeTest, Unconstrained_3bytes_UT_OER_ENC_OS_004) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    uint8_t data[] = {0x01, 0x02, 0x03};
    auto r = encoder_.encode_octet_string<unconstrained_octets_meta>(
        std::span<const uint8_t>(data, 3), view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 4),
        std::vector<uint8_t>({0x03, 0x01, 0x02, 0x03}));
}

TEST_F(OerOctetStringEncodeTest, BufferOverflow_UT_OER_ENC_OS_005) {
    std::vector<uint8_t> buf(1);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    auto r = encoder_.encode_octet_string<fixed_octets_meta>(
        std::span<const uint8_t>(data, 4), view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::buffer_overflow);
}

// ============================================================================
// 5. BIT STRING encoding (X.696 14)
// ============================================================================

class OerBitStringEncodeTest : public ::testing::Test {
protected:
    oer_encoder encoder_;
};

TEST_F(OerBitStringEncodeTest, FixedSize_NoUnusedBits_UT_OER_ENC_BS_001) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    uint8_t data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    auto r = encoder_.encode_bit_string<fixed_bits_meta>(
        std::span<const uint8_t>(data, 4), 0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 4),
        std::vector<uint8_t>({0xAA, 0xBB, 0xCC, 0xDD}));
}

TEST_F(OerBitStringEncodeTest, Unconstrained_WithUnused_UT_OER_ENC_BS_002) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    uint8_t data[] = {0xA0};
    auto r = encoder_.encode_bit_string<unconstrained_bits_meta>(
        std::span<const uint8_t>(data, 1), 4, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 3),
        std::vector<uint8_t>({0x02, 0x04, 0xA0}));
}

// ============================================================================
// 6. ENUMERATED encoding (X.696 7.7)
// ============================================================================

class OerEnumeratedEncodeTest : public ::testing::Test {
protected:
    oer_encoder encoder_;
};

TEST_F(OerEnumeratedEncodeTest, Small_Index1_UT_OER_ENC_ENUM_001) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_enumerated<small_enum_meta>(1, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x01}));
}

TEST_F(OerEnumeratedEncodeTest, Small_Index0_UT_OER_ENC_ENUM_002) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_enumerated<small_enum_meta>(0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x00}));
}

TEST_F(OerEnumeratedEncodeTest, Large_Index50_UT_OER_ENC_ENUM_003) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_enumerated<large_enum_meta>(50, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 2), std::vector<uint8_t>({0x00, 0x32}));
}

TEST_F(OerEnumeratedEncodeTest, Extension_Index2_UT_OER_ENC_ENUM_004) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_enumerated<extension_enum_meta>(2, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x02}));
}

// ============================================================================
// 7. Length Determinant encoding (X.696 4)
// ============================================================================

class OerLengthDeterminantTest : public ::testing::Test {
protected:
    oer_encoder encoder_;
};

TEST_F(OerLengthDeterminantTest, ShortForm_0_UT_OER_ENC_LD_001) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_length_determinant(0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x00}));
}

TEST_F(OerLengthDeterminantTest, ShortForm_42_UT_OER_ENC_LD_002) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_length_determinant(42, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x2A}));
}

TEST_F(OerLengthDeterminantTest, ShortForm_127_UT_OER_ENC_LD_003) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_length_determinant(127, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x7F}));
}

TEST_F(OerLengthDeterminantTest, TwoByte_128_UT_OER_ENC_LD_004) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_length_determinant(128, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 2), std::vector<uint8_t>({0x80, 0x80}));
}

TEST_F(OerLengthDeterminantTest, TwoByte_16383_UT_OER_ENC_LD_005) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_length_determinant(16383, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 2), std::vector<uint8_t>({0xBF, 0xFF}));
}

TEST_F(OerLengthDeterminantTest, LongForm_16384_UT_OER_ENC_LD_006) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_length_determinant(16384, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 3), std::vector<uint8_t>({0xC0, 0x40, 0x00}));
}

// ============================================================================
// 8. SEQUENCE helpers (X.696 18)
// ============================================================================

class OerSequenceTest : public ::testing::Test {
protected:
    oer_encoder encoder_;
};

TEST_F(OerSequenceTest, NoOptional_UT_OER_ENC_SEQ_001) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_sequence_start<seq_no_ext_no_opt_meta>(
        view, nullptr, 0);
    ASSERT_TRUE(r.is_ok());
}

TEST_F(OerSequenceTest, OneOptional_Absent_UT_OER_ENC_SEQ_002) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    bool present[] = {false};
    auto r = encoder_.encode_sequence_start<seq_one_opt_meta>(
        view, present, 1);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x00}));
}

TEST_F(OerSequenceTest, OneOptional_Present_UT_OER_ENC_SEQ_003) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    bool present[] = {true};
    auto r = encoder_.encode_sequence_start<seq_one_opt_meta>(
        view, present, 1);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x80}));
}

TEST_F(OerSequenceTest, MultiOptional_Mixed_UT_OER_ENC_SEQ_004) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    bool present[] = {true, false, true, false, true};
    auto r = encoder_.encode_sequence_start<seq_multi_opt_meta>(
        view, present, 5);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0xA8}));
}

TEST_F(OerSequenceTest, Extension_Opts_UT_OER_ENC_SEQ_005) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    bool present[] = {true, false};
    auto r = encoder_.encode_sequence_start<seq_ext_opt_meta>(
        view, present, 2);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x40}));
}

TEST_F(OerSequenceTest, End_NoOp_UT_OER_ENC_SEQ_006) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_sequence_end(view);
    ASSERT_TRUE(r.is_ok());
}

TEST_F(OerSequenceTest, SixteenOptional_Bitmap_UT_OER_ENC_SEQ_007) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    bool present[16] = {};
    present[0] = true;
    present[15] = true;
    auto r = encoder_.encode_sequence_start<seq_16_opt_meta>(
        view, present, 16);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 2), std::vector<uint8_t>({0x80, 0x01}));
}

// ============================================================================
// 9. CHOICE index encoding (X.696 19)
// ============================================================================

class OerChoiceTest : public ::testing::Test {
protected:
    oer_encoder encoder_;
};

TEST_F(OerChoiceTest, Small_Index0_UT_OER_ENC_CHOICE_001) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_choice_index<choice_small_meta>(0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x00}));
}

TEST_F(OerChoiceTest, Small_Index2_UT_OER_ENC_CHOICE_002) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_choice_index<choice_small_meta>(2, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x02}));
}

TEST_F(OerChoiceTest, Large_Index5_UT_OER_ENC_CHOICE_003) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_choice_index<choice_large_meta>(5, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 2), std::vector<uint8_t>({0x00, 0x05}));
}

// ============================================================================
// 10. SEQUENCE OF length (X.696 20)
// ============================================================================

class OerSequenceOfTest : public ::testing::Test {
protected:
    oer_encoder encoder_;
};

TEST_F(OerSequenceOfTest, Length0_UT_OER_ENC_SOF_001) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_sequence_of_length<seqof_meta>(0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x00}));
}

TEST_F(OerSequenceOfTest, Length3_UT_OER_ENC_SOF_002) {
    std::vector<uint8_t> buf(8);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    auto r = encoder_.encode_sequence_of_length<seqof_meta>(3, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 1), std::vector<uint8_t>({0x03}));
}

// ============================================================================
// 11. OBJECT IDENTIFIER encoding (X.696 7.6)
// ============================================================================

class OerOidTest : public ::testing::Test {
protected:
    oer_encoder encoder_;
};

TEST_F(OerOidTest, PreEncodedOid_UT_OER_ENC_OID_001) {
    std::vector<uint8_t> buf(16);
    buffer_view view(static_cast<std::span<uint8_t>>(buf));
    uint8_t encoded[] = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B};
    auto r = encoder_.encode_oid(
        std::span<const uint8_t>(encoded, 9), view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(encoded_bytes(buf, 10),
        std::vector<uint8_t>({0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B}));
}

// ============================================================================
// 12. OER DECODER tests
// ============================================================================

class OerIntegerDecodeTest : public ::testing::Test {
protected:
    oer_decoder decoder_;
};

TEST_F(OerIntegerDecodeTest, DecodeUint8_42_UT_OER_DEC_INT_001) {
    uint8_t encoded[] = {0x2A};
    buffer_view view(encoded, 1);
    auto r = decoder_.decode_integer<uint8_int_meta>(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 42);
    EXPECT_EQ(view.size(), 0u);
}

TEST_F(OerIntegerDecodeTest, DecodeUint8_0_UT_OER_DEC_INT_002) {
    uint8_t encoded[] = {0x00};
    buffer_view view(encoded, 1);
    auto r = decoder_.decode_integer<uint8_int_meta>(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0);
}

TEST_F(OerIntegerDecodeTest, DecodeUint16_1000_UT_OER_DEC_INT_003) {
    uint8_t encoded[] = {0x03, 0xE8};
    buffer_view view(encoded, 2);
    auto r = decoder_.decode_integer<uint16_int_meta>(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 1000);
}

TEST_F(OerIntegerDecodeTest, DecodeUint32_100000_UT_OER_DEC_INT_004) {
    uint8_t encoded[] = {0x00, 0x01, 0x86, 0xA0};
    buffer_view view(encoded, 4);
    auto r = decoder_.decode_integer<uint32_int_meta>(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 100000);
}

TEST_F(OerIntegerDecodeTest, DecodeUint64_Int64Max_UT_OER_DEC_INT_005) {
    uint8_t encoded[] = {0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    buffer_view view(encoded, 8);
    auto r = decoder_.decode_integer<uint64_int_meta>(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), INT64_MAX);
}

TEST_F(OerIntegerDecodeTest, DecodeSigned8_Negative1_UT_OER_DEC_INT_006) {
    uint8_t encoded[] = {0xFF};
    buffer_view view(encoded, 1);
    auto r = decoder_.decode_integer<signed_8bit_meta>(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), -1);
}

TEST_F(OerIntegerDecodeTest, DecodeUnconstrained_42_UT_OER_DEC_INT_007) {
    uint8_t encoded[] = {0x01, 0x2A};
    buffer_view view(encoded, 2);
    auto r = decoder_.decode_integer<unconstrained_int_meta>(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 42);
}

TEST_F(OerIntegerDecodeTest, DecodeUnconstrained_Negative1_UT_OER_DEC_INT_008) {
    uint8_t encoded[] = {0x01, 0xFF};
    buffer_view view(encoded, 2);
    auto r = decoder_.decode_integer<unconstrained_int_meta>(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), -1);
}

TEST_F(OerIntegerDecodeTest, DecodeUnconstrained_256_UT_OER_DEC_INT_009) {
    uint8_t encoded[] = {0x02, 0x01, 0x00};
    buffer_view view(encoded, 3);
    auto r = decoder_.decode_integer<unconstrained_int_meta>(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 256);
}

TEST_F(OerIntegerDecodeTest, BufferUnderflow_UT_OER_DEC_INT_010) {
    uint8_t encoded[] = {0x01};
    buffer_view view(encoded, 1);
    auto r = decoder_.decode_integer<unconstrained_int_meta>(view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::buffer_underflow);
}

class OerBooleanDecodeTest : public ::testing::Test {
protected:
    oer_decoder decoder_;
};

TEST_F(OerBooleanDecodeTest, DecodeTrue_UT_OER_DEC_BOOL_001) {
    uint8_t encoded[] = {0xFF};
    buffer_view view(encoded, 1);
    auto r = decoder_.decode_boolean(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_TRUE(r.value());
}

TEST_F(OerBooleanDecodeTest, DecodeFalse_UT_OER_DEC_BOOL_002) {
    uint8_t encoded[] = {0x00};
    buffer_view view(encoded, 1);
    auto r = decoder_.decode_boolean(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_FALSE(r.value());
}

class OerOctetStringDecodeTest : public ::testing::Test {
protected:
    oer_decoder decoder_;
};

TEST_F(OerOctetStringDecodeTest, DecodeFixed4_UT_OER_DEC_OS_001) {
    uint8_t encoded[] = {0xAA, 0xBB, 0xCC, 0xDD};
    buffer_view view(encoded, 4);
    auto r = decoder_.decode_octet_string<fixed_octets_meta>(view);
    ASSERT_TRUE(r.is_ok());
    const auto& data = r.value();
    EXPECT_EQ(data.size(), 4u);
    EXPECT_EQ(data[0], 0xAA);
    EXPECT_EQ(data[1], 0xBB);
    EXPECT_EQ(data[2], 0xCC);
    EXPECT_EQ(data[3], 0xDD);
}

TEST_F(OerOctetStringDecodeTest, DecodeUnconstrained_UT_OER_DEC_OS_002) {
    uint8_t encoded[] = {0x03, 0x10, 0x20, 0x30};
    buffer_view view(encoded, 4);
    auto r = decoder_.decode_octet_string<unconstrained_octets_meta>(view);
    ASSERT_TRUE(r.is_ok());
    const auto& data = r.value();
    EXPECT_EQ(data.size(), 3u);
    EXPECT_EQ(data[0], 0x10);
    EXPECT_EQ(data[1], 0x20);
    EXPECT_EQ(data[2], 0x30);
}

class OerBitStringDecodeTest : public ::testing::Test {
protected:
    oer_decoder decoder_;
};

TEST_F(OerBitStringDecodeTest, DecodeFixed_NoUnused_UT_OER_DEC_BS_001) {
    uint8_t encoded[] = {0xAA, 0xBB, 0xCC, 0xDD};
    buffer_view view(encoded, 4);
    auto r = decoder_.decode_bit_string<fixed_bits_meta>(view);
    ASSERT_TRUE(r.is_ok());
    const auto& [data, unused] = r.value();
    EXPECT_EQ(data.size(), 4u);
    EXPECT_EQ(unused, 0);
    EXPECT_EQ(data[0], 0xAA);
}

class OerEnumeratedDecodeTest : public ::testing::Test {
protected:
    oer_decoder decoder_;
};

TEST_F(OerEnumeratedDecodeTest, DecodeSmallEnum_Index1_UT_OER_DEC_ENUM_001) {
    uint8_t encoded[] = {0x01};
    buffer_view view(encoded, 1);
    auto r = decoder_.decode_enumerated<small_enum_meta>(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 1);
}

TEST_F(OerEnumeratedDecodeTest, DecodeLargeEnum_Index50_UT_OER_DEC_ENUM_002) {
    uint8_t encoded[] = {0x00, 0x32};
    buffer_view view(encoded, 2);
    auto r = decoder_.decode_enumerated<large_enum_meta>(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 50);
}

class OerChoiceDecodeTest : public ::testing::Test {
protected:
    oer_decoder decoder_;
};

TEST_F(OerChoiceDecodeTest, DecodeChoiceSmall_Index2_UT_OER_DEC_CHOICE_001) {
    uint8_t encoded[] = {0x02};
    buffer_view view(encoded, 1);
    auto r = decoder_.decode_choice_index<choice_small_meta>(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 2);
}

class OerSeqOfDecodeTest : public ::testing::Test {
protected:
    oer_decoder decoder_;
};

TEST_F(OerSeqOfDecodeTest, DecodeLength_3_UT_OER_DEC_SOF_001) {
    uint8_t encoded[] = {0x03};
    buffer_view view(encoded, 1);
    auto r = decoder_.decode_sequence_of_length<seqof_meta>(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 3u);
}

class OerDecodeLengthTest : public ::testing::Test {
protected:
    oer_decoder decoder_;
};

TEST_F(OerDecodeLengthTest, DecodeShort_42_UT_OER_DEC_LD_001) {
    uint8_t encoded[] = {0x2A};
    buffer_view view(encoded, 1);
    auto r = decoder_.decode_length_determinant(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 42u);
}

TEST_F(OerDecodeLengthTest, DecodeShort_0_UT_OER_DEC_LD_002) {
    uint8_t encoded[] = {0x00};
    buffer_view view(encoded, 1);
    auto r = decoder_.decode_length_determinant(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 0u);
}

TEST_F(OerDecodeLengthTest, DecodeTwoByte_128_UT_OER_DEC_LD_003) {
    uint8_t encoded[] = {0x80, 0x80};
    buffer_view view(encoded, 2);
    auto r = decoder_.decode_length_determinant(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 128u);
}

TEST_F(OerDecodeLengthTest, DecodeLongForm_16384_UT_OER_DEC_LD_004) {
    uint8_t encoded[] = {0xC0, 0x40, 0x00};
    buffer_view view(encoded, 3);
    auto r = decoder_.decode_length_determinant(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 16384u);
}

// ============================================================================
// 13. OER Round-Trip tests
// ============================================================================

class OerRoundTripTest : public ::testing::Test {
protected:
    oer_encoder encoder_;
    oer_decoder decoder_;
};

TEST_F(OerRoundTripTest, IntegerUint8_42_UT_OER_RT_001) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view(static_cast<std::span<uint8_t>>(buf));
    auto enc_r = encoder_.encode_integer<uint8_int_meta>(42, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    buffer_view dec_view(buf.data(), 1);
    auto dec_r = decoder_.decode_integer<uint8_int_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 42);
}

TEST_F(OerRoundTripTest, IntegerUint16_4096_UT_OER_RT_002) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view(static_cast<std::span<uint8_t>>(buf));
    auto enc_r = encoder_.encode_integer<uint16_int_meta>(4096, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    buffer_view dec_view(buf.data(), 2);
    auto dec_r = decoder_.decode_integer<uint16_int_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 4096);
}

TEST_F(OerRoundTripTest, UnconstrainedInt_777_UT_OER_RT_003) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view(static_cast<std::span<uint8_t>>(buf));
    auto enc_r = encoder_.encode_integer<unconstrained_int_meta>(777, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    buffer_view dec_view(buf.data(), 3);
    auto dec_r = decoder_.decode_integer<unconstrained_int_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 777);
}

TEST_F(OerRoundTripTest, Boolean_True_UT_OER_RT_004) {
    std::vector<uint8_t> buf(8);
    buffer_view enc_view(static_cast<std::span<uint8_t>>(buf));
    auto enc_r = encoder_.encode_boolean(true, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    buffer_view dec_view(buf.data(), 1);
    auto dec_r = decoder_.decode_boolean(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_TRUE(dec_r.value());
}

TEST_F(OerRoundTripTest, Boolean_False_UT_OER_RT_005) {
    std::vector<uint8_t> buf(8);
    buffer_view enc_view(static_cast<std::span<uint8_t>>(buf));
    auto enc_r = encoder_.encode_boolean(false, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    buffer_view dec_view(buf.data(), 1);
    auto dec_r = decoder_.decode_boolean(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_FALSE(dec_r.value());
}

TEST_F(OerRoundTripTest, OctetString_Fixed4_UT_OER_RT_006) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view(static_cast<std::span<uint8_t>>(buf));
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    auto enc_r = encoder_.encode_octet_string<fixed_octets_meta>(
        std::span<const uint8_t>(data, 4), enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    buffer_view dec_view(buf.data(), 4);
    auto dec_r = decoder_.decode_octet_string<fixed_octets_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    const auto& result = dec_r.value();
    EXPECT_EQ(result.size(), 4u);
    EXPECT_EQ(result[0], 0xDE);
    EXPECT_EQ(result[1], 0xAD);
    EXPECT_EQ(result[2], 0xBE);
    EXPECT_EQ(result[3], 0xEF);
}

TEST_F(OerRoundTripTest, OctetString_Unconstrained_UT_OER_RT_007) {
    std::vector<uint8_t> buf(16);
    buffer_view enc_view(static_cast<std::span<uint8_t>>(buf));
    uint8_t data[] = {0xCA, 0xFE};
    auto enc_r = encoder_.encode_octet_string<unconstrained_octets_meta>(
        std::span<const uint8_t>(data, 2), enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    buffer_view dec_view(buf.data(), 3);
    auto dec_r = decoder_.decode_octet_string<unconstrained_octets_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    const auto& result = dec_r.value();
    EXPECT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], 0xCA);
    EXPECT_EQ(result[1], 0xFE);
}

TEST_F(OerRoundTripTest, Enumerated_Small_UT_OER_RT_008) {
    std::vector<uint8_t> buf(8);
    buffer_view enc_view(static_cast<std::span<uint8_t>>(buf));
    auto enc_r = encoder_.encode_enumerated<small_enum_meta>(2, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    buffer_view dec_view(buf.data(), 1);
    auto dec_r = decoder_.decode_enumerated<small_enum_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 2);
}

TEST_F(OerRoundTripTest, ChoiceIndex_Small_UT_OER_RT_009) {
    std::vector<uint8_t> buf(8);
    buffer_view enc_view(static_cast<std::span<uint8_t>>(buf));
    auto enc_r = encoder_.encode_choice_index<choice_small_meta>(1, enc_view);
    ASSERT_TRUE(enc_r.is_ok());
    buffer_view dec_view(buf.data(), 1);
    auto dec_r = decoder_.decode_choice_index<choice_small_meta>(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 1);
}

TEST_F(OerRoundTripTest, FullIntegerRange_RoundTrip_UT_OER_RT_010) {
    int64_t values[] = {0, 1, 42, 127, 128, 255, 256, 1000, 65535, 65536, 100000};
    for (int64_t val : values) {
        std::vector<uint8_t> buf(16);
        buffer_view enc_view(static_cast<std::span<uint8_t>>(buf));
        auto enc_r = encoder_.encode_integer<unconstrained_int_meta>(val, enc_view);
        ASSERT_TRUE(enc_r.is_ok()) << "Failed encoding " << val;
        buffer_view dec_view(buf.data(), 16);
        auto dec_r = decoder_.decode_integer<unconstrained_int_meta>(dec_view);
        ASSERT_TRUE(dec_r.is_ok()) << "Failed decoding " << val;
        EXPECT_EQ(dec_r.value(), val) << "Round-trip mismatch for " << val;
    }
}

TEST_F(OerRoundTripTest, NegativeIntegerValues_UT_OER_RT_011) {
    int64_t values[] = {-1, -42, -128, -256, -1000, -65536};
    for (int64_t val : values) {
        std::vector<uint8_t> buf(16);
        buffer_view enc_view(static_cast<std::span<uint8_t>>(buf));
        auto enc_r = encoder_.encode_integer<unconstrained_int_meta>(val, enc_view);
        ASSERT_TRUE(enc_r.is_ok()) << "Failed encoding " << val;
        buffer_view dec_view(buf.data(), 16);
        auto dec_r = decoder_.decode_integer<unconstrained_int_meta>(dec_view);
        ASSERT_TRUE(dec_r.is_ok()) << "Failed decoding " << val;
        EXPECT_EQ(dec_r.value(), val) << "Round-trip mismatch for " << val;
    }
}

TEST_F(OerRoundTripTest, LengthDeterminant_RoundTrip_UT_OER_RT_012) {
    size_t lengths[] = {0, 1, 42, 127, 128, 256, 1000, 16383, 16384, 50000};
    for (size_t len : lengths) {
        std::vector<uint8_t> buf(8);
        buffer_view enc_view(static_cast<std::span<uint8_t>>(buf));
        auto enc_r = encoder_.encode_length_determinant(len, enc_view);
        ASSERT_TRUE(enc_r.is_ok()) << "Failed encoding length " << len;
        buffer_view dec_view(buf.data(), 8);
        auto dec_r = decoder_.decode_length_determinant(dec_view);
        ASSERT_TRUE(dec_r.is_ok()) << "Failed decoding length " << len;
        EXPECT_EQ(dec_r.value(), len) << "Length round-trip mismatch for " << len;
    }
}
