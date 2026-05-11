#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>
#include <utility>
#include <cstring>

#include "codec/ber/encoder.hpp"
#include "codec/ber/decoder.hpp"
#include "codec/ber/der_encoder.hpp"
#include "codec/ber/tlv.hpp"
#include "codec/traits.hpp"
#include "codec/result.hpp"
#include "buffer/buffer_view.hpp"

using namespace asn1pp;
using namespace asn1pp::ber;

// ============================================================================
// Helpers
// ============================================================================

namespace {

buffer_view make_mutable_view(std::vector<uint8_t>& vec) {
    return buffer_view(vec);
}

buffer_view make_const_view(const std::vector<uint8_t>& vec) {
    return buffer_view(vec.data(), vec.size());
}

/// Returns the first N bytes from buf (the encoded portion).
std::vector<uint8_t> encoded_prefix(const std::vector<uint8_t>& buf, size_t total_size) {
    // The encoder advances the view, so the unused suffix is at the end.
    // We need to return what was written: buf[0..total_size-1].
    return std::vector<uint8_t>(
        buf.begin(),
        buf.begin() + static_cast<long>(total_size));
}

/// How many bytes were consumed from a buffer_view (initial - remaining).
size_t consumed_bytes(size_t initial_size, const buffer_view& remaining) {
    return initial_size - remaining.size();
}

} // namespace

// ============================================================================
// 1. INTEGER round-trip: BER encode → BER decode → original value matches
// ============================================================================

class IntegerRoundtripTest : public ::testing::Test {
protected:
    ber_encoder enc_;
    ber_decoder dec_;
};

TEST_F(IntegerRoundtripTest, Zero_Roundtrip_IT_DER_001) {
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_integer(0, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_integer(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 0);
}

TEST_F(IntegerRoundtripTest, Positive_42_Roundtrip_IT_DER_002) {
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_integer(42, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_integer(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 42);
}

TEST_F(IntegerRoundtripTest, Positive_255_Roundtrip_IT_DER_003) {
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_integer(255, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_integer(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 255);
}

TEST_F(IntegerRoundtripTest, Positive_65536_Roundtrip_IT_DER_004) {
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_integer(65536, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_integer(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 65536);
}

TEST_F(IntegerRoundtripTest, Negative_Minus1_Roundtrip_IT_DER_005) {
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_integer(-1, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_integer(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), -1);
}

TEST_F(IntegerRoundtripTest, Negative_Minus128_Roundtrip_IT_DER_006) {
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_integer(-128, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_integer(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), -128);
}

TEST_F(IntegerRoundtripTest, Negative_Minus32768_Roundtrip_IT_DER_007) {
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_integer(-32768, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_integer(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), -32768);
}

TEST_F(IntegerRoundtripTest, Int64Max_Roundtrip_IT_DER_008) {
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_integer(INT64_MAX, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_integer(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), INT64_MAX);
}

TEST_F(IntegerRoundtripTest, Int64Min_Roundtrip_IT_DER_009) {
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_integer(INT64_MIN, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_integer(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), INT64_MIN);
}

// ============================================================================
// 2. BOOLEAN round-trip: BER encode → BER decode → original value matches
// ============================================================================

class BooleanRoundtripTest : public ::testing::Test {
protected:
    ber_encoder enc_;
    ber_decoder dec_;
};

TEST_F(BooleanRoundtripTest, True_Roundtrip_IT_DER_010) {
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_boolean(true, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_boolean(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_TRUE(dec_r.value());
}

TEST_F(BooleanRoundtripTest, False_Roundtrip_IT_DER_011) {
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_boolean(false, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_boolean(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_FALSE(dec_r.value());
}

// ============================================================================
// 3. NULL round-trip: BER encode → BER decode → success
// ============================================================================

class NullRoundtripTest : public ::testing::Test {
protected:
    ber_encoder enc_;
    ber_decoder dec_;
};

TEST_F(NullRoundtripTest, EncodeDecode_IT_DER_012) {
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_null(view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    // Verify canonical encoding: 05 00
    EXPECT_EQ(encoded, std::vector<uint8_t>({0x05, 0x00}));

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_null(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
}

// ============================================================================
// 4. OCTET STRING round-trip: short and long forms
// ============================================================================

#ifndef ASN1PP_EMBEDDED
class OctetStringRoundtripTest : public ::testing::Test {
protected:
    ber_encoder enc_;
    ber_decoder dec_;
};

TEST_F(OctetStringRoundtripTest, Short_5_Bytes_IT_DER_013) {
    std::vector<uint8_t> original = {0xDE, 0xAD, 0xBE, 0xEF, 0x00};
    std::vector<uint8_t> buf(128);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_octet_string(original, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_octet_string(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), original);
}

TEST_F(OctetStringRoundtripTest, Empty_String_IT_DER_014) {
    std::vector<uint8_t> original = {};
    std::vector<uint8_t> buf(128);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_octet_string(original, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_octet_string(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_TRUE(dec_r.value().empty());
}

TEST_F(OctetStringRoundtripTest, Long_200_Bytes_IT_DER_015) {
    std::vector<uint8_t> original(200);
    for (size_t i = 0; i < original.size(); ++i) {
        original[i] = static_cast<uint8_t>(i & 0xFF);
    }

    std::vector<uint8_t> buf(512);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_octet_string(original, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_octet_string(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), original);
}

// ============================================================================
// 5. BIT STRING round-trip: various unused_bits counts
// ============================================================================

class BitStringRoundtripTest : public ::testing::Test {
protected:
    ber_encoder enc_;
    ber_decoder dec_;
};

TEST_F(BitStringRoundtripTest, Zero_Unused_Bits_IT_DER_016) {
    std::vector<uint8_t> original = {0xAA, 0xBB, 0xCC};
    std::vector<uint8_t> buf(128);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_bit_string(original, 0, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_bit_string(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    auto& [data, unused] = dec_r.value();
    EXPECT_EQ(data, original);
    EXPECT_EQ(unused, 0);
}

TEST_F(BitStringRoundtripTest, Four_Unused_Bits_IT_DER_017) {
    // Only 4 bits significant in the last byte
    std::vector<uint8_t> original = {0xF0};  // 0b11110000, lower 4 bits are ignored
    std::vector<uint8_t> buf(128);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_bit_string(original, 4, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_bit_string(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    auto& [data, unused] = dec_r.value();
    EXPECT_EQ(unused, 4);
    // Data byte is preserved as-is
    EXPECT_EQ(data, original);
}

TEST_F(BitStringRoundtripTest, Seven_Unused_Bits_IT_DER_018) {
    std::vector<uint8_t> original = {0x80};  // only bit 7 set
    std::vector<uint8_t> buf(128);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_bit_string(original, 7, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_bit_string(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    auto& [data, unused] = dec_r.value();
    EXPECT_EQ(data, original);
    EXPECT_EQ(unused, 7);
}

TEST_F(BitStringRoundtripTest, Empty_IT_DER_019) {
    std::vector<uint8_t> original = {};
    std::vector<uint8_t> buf(128);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_bit_string(original, 0, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_bit_string(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    auto& [data, unused] = dec_r.value();
    EXPECT_TRUE(data.empty());
    EXPECT_EQ(unused, 0);
}
#endif // ASN1PP_EMBEDDED

// ============================================================================
// 6. ENUMERATED round-trip
// ============================================================================

class EnumeratedRoundtripTest : public ::testing::Test {
protected:
    ber_encoder enc_;
    ber_decoder dec_;
};

TEST_F(EnumeratedRoundtripTest, Enumerated_0_Roundtrip_IT_DER_020) {
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_enumerated(0, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_enumerated(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 0);
}

TEST_F(EnumeratedRoundtripTest, Enumerated_5_Roundtrip_IT_DER_021) {
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);
    auto r = enc_.encode_enumerated(5, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    buffer_view dec_view = make_const_view(encoded);
    auto dec_r = dec_.decode_enumerated(dec_view);
    ASSERT_TRUE(dec_r.is_ok());
    EXPECT_EQ(dec_r.value(), 5);
}

// ============================================================================
// 7. SEQUENCE round-trip
// ============================================================================

class SequenceRoundtripTest : public ::testing::Test {
protected:
    ber_encoder enc_;
    ber_decoder dec_;
};

TEST_F(SequenceRoundtripTest, Sequence_Int42_BoolTrue_IT_DER_022) {
    // SEQUENCE { INTEGER 42, BOOLEAN TRUE }
    // Step 1: encode inner elements
    uint8_t int_data[16] = {};
    uint8_t* int_ptr = int_data;
    buffer_view int_view(int_ptr, sizeof(int_data));
    // Make mutable by using a non-const pointer
    {
        std::vector<uint8_t> int_buf_vec(sizeof(int_data));
        buffer_view iv = make_mutable_view(int_buf_vec);
        auto r = enc_.encode_integer(42, iv);
        ASSERT_TRUE(r.is_ok());
        size_t consumed = consumed_bytes(int_buf_vec.size(), iv);
        std::memcpy(int_data, int_buf_vec.data(), consumed);
    }

    uint8_t bool_data[16] = {};
    {
        std::vector<uint8_t> bool_buf_vec(sizeof(bool_data));
        buffer_view bv = make_mutable_view(bool_buf_vec);
        auto r = enc_.encode_boolean(true, bv);
        ASSERT_TRUE(r.is_ok());
        size_t consumed = consumed_bytes(bool_buf_vec.size(), bv);
        std::memcpy(bool_data, bool_buf_vec.data(), consumed);
    }

    // INT 42 = 02 01 2A (3 bytes)
    // BOOL true = 01 01 FF (3 bytes)
    // Total content: 6 bytes
    // SEQUENCE header: 30 06

    std::vector<uint8_t> seq_buf(32);
    buffer_view seq_view = make_mutable_view(seq_buf);

    // Encode SEQUENCE header with content length 6
    auto hdr_r = enc_.encode_sequence_header(
        make_universal(universal_tag::sequence, true), 6, seq_view);
    ASSERT_TRUE(hdr_r.is_ok());

    // Copy inner content bytes into seq_buf after header
    size_t hdr_size = consumed_bytes(seq_buf.size(), seq_view);
    ASSERT_EQ(hdr_size, 2); // tag + length

    // Now seq_view points to remaining space: copy inner content
    uint8_t* content_dest = const_cast<uint8_t*>(seq_view.data());
    std::memcpy(content_dest, int_data, 3);       // 02 01 2A
    std::memcpy(content_dest + 3, bool_data, 3);  // 01 01 FF

    // Total encoded = 2 + 6 = 8 bytes
    std::vector<uint8_t> encoded(seq_buf.begin(), seq_buf.begin() + 8);

    // Expected: 30 06 02 01 2A 01 01 FF
    std::vector<uint8_t> expected = {
        0x30, 0x06,           // SEQUENCE { length 6
        0x02, 0x01, 0x2A,     //   INTEGER 42
        0x01, 0x01, 0xFF      //   BOOLEAN TRUE
    };
    EXPECT_EQ(encoded, expected);

    // Decode back
    buffer_view dec_view = make_const_view(encoded);
    size_t content_length = 0;
    auto tag_r = dec_.decode_sequence_header(dec_view, content_length);
    ASSERT_TRUE(tag_r.is_ok());
    EXPECT_EQ(content_length, 6);
    EXPECT_EQ(tag_r.value(), make_universal(universal_tag::sequence, true));

    // Decode inner INTEGER
    auto int_r = dec_.decode_integer(dec_view);
    ASSERT_TRUE(int_r.is_ok());
    EXPECT_EQ(int_r.value(), 42);

    // Decode inner BOOLEAN
    auto bool_r = dec_.decode_boolean(dec_view);
    ASSERT_TRUE(bool_r.is_ok());
    EXPECT_TRUE(bool_r.value());
}

TEST_F(SequenceRoundtripTest, Sequence_Nested_IT_DER_023) {
    // SEQUENCE { SEQUENCE { INTEGER 1 } }
    // Inner SEQUENCE: 30 03 02 01 01 (5 bytes content)

    uint8_t inner_int[16] = {};
    {
        std::vector<uint8_t> v(sizeof(inner_int));
        buffer_view iv = make_mutable_view(v);
        auto r = enc_.encode_integer(1, iv);
        ASSERT_TRUE(r.is_ok());
        size_t c = consumed_bytes(v.size(), iv);
        std::memcpy(inner_int, v.data(), c);
    }

    // Inner SEQUENCE header
    uint8_t inner_seq_content[16] = {};
    {
        std::vector<uint8_t> v(sizeof(inner_seq_content));
        buffer_view sv = make_mutable_view(v);
        auto hdr = enc_.encode_sequence_header(
            make_universal(universal_tag::sequence, true), 3, sv);
        ASSERT_TRUE(hdr.is_ok());
        size_t hdr_sz = consumed_bytes(v.size(), sv);
        uint8_t* dc = const_cast<uint8_t*>(sv.data());
        std::memcpy(dc, inner_int, 3);
        std::memcpy(inner_seq_content, v.data(), hdr_sz + 3);
    }

    // Outer SEQUENCE header
    std::vector<uint8_t> outer_buf(32);
    buffer_view outer_view = make_mutable_view(outer_buf);
    auto outer_hdr = enc_.encode_sequence_header(
        make_universal(universal_tag::sequence, true), 5, outer_view);
    ASSERT_TRUE(outer_hdr.is_ok());
    size_t outer_hdr_sz = consumed_bytes(outer_buf.size(), outer_view);
    uint8_t* od = const_cast<uint8_t*>(outer_view.data());
    std::memcpy(od, inner_seq_content, 5);

    std::vector<uint8_t> encoded(outer_buf.begin(), outer_buf.begin() + outer_hdr_sz + 5);

    std::vector<uint8_t> expected = {
        0x30, 0x05,           // SEQUENCE {
        0x30, 0x03,           //   SEQUENCE {
        0x02, 0x01, 0x01      //     INTEGER 1
    };
    EXPECT_EQ(encoded, expected);

    // Decode outer
    buffer_view dec_view = make_const_view(encoded);
    size_t outer_len = 0;
    auto outer_tag = dec_.decode_sequence_header(dec_view, outer_len);
    ASSERT_TRUE(outer_tag.is_ok());
    EXPECT_EQ(outer_len, 5);

    // Decode inner sequence
    size_t inner_len = 0;
    auto inner_tag = dec_.decode_sequence_header(dec_view, inner_len);
    ASSERT_TRUE(inner_tag.is_ok());
    EXPECT_EQ(inner_len, 3);

    // Decode inner integer
    auto int_r = dec_.decode_integer(dec_view);
    ASSERT_TRUE(int_r.is_ok());
    EXPECT_EQ(int_r.value(), 1);
}

// ============================================================================
// 8. DER canonical validation: rejects non-canonical encodings
// ============================================================================

class DerCanonicalValidationTest : public ::testing::Test {
protected:
    der_encoder der_enc_;
    ber_encoder  ber_enc_;
};

TEST_F(DerCanonicalValidationTest, Boolean_NonCanonical_0x42_IT_DER_024) {
    // BER encodes non-zero as TRUE, but DER requires exactly 0xFF for TRUE
    // We construct a full TLV with non-canonical content byte 0x42
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);

    tag bool_tag = make_universal(universal_tag::boolean);
    uint8_t non_canonical_byte = 0x42;
    auto r = encode_tlv(view, bool_tag,
                        std::span<const uint8_t>(&non_canonical_byte, 1));
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> non_canonical_tlv = encoded_prefix(buf, total);

    // Validate with der_encoder
    auto validate_r = der_enc_.validate(non_canonical_tlv);
    EXPECT_TRUE(validate_r.is_err());
}

TEST_F(DerCanonicalValidationTest, Boolean_NonCanonical_0x01_IT_DER_025) {
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);

    tag bool_tag = make_universal(universal_tag::boolean);
    uint8_t non_canonical_byte = 0x01;
    auto r = encode_tlv(view, bool_tag,
                        std::span<const uint8_t>(&non_canonical_byte, 1));
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> non_canonical_tlv = encoded_prefix(buf, total);

    auto validate_r = der_enc_.validate(non_canonical_tlv);
    EXPECT_TRUE(validate_r.is_err());
}

TEST_F(DerCanonicalValidationTest, Boolean_Canonical_True_Passes_IT_DER_026) {
    der_encoder enc;
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);
    auto r = enc.encode_boolean(true, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    auto validate_r = der_enc_.validate(encoded);
    EXPECT_TRUE(validate_r.is_ok());
}

TEST_F(DerCanonicalValidationTest, Boolean_Canonical_False_Passes_IT_DER_027) {
    der_encoder enc;
    std::vector<uint8_t> buf(64);
    buffer_view view = make_mutable_view(buf);
    auto r = enc.encode_boolean(false, view);
    ASSERT_TRUE(r.is_ok());

    size_t total = consumed_bytes(buf.size(), view);
    std::vector<uint8_t> encoded = encoded_prefix(buf, total);

    auto validate_r = der_enc_.validate(encoded);
    EXPECT_TRUE(validate_r.is_ok());
}

TEST_F(DerCanonicalValidationTest, Integer_NonMinimal_Rejected_IT_DER_028) {
    // Non-minimal INTEGER: should be 01 00 but is 02 00 00 (excess leading byte)
    std::vector<uint8_t> non_minimal = {0x02, 0x02, 0x00, 0x00};
    auto validate_r = der_enc_.validate(non_minimal);
    EXPECT_TRUE(validate_r.is_err());
}

// ============================================================================
// 9. DER deterministic: same value → identical byte output
// ============================================================================

class DerDeterministicTest : public ::testing::Test {
protected:
    der_encoder der_enc_;
};

TEST_F(DerDeterministicTest, Integer_Deterministic_IT_DER_029) {
    std::vector<uint8_t> buf1(64), buf2(64);
    {
        buffer_view v = make_mutable_view(buf1);
        auto r = der_enc_.encode_integer(42, v);
        ASSERT_TRUE(r.is_ok());
    }
    {
        buffer_view v = make_mutable_view(buf2);
        auto r = der_enc_.encode_integer(42, v);
        ASSERT_TRUE(r.is_ok());
    }

    size_t encoded_sz = 3;  // 02 01 2A
    EXPECT_EQ(encoded_prefix(buf1, encoded_sz), encoded_prefix(buf2, encoded_sz));
}

TEST_F(DerDeterministicTest, Boolean_Deterministic_IT_DER_030) {
    std::vector<uint8_t> buf1(64), buf2(64);
    {
        buffer_view v = make_mutable_view(buf1);
        auto r = der_enc_.encode_boolean(true, v);
        ASSERT_TRUE(r.is_ok());
    }
    {
        buffer_view v = make_mutable_view(buf2);
        auto r = der_enc_.encode_boolean(true, v);
        ASSERT_TRUE(r.is_ok());
    }

    size_t encoded_sz = 3;  // 01 01 FF
    EXPECT_EQ(encoded_prefix(buf1, encoded_sz), encoded_prefix(buf2, encoded_sz));
}

TEST_F(DerDeterministicTest, OctetString_Deterministic_IT_DER_031) {
    std::vector<uint8_t> data = {0xCA, 0xFE, 0xBA, 0xBE};

    std::vector<uint8_t> buf1(64), buf2(64);
    {
        buffer_view v = make_mutable_view(buf1);
        auto r = der_enc_.encode_octet_string(data, v);
        ASSERT_TRUE(r.is_ok());
    }
    {
        buffer_view v = make_mutable_view(buf2);
        auto r = der_enc_.encode_octet_string(data, v);
        ASSERT_TRUE(r.is_ok());
    }

    size_t encoded_sz = 6;  // 04 04 CA FE BA BE
    EXPECT_EQ(encoded_prefix(buf1, encoded_sz), encoded_prefix(buf2, encoded_sz));
}
