#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <type_traits>

#include "codec/codec_interface.hpp"

using namespace asn1pp;

// ============================================================================
// Mock type: mock_int_type with asn1_tag specialization
// ============================================================================

struct mock_int_type {
    int64_t value;
};

template<>
struct asn1_tag<mock_int_type> {
    static constexpr auto value = universal_tag::integer;
    static constexpr bool is_specialized = true;
};

// ============================================================================
// Mock encoder: satisfies encoder_for with result<void>
// ============================================================================

struct mock_encoder {
    result<void> encode(const mock_int_type& /*val*/, buffer_view& /*buf*/) {
        return result<void>::ok();
    }
};

// ============================================================================
// Mock decoder: template decode returning result<T>
//               This does NOT match decoder_for with result<T> return
//               because decoder_for expects non-template decode returning result<T>
// ============================================================================

struct mock_decoder {
    template<typename T>
    result<T> decode(buffer_view& /*buf*/) {
        return result<T>::err(error_code::parse_error);
    }
};

// ============================================================================
// Specific-type decoder: matches decoder_for<mock_specific_decoder, mock_int_type>
// ============================================================================

struct mock_specific_decoder {
    result<mock_int_type> decode(buffer_view& /*buf*/) {
        return result<mock_int_type>::err(error_code::parse_error);
    }
};

// ============================================================================
// Combined mock codec: satisfies both encoder_for and decoder_for
// ============================================================================

struct mock_codec {
    int state{0};

    result<void> encode(const mock_int_type& /*val*/, buffer_view& /*buf*/) {
        return result<void>::ok();
    }

    result<mock_int_type> decode(buffer_view& /*buf*/) {
        return result<mock_int_type>::err(error_code::parse_error);
    }
};

// ============================================================================
// Stateless codec: empty class
// ============================================================================

struct mock_stateless_codec {
    result<void> encode(const mock_int_type& /*val*/, buffer_view& /*buf*/) {
        return result<void>::ok();
    }

    result<mock_int_type> decode(buffer_view& /*buf*/) {
        return result<mock_int_type>::err(error_code::parse_error);
    }
};
static_assert(std::is_empty_v<mock_stateless_codec>);

// ============================================================================
// Non-encoder type (for negative concept tests)
// ============================================================================

struct non_codec_type {
    int unrelated_field;
};

// ============================================================================
// Concept tests: encoder_for
// ============================================================================

// Positive: mock_encoder satisfies encoder_for<mock_encoder, mock_int_type>
static_assert(encoder_for<mock_encoder, mock_int_type>,
    "mock_encoder must satisfy encoder_for<mock_encoder, mock_int_type>");

// Negative: int is not an encoder
static_assert(!encoder_for<int, mock_int_type>,
    "int must NOT satisfy encoder_for");

// Negative: non_codec_type is not an encoder
static_assert(!encoder_for<non_codec_type, mock_int_type>,
    "non_codec_type must NOT satisfy encoder_for");

// ============================================================================
// Concept tests: decoder_for
// ============================================================================

// Positive: mock_specific_decoder satisfies decoder_for
static_assert(decoder_for<mock_specific_decoder, mock_int_type>,
    "mock_specific_decoder must satisfy decoder_for<mock_specific_decoder, mock_int_type>");

// Negative: mock_decoder (template decode) does NOT match — it requires
// explicit T in the decode signature, not a template
static_assert(!decoder_for<mock_decoder, mock_int_type>,
    "mock_decoder (template decode) must NOT satisfy decoder_for");

// Negative: int is not a decoder
static_assert(!decoder_for<int, mock_int_type>,
    "int must NOT satisfy decoder_for");

// ============================================================================
// Concept tests: codec_for
// ============================================================================

// Positive: mock_codec satisfies both encoder_for and decoder_for
static_assert(codec_for<mock_codec, mock_int_type>,
    "mock_codec must satisfy codec_for<mock_codec, mock_int_type>");

// Negative: mock_encoder (only encoder, not decoder) is NOT a full codec
static_assert(!codec_for<mock_encoder, mock_int_type>,
    "mock_encoder (encoder only) must NOT satisfy codec_for");

// ============================================================================
// Concept tests: stateless_codec
// ============================================================================

// Positive: empty class satisfies stateless_codec
static_assert(stateless_codec<mock_stateless_codec>,
    "mock_stateless_codec (empty class) must satisfy stateless_codec");

// Negative: non-empty class does not satisfy stateless_codec
static_assert(!stateless_codec<mock_codec>,
    "mock_codec may not be empty and thus may or may not satisfy stateless_codec");

// ============================================================================
// CRTP base class tests: encoder_base
// ============================================================================

// Minimal derived encoder using CRTP
struct derived_encoder : encoder_base<derived_encoder> {
    result<void> encode(const mock_int_type& /*val*/, buffer_view& /*buf*/) {
        return result<void>::ok();
    }
};

// Verify it compiles and satisfies the concept
static_assert(encoder_for<derived_encoder, mock_int_type>,
    "derived_encoder (CRTP) must satisfy encoder_for");

// Verify encode_fields stub compiles
TEST(CodecInterfaceCrtpTest, derived_encoder_compiles) {
    derived_encoder enc;
    mock_int_type val{42};
    std::vector<uint8_t> data(256);
    buffer_view buf(data);

    auto r = enc.encode(val, buf);
    EXPECT_TRUE(r.is_ok());
}

// ============================================================================
// CRTP base class tests: decoder_base
// ============================================================================

// Minimal derived decoder using CRTP
struct derived_decoder : decoder_base<derived_decoder> {
    result<mock_int_type> decode(buffer_view& /*buf*/) {
        return result<mock_int_type>::err(error_code::parse_error);
    }
};

// Verify it compiles and satisfies the concept
static_assert(decoder_for<derived_decoder, mock_int_type>,
    "derived_decoder (CRTP) must satisfy decoder_for");

TEST(CodecInterfaceCrtpTest, derived_decoder_compiles) {
    derived_decoder dec;
    std::vector<uint8_t> data(256);
    buffer_view buf(data);

    auto r = dec.decode(buf);
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::parse_error);
}

// ============================================================================
// Combined CRTP codec
// ============================================================================

struct derived_codec : encoder_base<derived_codec>, decoder_base<derived_codec> {
    result<void> encode(const mock_int_type& /*val*/, buffer_view& /*buf*/) {
        return result<void>::ok();
    }

    result<mock_int_type> decode(buffer_view& /*buf*/) {
        return result<mock_int_type>::err(error_code::parse_error);
    }
};

static_assert(codec_for<derived_codec, mock_int_type>,
    "derived_codec (CRTP) must satisfy codec_for");

TEST(CodecInterfaceCrtpTest, derived_codec_encode_decode) {
    derived_codec codec;
    std::vector<uint8_t> data(256);
    buffer_view buf(data);

    auto enc_res = codec.encode(mock_int_type{42}, buf);
    EXPECT_TRUE(enc_res.is_ok());

    auto dec_res = codec.decode(buf);
    EXPECT_TRUE(dec_res.is_err());
}

// ============================================================================
// Runtime concept validation (not static_assert — via GTest)
// ============================================================================

TEST(CodecInterfaceConceptTest, encoder_for_mock_encoder_is_true) {
    constexpr bool result = encoder_for<mock_encoder, mock_int_type>;
    EXPECT_TRUE(result);
}

TEST(CodecInterfaceConceptTest, encoder_for_int_is_false) {
    constexpr bool result = encoder_for<int, mock_int_type>;
    EXPECT_FALSE(result);
}

TEST(CodecInterfaceConceptTest, decoder_for_mock_specific_decoder_is_true) {
    constexpr bool result = decoder_for<mock_specific_decoder, mock_int_type>;
    EXPECT_TRUE(result);
}

TEST(CodecInterfaceConceptTest, decoder_for_template_decoder_is_false) {
    // Template decode doesn't match — requires explicit type in signature
    constexpr bool result = decoder_for<mock_decoder, mock_int_type>;
    EXPECT_FALSE(result);
}

TEST(CodecInterfaceConceptTest, codec_for_mock_codec_is_true) {
    constexpr bool result = codec_for<mock_codec, mock_int_type>;
    EXPECT_TRUE(result);
}

TEST(CodecInterfaceConceptTest, codec_for_encoder_only_is_false) {
    constexpr bool result = codec_for<mock_encoder, mock_int_type>;
    EXPECT_FALSE(result);
}

TEST(CodecInterfaceConceptTest, stateless_codec_for_empty_type_is_true) {
    constexpr bool result = stateless_codec<mock_stateless_codec>;
    EXPECT_TRUE(result);
}
