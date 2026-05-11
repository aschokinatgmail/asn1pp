#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>
#include <cstring>

#include "codec/traits.hpp"
#include "codec/tagged_types.hpp"
#include "codec/result.hpp"
#include "codec/ber/tlv.hpp"
#include "codec/ber/encoder.hpp"
#include "codec/ber/decoder.hpp"
#include "codec/ber/tagged_codec.hpp"
#include "buffer/buffer_view.hpp"

using namespace asn1pp;
using namespace asn1pp::ber;

namespace {

buffer_view make_view(std::vector<uint8_t>& vec) {
    return buffer_view(vec);
}

std::vector<uint8_t> consumed_bytes(const std::vector<uint8_t>& buf, size_t encoded_size) {
    return std::vector<uint8_t>(buf.begin(), buf.begin() + static_cast<long>(encoded_size));
}

TEST(TaggedCodec, ImplicitIntegerUsesApplicationOverrideTag) {
    tag override_tag = tag{tag_class::application, false, 1};

    std::vector<uint8_t> buf(128);
    buffer_view view = make_view(buf);

    uint8_t int_buf[2] = {0x2A};
    size_t val_len = 1;

    auto r = encode_tlv(view, override_tag, std::span<const uint8_t>(int_buf, val_len));
    ASSERT_TRUE(r.is_ok());

    size_t encoded_size = buf.size() - view.size();
    auto bytes = consumed_bytes(buf, encoded_size);

    EXPECT_EQ(bytes[0], 0x41);
    EXPECT_EQ(bytes[1], 0x01);
    EXPECT_EQ(bytes[2], 0x2A);
}

TEST(TaggedCodec, ImplicitBoolUsesContextSpecificTag) {
    tag override_tag = tag{tag_class::context_specific, false, 5};

    std::vector<uint8_t> buf(128);
    buffer_view view = make_view(buf);

    uint8_t byte_val = 0xFF;
    auto r = encode_tlv(view, override_tag, std::span<const uint8_t>(&byte_val, 1));
    ASSERT_TRUE(r.is_ok());

    size_t encoded_size = buf.size() - view.size();
    auto bytes = consumed_bytes(buf, encoded_size);

    EXPECT_EQ(bytes[0], 0x85);
    EXPECT_EQ(bytes[1], 0x01);
    EXPECT_EQ(bytes[2], 0xFF);
}

TEST(TaggedCodec, ImplicitTaggedPreservesValueEncoding) {
    tag override_tag = tag{tag_class::application, false, 1};

    uint8_t int_bytes[] = {0x01, 0x00};
    std::vector<uint8_t> buf(128);
    buffer_view view = make_view(buf);

    auto r = encode_tlv(view, override_tag, std::span<const uint8_t>(int_bytes, 2));
    ASSERT_TRUE(r.is_ok());

    size_t encoded_size = buf.size() - view.size();
    auto bytes = consumed_bytes(buf, encoded_size);

    EXPECT_EQ(bytes[0], 0x41);
    EXPECT_EQ(bytes[1], 0x02);
    EXPECT_EQ(bytes[2], 0x01);
    EXPECT_EQ(bytes[3], 0x00);
}

TEST(TaggedCodec, ExplicitWrapsInnerTLV) {
    tag outer_tag = tag{tag_class::context_specific, true, 3};

    uint8_t inner_tlv[] = {0x02, 0x01, 0x2A};
    size_t inner_len = sizeof(inner_tlv);

    size_t total_needed = encoded_tag_size(outer_tag) + encoded_length_size(inner_len) + inner_len;

    std::vector<uint8_t> buf(total_needed);
    uint8_t* out = buf.data();
    size_t pos = 0;

    {
        buffer_view tag_view(out + pos, total_needed - pos);
        auto r = encode_tag(tag_view, outer_tag);
        ASSERT_TRUE(r.is_ok());
        pos += encoded_tag_size(outer_tag);
    }
    {
        buffer_view len_view(out + pos, total_needed - pos);
        auto r = encode_length(len_view, inner_len);
        ASSERT_TRUE(r.is_ok());
        pos += encoded_length_size(inner_len);
    }
    std::memcpy(out + pos, inner_tlv, inner_len);
    pos += inner_len;

    EXPECT_EQ(pos, total_needed);
    EXPECT_EQ(buf[0], 0xA3);
    EXPECT_EQ(buf[1], 0x03);
    EXPECT_EQ(buf[2], 0x02);
    EXPECT_EQ(buf[3], 0x01);
    EXPECT_EQ(buf[4], 0x2A);
}

TEST(TaggedCodec, ExplicitSequenceWrapsFullSEQUENCETLV) {
    tag outer_tag = tag{tag_class::context_specific, true, 2};

    uint8_t inner_tlv[] = {0x30, 0x03, 0x02, 0x01, 0x2A};
    size_t inner_len = sizeof(inner_tlv);

    size_t total_needed = encoded_tag_size(outer_tag) + encoded_length_size(inner_len) + inner_len;

    std::vector<uint8_t> buf(total_needed);
    uint8_t* out = buf.data();
    size_t pos = 0;

    {
        buffer_view tag_view(out + pos, total_needed - pos);
        auto r = encode_tag(tag_view, outer_tag);
        ASSERT_TRUE(r.is_ok());
        pos += encoded_tag_size(outer_tag);
    }
    {
        buffer_view len_view(out + pos, total_needed - pos);
        auto r = encode_length(len_view, inner_len);
        ASSERT_TRUE(r.is_ok());
        pos += encoded_length_size(inner_len);
    }
    std::memcpy(out + pos, inner_tlv, inner_len);
    pos += inner_len;

    EXPECT_EQ(pos, total_needed);
    EXPECT_EQ(buf[0], 0xA2);
    EXPECT_EQ(buf[1], 0x05);
    EXPECT_EQ(buf[2], 0x30);
    EXPECT_EQ(buf[3], 0x03);
    EXPECT_EQ(buf[4], 0x02);
    EXPECT_EQ(buf[5], 0x01);
    EXPECT_EQ(buf[6], 0x2A);
}

TEST(TaggedCodec, PerImplicitNoEffect) {
    implicit_tagged<int64_t> val{42};
    EXPECT_EQ(val.value, 42);
}

TEST(TaggedCodec, PerExplicitNoEffect) {
    explicit_tagged<int64_t> val{99};
    EXPECT_EQ(val.value, 99);
}

TEST(TaggedCodecTraits, ImplicitInheritsPrimitiveProperty) {
    EXPECT_TRUE(is_type_primitive_v<implicit_tagged<int64_t>>);
}

TEST(TaggedCodecTraits, TagForTypePropagatesImplicit) {
    tag expected = tag_for_type_v<int64_t>;
    EXPECT_EQ(tag_for_type_v<implicit_tagged<int64_t>>, expected);
}

TEST(TaggedCodecTraits, TagForTypePropagatesExplicit) {
    tag expected = tag_for_type_v<int64_t>;
    EXPECT_EQ(tag_for_type_v<explicit_tagged<int64_t>>, expected);
}

TEST(TaggedCodecTraits, ExplicitBoolInheritsBooleanTagForType) {
    tag expected = tag_for_type_v<bool>;
    EXPECT_EQ(tag_for_type_v<explicit_tagged<bool>>, expected);
}

TEST(TaggedCodecTraits, Asn1ExplicitOuterTagDefaultIsNotSet) {
    EXPECT_FALSE(has_explicit_outer_tag_v<int64_t>);
}

TEST(TaggedCodecTraits, Asn1ExplicitOuterTagExplicitTaggedHasOuter) {
    EXPECT_TRUE(has_explicit_outer_tag_v<explicit_tagged<int64_t>>);
}

TEST(TaggedCodec, EncodeIntegerViaBEREncoder) {
    ber_encoder enc;
    std::vector<uint8_t> buf(128);
    buffer_view view = make_view(buf);

    auto r = enc.encode_integer(42, view);
    ASSERT_TRUE(r.is_ok());

    size_t encoded_size = buf.size() - view.size();
    auto bytes = consumed_bytes(buf, encoded_size);

    EXPECT_EQ(bytes[0], 0x02);
    EXPECT_EQ(bytes[1], 0x01);
    EXPECT_EQ(bytes[2], 0x2A);
}

}  // namespace
