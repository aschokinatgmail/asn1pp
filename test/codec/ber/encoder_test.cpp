#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>
#include <numeric>

#include "codec/traits.hpp"
#include "codec/result.hpp"
#include "codec/ber/tlv.hpp"
#include "codec/ber/encoder.hpp"

using namespace asn1pp;
using namespace asn1pp::ber;

// ============================================================================
// Helpers
// ============================================================================

namespace {

buffer_view make_mutable_view(std::vector<uint8_t>& vec) {
    return buffer_view(vec);
}

buffer_view make_view(const std::vector<uint8_t>& vec) {
    return buffer_view(vec.data(), vec.size());
}

std::vector<uint8_t> remaining_bytes(const std::vector<uint8_t>& buf, size_t encoded_size) {
    return std::vector<uint8_t>(buf.begin(), buf.begin() + static_cast<long>(encoded_size));
}

} // namespace

// ============================================================================
// 1. INTEGER encoding (X.690 §8.3)
// ============================================================================

class IntegerEncodingTest : public ::testing::Test {
protected:
    ber_encoder encoder_;
};

TEST_F(IntegerEncodingTest, EncodeZero_UT_BER_ENC_001) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x02, 0x01, 0x00}));
}

TEST_F(IntegerEncodingTest, Encode127_UT_BER_ENC_002) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(127, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x02, 0x01, 0x7F}));
}

TEST_F(IntegerEncodingTest, Encode128_UT_BER_ENC_003) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(128, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 4), std::vector<uint8_t>({0x02, 0x02, 0x00, 0x80}));
}

TEST_F(IntegerEncodingTest, Encode256_UT_BER_ENC_004) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(256, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 4), std::vector<uint8_t>({0x02, 0x02, 0x01, 0x00}));
}

TEST_F(IntegerEncodingTest, EncodeNegativeOne_UT_BER_ENC_005) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(-1, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x02, 0x01, 0xFF}));
}

TEST_F(IntegerEncodingTest, EncodeNegative128_UT_BER_ENC_006) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(-128, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x02, 0x01, 0x80}));
}

TEST_F(IntegerEncodingTest, EncodeNegative129_UT_BER_ENC_007) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(-129, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 4), std::vector<uint8_t>({0x02, 0x02, 0xFF, 0x7F}));
}

TEST_F(IntegerEncodingTest, EncodeLargePositive_MaxInt32_UT_BER_ENC_008) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(0x7FFFFFFF, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 6), std::vector<uint8_t>({0x02, 0x04, 0x7F, 0xFF, 0xFF, 0xFF}));
}

TEST_F(IntegerEncodingTest, EncodeLargeNegative_MinInt32_UT_BER_ENC_009) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(static_cast<int64_t>(static_cast<int32_t>(0x80000000)), view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 6), std::vector<uint8_t>({0x02, 0x04, 0x80, 0x00, 0x00, 0x00}));
}

TEST_F(IntegerEncodingTest, Encode255_UT_BER_ENC_010) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(255, view);
    ASSERT_TRUE(r.is_ok());
    // 255 = 0xFF, needs leading 0x00 to distinguish from negative
    EXPECT_EQ(remaining_bytes(buf, 4), std::vector<uint8_t>({0x02, 0x02, 0x00, 0xFF}));
}

// ============================================================================
// 2. BOOLEAN encoding (X.690 §8.2)
// ============================================================================

class BooleanEncodingTest : public ::testing::Test {
protected:
    ber_encoder encoder_;
};

TEST_F(BooleanEncodingTest, EncodeTrue_UT_BER_ENC_011) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_boolean(true, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x01, 0x01, 0xFF}));
}

TEST_F(BooleanEncodingTest, EncodeFalse_UT_BER_ENC_012) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_boolean(false, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x01, 0x01, 0x00}));
}

// ============================================================================
// 3. NULL encoding (X.690 §8.8)
// ============================================================================

class NullEncodingTest : public ::testing::Test {
protected:
    ber_encoder encoder_;
};

TEST_F(NullEncodingTest, EncodeNull_UT_BER_ENC_013) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_null(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 2), std::vector<uint8_t>({0x05, 0x00}));
}

// ============================================================================
// 4. OCTET STRING encoding (X.690 §8.7)
// ============================================================================

class OctetStringEncodingTest : public ::testing::Test {
protected:
    ber_encoder encoder_;
};

TEST_F(OctetStringEncodingTest, EncodeEmpty_UT_BER_ENC_014) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    std::vector<uint8_t> empty;
    auto r = encoder_.encode_octet_string(empty, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 2), std::vector<uint8_t>({0x04, 0x00}));
}

TEST_F(OctetStringEncodingTest, EncodeTest_UT_BER_ENC_015) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    const std::vector<uint8_t> data = {'t', 'e', 's', 't'};
    auto r = encoder_.encode_octet_string(data, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 6), std::vector<uint8_t>({0x04, 0x04, 't', 'e', 's', 't'}));
}

TEST_F(OctetStringEncodingTest, EncodeLongStringLongFormLength_UT_BER_ENC_016) {
    // 200 bytes → length 0x81 0xC8 (long form: 128 + 200)
    const size_t N = 200;
    std::vector<uint8_t> data(N);
    std::iota(data.begin(), data.end(), 0x00);

    std::vector<uint8_t> buf(4 + N);  // tag(1) + length(2) + data(200)
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_octet_string(data, view);
    ASSERT_TRUE(r.is_ok());

    std::vector<uint8_t> expected;
    expected.push_back(0x04);          // tag
    expected.push_back(0x81);          // long form, 1 len byte
    expected.push_back(0xC8);          // 200
    expected.insert(expected.end(), data.begin(), data.end());

    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(buf[i], expected[i]) << "Mismatch at byte " << i;
    }
}

// ============================================================================
// 5. BIT STRING encoding (X.690 §8.6)
// ============================================================================

class BitStringEncodingTest : public ::testing::Test {
protected:
    ber_encoder encoder_;
};

TEST_F(BitStringEncodingTest, EncodeSingleBitSet_UT_BER_ENC_017) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    const std::vector<uint8_t> data = {0x80};
    auto r = encoder_.encode_bit_string(data, 7, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 4), std::vector<uint8_t>({0x03, 0x02, 0x07, 0x80}));
}

TEST_F(BitStringEncodingTest, EncodeFullByteNoUnused_UT_BER_ENC_018) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    const std::vector<uint8_t> data = {0xFF};
    auto r = encoder_.encode_bit_string(data, 0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 4), std::vector<uint8_t>({0x03, 0x02, 0x00, 0xFF}));
}

TEST_F(BitStringEncodingTest, EncodeEmpty_UT_BER_ENC_019) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    const std::vector<uint8_t> empty;
    auto r = encoder_.encode_bit_string(empty, 0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x03, 0x01, 0x00}));
}

TEST_F(BitStringEncodingTest, EncodeMultiByteWithUnused_UT_BER_ENC_020) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    const std::vector<uint8_t> data = {0x0A, 0x3B};  // ~5 bits unused in last byte
    auto r = encoder_.encode_bit_string(data, 4, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 5), std::vector<uint8_t>({0x03, 0x03, 0x04, 0x0A, 0x3B}));
}

// ============================================================================
// 6. ENUMERATED encoding (X.690 §8.4)
// ============================================================================

class EnumeratedEncodingTest : public ::testing::Test {
protected:
    ber_encoder encoder_;
};

TEST_F(EnumeratedEncodingTest, EncodeZero_UT_BER_ENC_021) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_enumerated(0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x0A, 0x01, 0x00}));
}

TEST_F(EnumeratedEncodingTest, Encode42_UT_BER_ENC_022) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_enumerated(42, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x0A, 0x01, 0x2A}));
}

TEST_F(EnumeratedEncodingTest, EncodeLarge_UT_BER_ENC_023) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_enumerated(1000, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 4), std::vector<uint8_t>({0x0A, 0x02, 0x03, 0xE8}));
}

// ============================================================================
// 7. SEQUENCE encoding (X.690 §8.9)
// ============================================================================

class SequenceEncodingTest : public ::testing::Test {
protected:
    ber_encoder encoder_;
};

TEST_F(SequenceEncodingTest, EncodeEmptySequence_UT_BER_ENC_024) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_sequence_header(
        make_universal(universal_tag::sequence, true), 0, view);
    ASSERT_TRUE(r.is_ok());
    // SEQUENCE tag (0x30) + length 0 = 0x30 0x00
    EXPECT_EQ(remaining_bytes(buf, 2), std::vector<uint8_t>({0x30, 0x00}));
}

TEST_F(SequenceEncodingTest, EncodeSequenceWithOneInteger_UT_BER_ENC_025) {
    // The sequence header and integer are encoded separately, then concatenated.
    // SEQ header: 30 03 (content = 3 bytes: 02 01 2A)
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    auto r_seq = encoder_.encode_sequence_header(
        make_universal(universal_tag::sequence, true), 3, view);
    ASSERT_TRUE(r_seq.is_ok());

    auto r_int = encoder_.encode_integer(42, view);
    ASSERT_TRUE(r_int.is_ok());

    EXPECT_EQ(remaining_bytes(buf, 5),
              std::vector<uint8_t>({0x30, 0x03, 0x02, 0x01, 0x2A}));
}

TEST_F(SequenceEncodingTest, EncodeSequenceWithBooleanAndInteger_UT_BER_ENC_026) {
    // Content: BOOLEAN(true) = 01 01 FF (3 bytes) + INTEGER(5) = 02 01 05 (3 bytes)
    // Total content = 6 bytes
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    auto r_seq = encoder_.encode_sequence_header(
        make_universal(universal_tag::sequence, true), 6, view);
    ASSERT_TRUE(r_seq.is_ok());

    auto r_bool = encoder_.encode_boolean(true, view);
    ASSERT_TRUE(r_bool.is_ok());

    auto r_int = encoder_.encode_integer(5, view);
    ASSERT_TRUE(r_int.is_ok());

    EXPECT_EQ(remaining_bytes(buf, 8),
              std::vector<uint8_t>({0x30, 0x06, 0x01, 0x01, 0xFF, 0x02, 0x01, 0x05}));
}

TEST_F(SequenceEncodingTest, EncodeSequenceWithOctetString_UT_BER_ENC_027) {
    // Content: OCTET STRING "AB" = 04 02 41 42 (4 bytes)
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    auto r_seq = encoder_.encode_sequence_header(
        make_universal(universal_tag::sequence, true), 4, view);
    ASSERT_TRUE(r_seq.is_ok());

    const std::vector<uint8_t> octets = {'A', 'B'};
    auto r_oct = encoder_.encode_octet_string(octets, view);
    ASSERT_TRUE(r_oct.is_ok());

    EXPECT_EQ(remaining_bytes(buf, 6),
              std::vector<uint8_t>({0x30, 0x04, 0x04, 0x02, 'A', 'B'}));
}

TEST_F(SequenceEncodingTest, EncodeSequenceWithNestedSequence_UT_BER_ENC_028) {
    // Outer SEQUENCE { Inner SEQUENCE { INTEGER 7 } }
    // Inner: 30 03 02 01 07 = 5 bytes
    // Outer: 30 [5] + inner bytes = 7 bytes total
    std::vector<uint8_t> buf(32);
    buffer_view view = make_mutable_view(buf);

    auto r_outer = encoder_.encode_sequence_header(
        make_universal(universal_tag::sequence, true), 5, view);
    ASSERT_TRUE(r_outer.is_ok());

    auto r_inner = encoder_.encode_sequence_header(
        make_universal(universal_tag::sequence, true), 3, view);
    ASSERT_TRUE(r_inner.is_ok());

    auto r_int = encoder_.encode_integer(7, view);
    ASSERT_TRUE(r_int.is_ok());

    EXPECT_EQ(remaining_bytes(buf, 7),
              std::vector<uint8_t>({0x30, 0x05,
                                    0x30, 0x03,
                                    0x02, 0x01, 0x07}));
}

// ============================================================================
// 8. OID encoding (X.690 §8.19)
// ============================================================================

class OidEncodingTest : public ::testing::Test {
protected:
    ber_encoder encoder_;
};

TEST_F(OidEncodingTest, EncodePreencodedOid_UT_BER_ENC_029) {
    // 1.2.840.113549 → pre-encoded: 2A 86 48 86 F7 0D
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    const std::vector<uint8_t> encoded = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D};
    auto r = encoder_.encode_oid(encoded, view);
    ASSERT_TRUE(r.is_ok());

    std::vector<uint8_t> expected = {0x06, 0x06};
    expected.insert(expected.end(), encoded.begin(), encoded.end());
    EXPECT_EQ(remaining_bytes(buf, expected.size()), expected);
}

TEST_F(OidEncodingTest, EncodeEmptyOid_UT_BER_ENC_030) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    const std::vector<uint8_t> empty;
    auto r = encoder_.encode_oid(empty, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 2), std::vector<uint8_t>({0x06, 0x00}));
}

// ============================================================================
// 9. Generic encode_tlv
// ============================================================================

class GenericTlvEncodingTest : public ::testing::Test {
protected:
    ber_encoder encoder_;
};

TEST_F(GenericTlvEncodingTest, EncodeTlvInteger_UT_BER_ENC_031) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    const std::vector<uint8_t> value = {0x2A};
    auto r = encoder_.encode_tlv(make_universal(universal_tag::integer), value, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 3), std::vector<uint8_t>({0x02, 0x01, 0x2A}));
}

TEST_F(GenericTlvEncodingTest, EncodeTlvContextSpecific_UT_BER_ENC_032) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    const std::vector<uint8_t> value = {0xAA, 0xBB};
    auto r = encoder_.encode_tlv(
        make_context_specific(1, true), value, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 4), std::vector<uint8_t>({0xA1, 0x02, 0xAA, 0xBB}));
}

// ============================================================================
// 10. Buffer overflow tests
// ============================================================================

class BufferOverflowTest : public ::testing::Test {
protected:
    ber_encoder encoder_;
};

TEST_F(BufferOverflowTest, IntegerOverflowBufferTooSmall_UT_BER_ENC_033) {
    std::vector<uint8_t> buf(1);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(0, view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::buffer_overflow);
}

TEST_F(BufferOverflowTest, BooleanOverflowBufferTooSmall_UT_BER_ENC_034) {
    std::vector<uint8_t> buf(1);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_boolean(true, view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::buffer_overflow);
}

TEST_F(BufferOverflowTest, NullOverflowBufferTooSmall_UT_BER_ENC_035) {
    std::vector<uint8_t> buf(1);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_null(view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::buffer_overflow);
}

TEST_F(BufferOverflowTest, OctetStringOverflow_UT_BER_ENC_036) {
    std::vector<uint8_t> buf(3);
    buffer_view view = make_mutable_view(buf);
    const std::vector<uint8_t> data = {1, 2, 3};
    auto r = encoder_.encode_octet_string(data, view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::buffer_overflow);
}

TEST_F(BufferOverflowTest, SequenceHeaderOverflowBufferTooSmall_UT_BER_ENC_037) {
    std::vector<uint8_t> buf(1);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_sequence_header(
        make_universal(universal_tag::sequence, true), 5, view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::buffer_overflow);
}

// ============================================================================
// 11. Round-trip tests (encode → decode → verify)
// ============================================================================

class RoundTripTest : public ::testing::Test {
protected:
    ber_encoder encoder_;
};

TEST_F(RoundTripTest, IntegerRoundTrip_UT_BER_ENC_038) {
    const int64_t values[] = {0, 1, -1, 127, 128, -128, 256, -129, 1000, -1000,
                              0x7FFFFFFF, 0x7FFF, 0xFFFF};
    for (auto val : values) {
        std::vector<uint8_t> buf(32);
        buffer_view view = make_mutable_view(buf);
        auto r = encoder_.encode_integer(val, view);
        ASSERT_TRUE(r.is_ok()) << "Failed to encode " << val;

        // Decode tag
        buffer_view decode_view = make_view(buf);
        tag out_tag{};
        size_t out_len = 0;
        auto r_hdr = decode_tlv_header(decode_view, out_tag, out_len);
        ASSERT_TRUE(r_hdr.is_ok()) << "Failed to decode TLV header for " << val;
        EXPECT_EQ(out_tag, make_universal(universal_tag::integer));
        EXPECT_GT(out_len, 0U);

        // Read integer bytes from the remaining view
        std::vector<uint8_t> int_bytes(out_len);
        std::copy(decode_view.data(), decode_view.data() + out_len, int_bytes.begin());

        // Verify two's complement value
        int64_t decoded = 0;
        if (!int_bytes.empty()) {
            bool negative = (int_bytes[0] & 0x80) != 0;
            for (size_t i = 0; i < out_len; ++i) {
                decoded = (decoded << 8) | int_bytes[i];
            }
            if (negative && out_len < 8) {
                // Sign-extend
                uint64_t mask = (1ULL << (out_len * 8)) - 1;
                decoded = static_cast<int64_t>(static_cast<uint64_t>(decoded) | ~mask);
            }
        }
        EXPECT_EQ(decoded, val) << "Round-trip mismatch for " << val;
    }
}

TEST_F(RoundTripTest, BooleanRoundTrip_UT_BER_ENC_039) {
    for (bool val : {true, false}) {
        std::vector<uint8_t> buf(16);
        buffer_view view = make_mutable_view(buf);
        auto r = encoder_.encode_boolean(val, view);
        ASSERT_TRUE(r.is_ok()) << "Failed to encode boolean " << val;

        buffer_view decode_view = make_view(buf);
        tag out_tag{};
        size_t out_len = 0;
        auto r_hdr = decode_tlv_header(decode_view, out_tag, out_len);
        ASSERT_TRUE(r_hdr.is_ok());
        EXPECT_EQ(out_tag, make_universal(universal_tag::boolean));
        EXPECT_EQ(out_len, 1U);
        EXPECT_EQ(decode_view[0], val ? 0xFF : 0x00);
    }
}

TEST_F(RoundTripTest, OctetStringRoundTrip_UT_BER_ENC_040) {
    const std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o'};
    std::vector<uint8_t> buf(32);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_octet_string(data, view);
    ASSERT_TRUE(r.is_ok());

    buffer_view decode_view = make_view(buf);
    tag out_tag{};
    size_t out_len = 0;
    auto r_hdr = decode_tlv_header(decode_view, out_tag, out_len);
    ASSERT_TRUE(r_hdr.is_ok());
    EXPECT_EQ(out_tag, make_universal(universal_tag::octet_string));
    EXPECT_EQ(out_len, data.size());
    std::vector<uint8_t> decoded(decode_view.data(), decode_view.data() + out_len);
    EXPECT_EQ(decoded, data);
}

TEST_F(RoundTripTest, NullRoundTrip_UT_BER_ENC_041) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_null(view);
    ASSERT_TRUE(r.is_ok());

    buffer_view decode_view = make_view(buf);
    tag out_tag{};
    size_t out_len = 0;
    auto r_hdr = decode_tlv_header(decode_view, out_tag, out_len);
    ASSERT_TRUE(r_hdr.is_ok());
    EXPECT_EQ(out_tag, make_universal(universal_tag::null));
    EXPECT_EQ(out_len, 0U);
}

TEST_F(RoundTripTest, BitStringRoundTrip_UT_BER_ENC_042) {
    const std::vector<uint8_t> data = {0xF0, 0x0F};
    const uint8_t unused = 4;
    std::vector<uint8_t> buf(32);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_bit_string(data, unused, view);
    ASSERT_TRUE(r.is_ok());

    buffer_view decode_view = make_view(buf);
    tag out_tag{};
    size_t out_len = 0;
    auto r_hdr = decode_tlv_header(decode_view, out_tag, out_len);
    ASSERT_TRUE(r_hdr.is_ok());
    EXPECT_EQ(out_tag, make_universal(universal_tag::bit_string));
    EXPECT_GE(out_len, 1U);
    // First content byte is unused bits count
    EXPECT_EQ(decode_view[0], unused);
    // Remaining bytes are the data
    std::vector<uint8_t> decoded_data(decode_view.data() + 1, decode_view.data() + out_len);
    EXPECT_EQ(decoded_data, data);
}

TEST_F(RoundTripTest, EnumeratedRoundTrip_UT_BER_ENC_043) {
    const int64_t values[] = {0, 1, 42, 128, 1000};
    for (auto val : values) {
        std::vector<uint8_t> buf(32);
        buffer_view view = make_mutable_view(buf);
        auto r = encoder_.encode_enumerated(val, view);
        ASSERT_TRUE(r.is_ok()) << "Failed to encode enumerated " << val;

        buffer_view decode_view = make_view(buf);
        tag out_tag{};
        size_t out_len = 0;
        auto r_hdr = decode_tlv_header(decode_view, out_tag, out_len);
        ASSERT_TRUE(r_hdr.is_ok());
        EXPECT_EQ(out_tag, make_universal(universal_tag::enumerated));
        EXPECT_GT(out_len, 0U);

        // Decode integer value
        int64_t decoded = 0;
        for (size_t i = 0; i < out_len; ++i) {
            decoded = (decoded << 8) | decode_view[i];
        }
        if ((decode_view[0] & 0x80) && out_len < 8) {
            uint64_t mask = (1ULL << (out_len * 8)) - 1;
            decoded = static_cast<int64_t>(static_cast<uint64_t>(decoded) | ~mask);
        }
        EXPECT_EQ(decoded, val) << "Round-trip mismatch for enumerated " << val;
    }
}

// ============================================================================
// 12. encode_sequence_end (indefinite-length termination)
// ============================================================================

class SequenceEndTest : public ::testing::Test {
protected:
    ber_encoder encoder_;
};

TEST_F(SequenceEndTest, EncodeEndOfContent_UT_BER_ENC_044) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_sequence_end(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(remaining_bytes(buf, 2), std::vector<uint8_t>({0x00, 0x00}));
}

TEST_F(SequenceEndTest, EndOfContentOverflow_UT_BER_ENC_045) {
    std::vector<uint8_t> buf(1);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_sequence_end(view);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::buffer_overflow);
}

// ============================================================================
// 13. Tag encoding correctness checks (tag via encode_tag inside encoder)
// ============================================================================

class EncoderTagTest : public ::testing::Test {
protected:
    ber_encoder encoder_;
};

TEST_F(EncoderTagTest, IntegerTagIsUniversalPrimitive2_UT_BER_ENC_046) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_integer(42, view);
    ASSERT_TRUE(r.is_ok());
    // First byte must be 0x02 (universal class, primitive, tag 2)
    EXPECT_EQ(buf[0], 0x02);
}

TEST_F(EncoderTagTest, SequenceTagIsConstructed_UT_BER_ENC_047) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    auto r = encoder_.encode_sequence_header(
        make_universal(universal_tag::sequence, true), 0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(buf[0], 0x30);
}

TEST_F(EncoderTagTest, BitStringTagIsUniversalPrimitive3_UT_BER_ENC_048) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);
    const std::vector<uint8_t> data = {0xFF};
    auto r = encoder_.encode_bit_string(data, 0, view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(buf[0], 0x03);
}
