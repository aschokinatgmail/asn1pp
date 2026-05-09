#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>

#include "codec/traits.hpp"
#include "codec/result.hpp"
#include "codec/ber/tlv.hpp"

using namespace asn1pp;
using namespace asn1pp::ber;

// ============================================================================
// Helper utilities
// ============================================================================

namespace {

// Create a mutable buffer_view from a vector for encoding tests.
// The buffer_view holds const* but encoding functions use const_cast
// to write into the mutable backing storage.
buffer_view make_mutable_view(std::vector<uint8_t>& vec) {
    return buffer_view(vec);
}

// Create a read-only buffer_view from bytes for decoding tests.
buffer_view make_view(const std::vector<uint8_t>& vec) {
    return buffer_view(vec.data(), vec.size());
}

// Check that a buffer_view's remaining content matches expected bytes.
void expect_encoded_content(buffer_view& write_view, const std::vector<uint8_t>& buf, const std::vector<uint8_t>& expected) {
    // write_view has advanced past encoded bytes; read from buf front instead
    (void)write_view;
    const size_t encoded_size = expected.size();
    for (size_t i = 0; i < encoded_size; ++i) {
        EXPECT_EQ(buf[i], expected[i]) << "Mismatch at byte " << i;
    }
    // Also verify write_view advanced by the right amount
    EXPECT_EQ(write_view.size(), buf.size() - encoded_size);
}

} // namespace

// ============================================================================
// 1. encode_tag tests
// ============================================================================

TEST(TagEncoding, UniversalPrimitiveTag2_Integral) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    auto result = encode_tag(view, make_universal(universal_tag::integer));
    ASSERT_TRUE(result.is_ok());

    std::vector<uint8_t> expected = {0x02};
    expect_encoded_content(view, buf, expected);
}

TEST(TagEncoding, UniversalConstructedSequence_0x30) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    auto result = encode_tag(view, make_universal(universal_tag::sequence, true));
    ASSERT_TRUE(result.is_ok());

    std::vector<uint8_t> expected = {0x30};
    expect_encoded_content(view, buf, expected);
}

TEST(TagEncoding, ApplicationPrimitiveTag0_0x40) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    tag t{tag_class::application, false, 0};
    auto result = encode_tag(view, t);
    ASSERT_TRUE(result.is_ok());

    std::vector<uint8_t> expected = {0x40};
    expect_encoded_content(view, buf, expected);
}

TEST(TagEncoding, ContextSpecificConstructedTag0_0xA0) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    tag t{tag_class::context_specific, true, 0};
    auto result = encode_tag(view, t);
    ASSERT_TRUE(result.is_ok());

    std::vector<uint8_t> expected = {0xA0};
    expect_encoded_content(view, buf, expected);
}

TEST(TagEncoding, PrivatePrimitiveTag1_0xC1) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    tag t{tag_class::private_class, false, 1};
    auto result = encode_tag(view, t);
    ASSERT_TRUE(result.is_ok());

    std::vector<uint8_t> expected = {0xC1};
    expect_encoded_content(view, buf, expected);
}

TEST(TagEncoding, Tag31_LongForm) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    tag t{tag_class::universal, false, 31};
    auto result = encode_tag(view, t);
    ASSERT_TRUE(result.is_ok());

    std::vector<uint8_t> expected = {0x1F, 0x1F};
    expect_encoded_content(view, buf, expected);
}

TEST(TagEncoding, Tag128_LongFormMultiByte) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    tag t{tag_class::context_specific, true, 128};
    auto result = encode_tag(view, t);
    ASSERT_TRUE(result.is_ok());

    // Tag 128 = 0b10000000 → long form: 0x81, 0x00 (128 encoded as base-128)
    // Actually: 128 = 0x80, in base-128: byte1=0x81 (bit8 set + 1), byte2=0x00
    // Wait, let me recalculate:
    // 128 / 128 = 1 rem 0. So high byte = 1 (with continuation), low byte = 0
    // But in big-endian: continuation byte 1 (0x81), last byte 0 (0x00)
    // So: 0xBF (class=10, constructed, tag=1F) and then 0x81, 0x00
    std::vector<uint8_t> expected = {0xBF, 0x81, 0x00};
    expect_encoded_content(view, buf, expected);
}

TEST(TagEncoding, Tag158_LongForm) {
    // Tag 158 = 127 + 31: 158 / 128 = 1 rem 30 → 0x81, 0x1E
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    tag t{tag_class::application, true, 158};
    auto result = encode_tag(view, t);
    ASSERT_TRUE(result.is_ok());

    // app(01) + constructed(1) + long_sentinel(1F) = 0x7F
    std::vector<uint8_t> expected = {0x7F, 0x81, 0x1E};
    expect_encoded_content(view, buf, expected);
}

TEST(TagEncoding, ContextSpecificPrimitiveTag2_0x82) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    tag t{tag_class::context_specific, false, 2};
    auto result = encode_tag(view, t);
    ASSERT_TRUE(result.is_ok());

    std::vector<uint8_t> expected = {0x82};
    expect_encoded_content(view, buf, expected);
}

TEST(TagEncoding, BufferOverflow) {
    std::vector<uint8_t> buf(0);  // zero-sized buffer
    buffer_view view = make_mutable_view(buf);

    auto result = encode_tag(view, make_universal(universal_tag::integer));
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error(), error_code::buffer_overflow);
}

TEST(TagEncoding, BufferOverflow_LongForm) {
    std::vector<uint8_t> buf(1);  // need at least 2 for tag 31
    buffer_view view = make_mutable_view(buf);

    tag t{tag_class::universal, false, 31};
    auto result = encode_tag(view, t);
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error(), error_code::buffer_overflow);
}

// ============================================================================
// 2. decode_tag tests
// ============================================================================

TEST(TagDecoding, RoundTripUniversalInteger) {
    tag original = make_universal(universal_tag::integer);

    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_tag(write_view, original).is_ok());

    buffer_view read_view = make_view(buf);  // read from original buffer position 0
    auto result = decode_tag(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), original);
}

TEST(TagDecoding, RoundTripSequenceConstructed) {
    tag original = make_universal(universal_tag::sequence, true);

    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_tag(write_view, original).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_tag(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), original);
}

TEST(TagDecoding, RoundTripApplicationPrimitiveTag0) {
    tag original{tag_class::application, false, 0};

    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_tag(write_view, original).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_tag(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), original);
}

TEST(TagDecoding, RoundTripContextSpecificConstructedTag0) {
    tag original{tag_class::context_specific, true, 0};

    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_tag(write_view, original).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_tag(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), original);
}

TEST(TagDecoding, RoundTripPrivateTag1) {
    tag original{tag_class::private_class, false, 1};

    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_tag(write_view, original).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_tag(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), original);
}

TEST(TagDecoding, RoundTripTag31LongForm) {
    tag original{tag_class::universal, false, 31};

    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_tag(write_view, original).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_tag(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), original);
}

TEST(TagDecoding, RoundTripTag128LongForm) {
    tag original{tag_class::context_specific, true, 128};

    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_tag(write_view, original).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_tag(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), original);
}

TEST(TagDecoding, RoundTripTag158LongForm) {
    tag original{tag_class::application, true, 158};

    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_tag(write_view, original).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_tag(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), original);
}

TEST(TagDecoding, RoundTripMaxTagNumber) {
    // A large but reasonable tag number
    tag original{tag_class::private_class, true, 0x3FFFFF};  // ~4 million

    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_tag(write_view, original).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_tag(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), original);
}

TEST(TagDecoding, BufferUnderflowEmpty) {
    std::vector<uint8_t> buf;
    buffer_view view = make_view(buf);

    auto result = decode_tag(view);
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error(), error_code::buffer_underflow);
}

TEST(TagDecoding, TruncatedLongForm) {
    // Long form sentinel but no continuation bytes
    std::vector<uint8_t> buf = {0x1F};
    buffer_view view = make_view(buf);

    auto result = decode_tag(view);
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error(), error_code::invalid_tag);
}

TEST(TagDecoding, AdvanceAfterDecode) {
    // Verify buffer_view advances after successful decode
    std::vector<uint8_t> buf = {0x02, 0x05, 0x00};
    buffer_view view = make_view(buf);

    auto result = decode_tag(view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(view.size(), 2);
    EXPECT_EQ(view[0], 0x05);
}

// ============================================================================
// 3. encode_length tests
// ============================================================================

TEST(LengthEncoding, Length0_ShortForm) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    auto result = encode_length(view, 0);
    ASSERT_TRUE(result.is_ok());

    std::vector<uint8_t> expected = {0x00};
    expect_encoded_content(view, buf, expected);
}

TEST(LengthEncoding, Length127_ShortFormBoundary) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    auto result = encode_length(view, 127);
    ASSERT_TRUE(result.is_ok());

    std::vector<uint8_t> expected = {0x7F};
    expect_encoded_content(view, buf, expected);
}

TEST(LengthEncoding, Length128_LongFormStart) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    auto result = encode_length(view, 128);
    ASSERT_TRUE(result.is_ok());

    std::vector<uint8_t> expected = {0x81, 0x80};
    expect_encoded_content(view, buf, expected);
}

TEST(LengthEncoding, Length255_LongForm) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    auto result = encode_length(view, 255);
    ASSERT_TRUE(result.is_ok());

    std::vector<uint8_t> expected = {0x81, 0xFF};
    expect_encoded_content(view, buf, expected);
}

TEST(LengthEncoding, Length256_LongFormTwoBytes) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    auto result = encode_length(view, 256);
    ASSERT_TRUE(result.is_ok());

    std::vector<uint8_t> expected = {0x82, 0x01, 0x00};
    expect_encoded_content(view, buf, expected);
}

TEST(LengthEncoding, Length65535_LongForm) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    auto result = encode_length(view, 65535);
    ASSERT_TRUE(result.is_ok());

    std::vector<uint8_t> expected = {0x82, 0xFF, 0xFF};
    expect_encoded_content(view, buf, expected);
}

TEST(LengthEncoding, Length65536_LongFormThreeBytes) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    auto result = encode_length(view, 65536);
    ASSERT_TRUE(result.is_ok());

    std::vector<uint8_t> expected = {0x83, 0x01, 0x00, 0x00};
    expect_encoded_content(view, buf, expected);
}

TEST(LengthEncoding, IndefiniteLength) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    auto result = encode_length(view, 0, true);  // indefinite
    ASSERT_TRUE(result.is_ok());

    std::vector<uint8_t> expected = {0x80};
    expect_encoded_content(view, buf, expected);
}

TEST(LengthEncoding, BufferOverflow_ShortForm) {
    std::vector<uint8_t> buf(0);
    buffer_view view = make_mutable_view(buf);

    auto result = encode_length(view, 10);
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error(), error_code::buffer_overflow);
}

TEST(LengthEncoding, BufferOverflow_LongForm) {
    std::vector<uint8_t> buf(1);  // need at least 3 for 256
    buffer_view view = make_mutable_view(buf);

    auto result = encode_length(view, 256);
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error(), error_code::buffer_overflow);
}

TEST(LengthEncoding, BufferOverflow_Indefinite) {
    std::vector<uint8_t> buf(0);
    buffer_view view = make_mutable_view(buf);

    auto result = encode_length(view, 0, true);
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error(), error_code::buffer_overflow);
}

// ============================================================================
// 4. decode_length tests
// ============================================================================

TEST(LengthDecoding, RoundTripLength0) {
    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_length(write_view, 0).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_length(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 0);
}

TEST(LengthDecoding, RoundTripLength127) {
    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_length(write_view, 127).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_length(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 127);
}

TEST(LengthDecoding, RoundTripLength128) {
    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_length(write_view, 128).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_length(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 128);
}

TEST(LengthDecoding, RoundTripLength255) {
    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_length(write_view, 255).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_length(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 255);
}

TEST(LengthDecoding, RoundTripLength256) {
    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_length(write_view, 256).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_length(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 256);
}

TEST(LengthDecoding, RoundTripLength65535) {
    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_length(write_view, 65535).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_length(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 65535);
}

TEST(LengthDecoding, RoundTripLength65536) {
    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_length(write_view, 65536).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_length(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 65536);
}

TEST(LengthDecoding, RoundTripLength1000000) {
    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_length(write_view, 1000000).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_length(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 1000000);
}

TEST(LengthDecoding, RoundTripIndefiniteLength) {
    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_length(write_view, 0, true).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_length(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), static_cast<size_t>(INDEFINITE_LENGTH));
}

TEST(LengthDecoding, BufferUnderflowEmpty) {
    std::vector<uint8_t> buf;
    buffer_view view = make_view(buf);

    auto result = decode_length(view);
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error(), error_code::buffer_underflow);
}

TEST(LengthDecoding, TruncatedLongForm) {
    // Long-form length header says 2 bytes follow, but buffer only has 1
    std::vector<uint8_t> buf = {0x82, 0x01};
    buffer_view view = make_view(buf);

    auto result = decode_length(view);
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error(), error_code::buffer_underflow);
}

TEST(LengthDecoding, ReservedLengthForm_FF) {
    // 0xFF is reserved per X.690 8.1.3.5.1
    std::vector<uint8_t> buf = {0xFF, 0x00};
    buffer_view view = make_view(buf);

    auto result = decode_length(view);
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error(), error_code::invalid_length);
}

TEST(LengthDecoding, ZeroLengthOctets_LongForm) {
    // Long form with 0 length octets (the 0x80 is indefinite, handled separately)
    // 0x80 with any non-zero value in short_length_mask... actually:
    // 0x80 is indefinite (handled), but 0x80 with a non-zero mask would be something else.
    // Long form with num_octets=0: first byte = 0b1xxxxxxx where xxxxxxx=0 → 0x80
    // which IS indefinite length. So there's no "long form with 0 octets" case.
    // But let's test what happens if someone sends 0x80 followed by more data,
    // it should still decode as indefinite (the first byte alone determines it).
    std::vector<uint8_t> buf = {0x80};
    buffer_view view = make_view(buf);

    auto result = decode_length(view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), static_cast<size_t>(INDEFINITE_LENGTH));
}

TEST(LengthDecoding, AdvanceAfterDecode) {
    // Verify buffer_view advances after successful decode
    std::vector<uint8_t> buf = {0x05, 0xFF, 0xFF};
    buffer_view view = make_view(buf);

    auto result = decode_length(view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 5);
    EXPECT_EQ(view.size(), 2);
    EXPECT_EQ(view[0], 0xFF);
}

// ============================================================================
// 5. End-of-content tests
// ============================================================================

TEST(EndOfContent, Encode) {
    std::vector<uint8_t> buf(16);
    buffer_view view = make_mutable_view(buf);

    auto result = encode_end_of_content(view);
    ASSERT_TRUE(result.is_ok());

    std::vector<uint8_t> expected = {0x00, 0x00};
    expect_encoded_content(view, buf, expected);
}

TEST(EndOfContent, DecodeTagReadsAsEndOfContent) {
    std::vector<uint8_t> buf = {0x00, 0x00};
    buffer_view view = make_view(buf);

    auto tag_result = decode_tag(view);
    ASSERT_TRUE(tag_result.is_ok());
    EXPECT_EQ(tag_result.value().number, 0);
    EXPECT_EQ(tag_result.value().cls, tag_class::universal);
    EXPECT_FALSE(tag_result.value().constructed);

    auto len_result = decode_length(view);
    ASSERT_TRUE(len_result.is_ok());
    EXPECT_EQ(len_result.value(), 0);
}

TEST(EndOfContent, BufferOverflow) {
    std::vector<uint8_t> buf(1);
    buffer_view view = make_mutable_view(buf);

    auto result = encode_end_of_content(view);
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error(), error_code::buffer_overflow);
}

// ============================================================================
// 6. Encoded size utilities
// ============================================================================

TEST(EncodedSize, TagShortForm) {
    EXPECT_EQ(encoded_tag_size(make_universal(universal_tag::integer)), 1);
    EXPECT_EQ(encoded_tag_size(make_universal(universal_tag::sequence, true)), 1);
    EXPECT_EQ(encoded_tag_size(tag{tag_class::application, false, 0}), 1);
    EXPECT_EQ(encoded_tag_size(tag{tag_class::context_specific, true, 30}), 1);
}

TEST(EncodedSize, TagLongForm) {
    // Tag 31: 1 initial + 1 continuation = 2
    EXPECT_EQ(encoded_tag_size(tag{tag_class::universal, false, 31}), 2);
    // Tag 128: 1 initial + 2 continuation = 3
    EXPECT_EQ(encoded_tag_size(tag{tag_class::context_specific, true, 128}), 3);
    // Tag 16383: 1 initial + 2 continuation = 3 (128*128 - 1)
    EXPECT_EQ(encoded_tag_size(tag{tag_class::application, true, 16383}), 3);
    // Tag 16384: 1 initial + 3 continuation = 4
    EXPECT_EQ(encoded_tag_size(tag{tag_class::private_class, false, 16384}), 4);
}

TEST(EncodedSize, Length) {
    EXPECT_EQ(encoded_length_size(0), 1);
    EXPECT_EQ(encoded_length_size(127), 1);
    EXPECT_EQ(encoded_length_size(128), 2);
    EXPECT_EQ(encoded_length_size(255), 2);
    EXPECT_EQ(encoded_length_size(256), 3);
    EXPECT_EQ(encoded_length_size(65535), 3);
    EXPECT_EQ(encoded_length_size(65536), 4);
    EXPECT_EQ(encoded_length_size(16777215), 4);
}

// ============================================================================
// 7. Full TLV convenience functions
// ============================================================================

TEST(TLVConvenience, EncodeDecodeRoundTrip) {
    tag t = make_universal(universal_tag::integer);
    std::vector<uint8_t> value = {0x01, 0x02, 0x03};

    std::vector<uint8_t> buf(256);
    buffer_view write_view = make_mutable_view(buf);

    auto enc_result = encode_tlv(write_view, t, value);
    ASSERT_TRUE(enc_result.is_ok());

    const size_t encoded_bytes = encoded_tag_size(t) + encoded_length_size(value.size()) + value.size();
    // Create a view over only the encoded bytes
    buffer_view read_view = buffer_view(buf.data(), encoded_bytes);
    tag decoded_tag{};
    size_t decoded_length = 0;
    auto dec_result = decode_tlv_header(read_view, decoded_tag, decoded_length);
    ASSERT_TRUE(dec_result.is_ok());

    EXPECT_EQ(decoded_tag, t);
    EXPECT_EQ(decoded_length, 3);
    ASSERT_EQ(read_view.size(), 3);
    EXPECT_EQ(read_view[0], 0x01);
    EXPECT_EQ(read_view[1], 0x02);
    EXPECT_EQ(read_view[2], 0x03);
}

TEST(TLVConvenience, EncodeDecodeEmptyValue) {
    tag t = make_universal(universal_tag::null);
    std::vector<uint8_t> value;

    std::vector<uint8_t> buf(256);
    buffer_view write_view = make_mutable_view(buf);

    auto enc_result = encode_tlv(write_view, t, value);
    ASSERT_TRUE(enc_result.is_ok());

    const size_t encoded_bytes = encoded_tag_size(t) + encoded_length_size(value.size()) + value.size();
    buffer_view read_view = buffer_view(buf.data(), encoded_bytes);
    tag decoded_tag{};
    size_t decoded_length = 0;
    auto dec_result = decode_tlv_header(read_view, decoded_tag, decoded_length);
    ASSERT_TRUE(dec_result.is_ok());

    EXPECT_EQ(decoded_tag, t);
    EXPECT_EQ(decoded_length, 0);
    EXPECT_EQ(read_view.size(), 0);
}

TEST(TLVConvenience, EncodeDecodeConstructedSequence) {
    tag t = make_universal(universal_tag::sequence, true);
    std::vector<uint8_t> value = {0x30, 0x00};

    std::vector<uint8_t> buf(256);
    buffer_view write_view = make_mutable_view(buf);

    auto enc_result = encode_tlv(write_view, t, value);
    ASSERT_TRUE(enc_result.is_ok());

    const size_t encoded_bytes = encoded_tag_size(t) + encoded_length_size(value.size()) + value.size();
    buffer_view read_view = buffer_view(buf.data(), encoded_bytes);
    tag decoded_tag{};
    size_t decoded_length = 0;
    auto dec_result = decode_tlv_header(read_view, decoded_tag, decoded_length);
    ASSERT_TRUE(dec_result.is_ok());

    EXPECT_EQ(decoded_tag, t);
    EXPECT_EQ(decoded_length, 2);
}

TEST(TLVConvenience, EncodeBufferOverflow) {
    tag t = make_universal(universal_tag::integer);
    std::vector<uint8_t> value = {0x01, 0x02};

    std::vector<uint8_t> buf(1);  // too small
    buffer_view write_view = make_mutable_view(buf);

    auto result = encode_tlv(write_view, t, value);
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error(), error_code::buffer_overflow);
}

TEST(TLVConvenience, DecodeHeaderValueTooLarge) {
    // Encode TLV with length 5, but provide only 4 bytes of value in decode buffer
    std::vector<uint8_t> buf = {0x02, 0x05, 0x01, 0x02, 0x03, 0x04};  // only 4 value bytes
    buffer_view view = make_view(buf);

    tag decoded_tag{};
    size_t decoded_length = 0;
    auto result = decode_tlv_header(view, decoded_tag, decoded_length);
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error(), error_code::buffer_underflow);
}

// ============================================================================
// 8. Edge cases & stress tests
// ============================================================================

TEST(EdgeCases, LargeTagRoundTrip) {
    // Test tag numbers across multiple boundary sizes
    for (uint32_t num : {128, 255, 16383, 16384, 2097151, 2097152}) {
        tag original{tag_class::context_specific, true, num};

        std::vector<uint8_t> buf(32);
        buffer_view write_view = make_mutable_view(buf);
        ASSERT_TRUE(encode_tag(write_view, original).is_ok()) << "Failed encoding tag " << num;

        buffer_view read_view = make_view(buf);
        auto result = decode_tag(read_view);
        ASSERT_TRUE(result.is_ok()) << "Failed decoding tag " << num;
        EXPECT_EQ(result.value(), original) << "Round-trip failed for tag " << num;
    }
}

TEST(EdgeCases, MultipleEncodesInSequence) {
    // Encode tag + length + tag + length in sequence, then decode all
    std::vector<uint8_t> buf(256);
    buffer_view view = make_mutable_view(buf);

    ASSERT_TRUE(encode_tag(view, make_universal(universal_tag::integer)).is_ok());
    ASSERT_TRUE(encode_length(view, 3).is_ok());
    ASSERT_TRUE(encode_tag(view, make_universal(universal_tag::octet_string)).is_ok());
    ASSERT_TRUE(encode_length(view, 5).is_ok());

    // Now decode
    buffer_view read_view = make_view(buf);

    auto tag1 = decode_tag(read_view);
    ASSERT_TRUE(tag1.is_ok());
    EXPECT_EQ(tag1.value(), make_universal(universal_tag::integer));

    auto len1 = decode_length(read_view);
    ASSERT_TRUE(len1.is_ok());
    EXPECT_EQ(len1.value(), 3);

    auto tag2 = decode_tag(read_view);
    ASSERT_TRUE(tag2.is_ok());
    EXPECT_EQ(tag2.value(), make_universal(universal_tag::octet_string));

    auto len2 = decode_length(read_view);
    ASSERT_TRUE(len2.is_ok());
    EXPECT_EQ(len2.value(), 5);
}

TEST(EdgeCases, ConstructedOctetStringTag) {
    // Universal octet_string, constructed = 0x24
    tag t = make_universal(universal_tag::octet_string, true);

    std::vector<uint8_t> buf(16);
    buffer_view write_view = make_mutable_view(buf);
    ASSERT_TRUE(encode_tag(write_view, t).is_ok());

    buffer_view read_view = make_view(buf);
    auto result = decode_tag(read_view);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), t);
    EXPECT_TRUE(result.value().constructed);
}

TEST(EdgeCases, AllTagClassesShortFormRoundTrip) {
    for (uint32_t num = 0; num <= 30; ++num) {
        for (int cls_val = 0; cls_val <= 3; ++cls_val) {
            auto cls = static_cast<tag_class>(cls_val);
            tag original{cls, true, num};

            std::vector<uint8_t> buf(16);
            buffer_view write_view = make_mutable_view(buf);
            ASSERT_TRUE(encode_tag(write_view, original).is_ok());

            buffer_view read_view = make_view(buf);
            auto result = decode_tag(read_view);
            ASSERT_TRUE(result.is_ok());
            EXPECT_EQ(result.value(), original);
        }
    }
}

// ============================================================================
// 9. Constant verification tests
// ============================================================================

TEST(Constants, MaskValuesMatchX690) {
    // X.690 8.1.2.2: bits 8-7 = class, bit 6 = constructed, bits 5-1 = tag number
    EXPECT_EQ(CLASS_MASK, 0xC0);
    EXPECT_EQ(CONSTRUCTED_BIT, 0x20);
    EXPECT_EQ(TAG_NUMBER_MASK, 0x1F);

    // X.690 8.1.3: length short form has bit 8 = 0
    EXPECT_EQ(LONG_LENGTH_FLAG, 0x80);
    EXPECT_EQ(SHORT_LENGTH_MASK, 0x7F);

    // X.690 8.1.5: end-of-content markers
    EXPECT_EQ(END_OF_CONTENT_TAG, 0x00);
    EXPECT_EQ(END_OF_CONTENT_LENGTH, 0x00);
}

TEST(Constants, IndefiniteLengthValue) {
    EXPECT_EQ(INDEFINITE_LENGTH, 0x80);
}
