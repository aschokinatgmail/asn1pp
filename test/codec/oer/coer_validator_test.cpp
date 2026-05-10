#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>
#include <cstring>

#include "codec/oer/coer_validator.hpp"
#include "codec/oer/encoder.hpp"
#include "codec/oer/decoder.hpp"

using namespace asn1pp;
using namespace asn1pp::oer;

struct unconstrained_int_meta {
    static constexpr bool has_range_constraint = false;
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = INT64_MAX;
};

struct unconstrained_octets_meta {
    static constexpr bool has_size_constraint = false;
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = SIZE_MAX;
};

inline std::vector<uint8_t> encode_oer_unconstrained_int(int64_t value) {
    oer_encoder enc;
    std::vector<uint8_t> buf(16);
    buffer_view view(buf);
    auto r = enc.encode_integer<unconstrained_int_meta>(value, view);
    EXPECT_TRUE(r.is_ok());
    return std::vector<uint8_t>(buf.begin(), buf.begin() + view.size());
}

inline std::vector<uint8_t> encode_oer_octet_string(const std::vector<uint8_t>& data) {
    oer_encoder enc;
    std::vector<uint8_t> buf(64);
    buffer_view view(buf);
    auto r = enc.encode_octet_string<unconstrained_octets_meta>(data, view);
    EXPECT_TRUE(r.is_ok());
    return std::vector<uint8_t>(buf.begin(), buf.begin() + view.size());
}

TEST(CoerValidatorInteger, AcceptsCanonicalPositiveInteger) {
    auto encoded = encode_oer_unconstrained_int(127);
    auto res = coer_validator::validate_integer<unconstrained_int_meta>(
        buffer_view(encoded.data(), encoded.size()));
    EXPECT_TRUE(res.is_ok());
}

TEST(CoerValidatorInteger, AcceptsCanonicalNegativeInteger) {
    auto encoded = encode_oer_unconstrained_int(-1);
    auto res = coer_validator::validate_integer<unconstrained_int_meta>(
        buffer_view(encoded.data(), encoded.size()));
    EXPECT_TRUE(res.is_ok());
}

TEST(CoerValidatorInteger, RejectsNonMinimalPositiveInteger) {
    std::vector<uint8_t> non_canonical = {0x02, 0x00, 0x7F};
    auto res = coer_validator::validate_integer<unconstrained_int_meta>(
        buffer_view(non_canonical.data(), non_canonical.size()));
    EXPECT_TRUE(res.is_err());
}

TEST(CoerValidatorInteger, RejectsNonMinimalNegativeInteger) {
    std::vector<uint8_t> non_canonical = {0x02, 0xFF, 0x80};
    auto res = coer_validator::validate_integer<unconstrained_int_meta>(
        buffer_view(non_canonical.data(), non_canonical.size()));
    EXPECT_TRUE(res.is_err());
}

TEST(CoerValidatorInteger, AcceptsMultiByteCanonicalPositive) {
    auto encoded = encode_oer_unconstrained_int(128);
    auto res = coer_validator::validate_integer<unconstrained_int_meta>(
        buffer_view(encoded.data(), encoded.size()));
    EXPECT_TRUE(res.is_ok());
}

TEST(CoerValidatorInteger, AcceptsMultiByteCanonicalNegative) {
    auto encoded = encode_oer_unconstrained_int(-129);
    auto res = coer_validator::validate_integer<unconstrained_int_meta>(
        buffer_view(encoded.data(), encoded.size()));
    EXPECT_TRUE(res.is_ok());
}

TEST(CoerValidatorLengthDeterminant, AcceptsShortForm) {
    std::vector<uint8_t> encoded = {0x32};
    auto res = coer_validator::validate_length_determinant(
        buffer_view(encoded.data(), encoded.size()));
    EXPECT_TRUE(res.is_ok());
}

TEST(CoerValidatorLengthDeterminant, AcceptsLongForm) {
    std::vector<uint8_t> encoded = {0x81, 0xC8};
    auto res = coer_validator::validate_length_determinant(
        buffer_view(encoded.data(), encoded.size()));
    EXPECT_TRUE(res.is_ok());
}

TEST(CoerValidatorLengthDeterminant, RejectsLeadingZeroInLongForm) {
    std::vector<uint8_t> non_canonical = {0x82, 0x00, 0x80};
    auto res = coer_validator::validate_length_determinant(
        buffer_view(non_canonical.data(), non_canonical.size()));
    EXPECT_TRUE(res.is_err());
}

TEST(CoerValidatorLengthDeterminant, RejectsUnnecessaryLongForm) {
    std::vector<uint8_t> non_canonical = {0x81, 0x64};
    auto res = coer_validator::validate_length_determinant(
        buffer_view(non_canonical.data(), non_canonical.size()));
    EXPECT_TRUE(res.is_err());
}

TEST(CoerValidatorOctetString, AcceptsCanonicalOctetString) {
    auto encoded = encode_oer_octet_string({'h', 'e', 'l', 'l', 'o'});
    auto vres = coer_validator::validate_octet_string<unconstrained_octets_meta>(
        buffer_view(encoded.data(), encoded.size()));
    EXPECT_TRUE(vres.is_ok());
}

TEST(CoerValidatorOctetString, RejectsNonMinimalLengthPrefix) {
    std::vector<uint8_t> non_canonical = {0x82, 0x00, 0x02, 'h', 'i'};
    auto res = coer_validator::validate_octet_string<unconstrained_octets_meta>(
        buffer_view(non_canonical.data(), non_canonical.size()));
    EXPECT_TRUE(res.is_err());
}

TEST(CoerValidatorFullValidation, AcceptsValidCanonicalOer) {
    auto encoded = encode_oer_unconstrained_int(42);
    auto res = coer_validator::validate<unconstrained_int_meta>(
        buffer_view(encoded.data(), encoded.size()));
    EXPECT_TRUE(res.is_ok());
}

TEST(CoerValidatorFullValidation, RejectsNonCanonicalOer) {
    std::vector<uint8_t> bad = {0x02, 0x00, 0x01};
    auto res = coer_validator::validate<unconstrained_int_meta>(
        buffer_view(bad.data(), bad.size()));
    EXPECT_TRUE(res.is_err());
}

TEST(CoerValidatorInteger, CanonicalRoundTrip) {
    int64_t test_values[] = {0, 1, 127, 128, 255, 256, 32767, 32768,
                             -1, -128, -129, -32768, -32769};

    for (int64_t val : test_values) {
        SCOPED_TRACE("Testing value: " + std::to_string(val));

        auto encoded = encode_oer_unconstrained_int(val);

        oer_decoder dec;
        buffer_view dv(encoded);
        auto decoded = dec.decode_integer<unconstrained_int_meta>(dv);
        EXPECT_TRUE(decoded.is_ok());
        EXPECT_EQ(decoded.value(), val);

        auto vres = coer_validator::validate_integer<unconstrained_int_meta>(
            buffer_view(encoded.data(), encoded.size()));
        EXPECT_TRUE(vres.is_ok());
    }
}

TEST(CoerValidatorLengthDeterminant, RejectsZeroPrefixedLongForm) {
    std::vector<uint8_t> non_canonical = {0x83, 0x00, 0x01, 0x00};
    auto res = coer_validator::validate_length_determinant(
        buffer_view(non_canonical.data(), non_canonical.size()));
    EXPECT_TRUE(res.is_err());
}
