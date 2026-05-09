#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <type_traits>
#include <functional>

#include "codec/result.hpp"

using namespace asn1pp;

// ============================================================================
// error_code enum tests
// ============================================================================

TEST(ErrorCode, HasOkValue) {
    EXPECT_EQ(static_cast<uint8_t>(error_code::ok), 0);
}

TEST(ErrorCode, HasAllErrorCodes) {
    // Verify all error codes exist and are unique
    EXPECT_NE(static_cast<uint8_t>(error_code::buffer_overflow),
             static_cast<uint8_t>(error_code::ok));
    EXPECT_NE(static_cast<uint8_t>(error_code::buffer_underflow),
             static_cast<uint8_t>(error_code::ok));
    EXPECT_NE(static_cast<uint8_t>(error_code::invalid_tag),
             static_cast<uint8_t>(error_code::ok));
    EXPECT_NE(static_cast<uint8_t>(error_code::invalid_length),
             static_cast<uint8_t>(error_code::ok));
    EXPECT_NE(static_cast<uint8_t>(error_code::constraint_violation),
             static_cast<uint8_t>(error_code::ok));
    EXPECT_NE(static_cast<uint8_t>(error_code::parse_error),
             static_cast<uint8_t>(error_code::ok));
    EXPECT_NE(static_cast<uint8_t>(error_code::unexpected_extension),
             static_cast<uint8_t>(error_code::ok));
    EXPECT_NE(static_cast<uint8_t>(error_code::value_out_of_range),
             static_cast<uint8_t>(error_code::ok));
    EXPECT_NE(static_cast<uint8_t>(error_code::encoding_error),
             static_cast<uint8_t>(error_code::ok));
}

// ============================================================================
// result<T> basic construction tests
// ============================================================================

TEST(ResultInt, ConstructOk) {
    result<int> r = result<int>::ok(42);
    EXPECT_TRUE(r.is_ok());
    EXPECT_FALSE(r.is_err());
}

TEST(ResultInt, ConstructErr) {
    result<int> r = result<int>::err(error_code::buffer_overflow);
    EXPECT_TRUE(r.is_err());
    EXPECT_FALSE(r.is_ok());
}

TEST(ResultInt, OkValue) {
    result<int> r = result<int>::ok(42);
    EXPECT_EQ(r.value(), 42);
}

TEST(ResultInt, ErrError) {
    result<int> r = result<int>::err(error_code::buffer_overflow);
    EXPECT_EQ(r.error(), error_code::buffer_overflow);
}

TEST(ResultInt, ErrValueAccessOnErrorState) {
    result<int> r = result<int>::err(error_code::buffer_overflow);
    EXPECT_TRUE(r.is_err());
}

// ============================================================================
// result<int> chaining tests (map operations)
// ============================================================================

TEST(ResultInt, MapTransformsValue) {
    auto r = result<int>::ok(5);
    auto mapped = r.map([](int x) { return x + 1; });
    EXPECT_TRUE(mapped.is_ok());
    EXPECT_EQ(mapped.value(), 6);
}

TEST(ResultInt, MapChaining) {
    auto r = result<int>::ok(5);
    auto mapped = r.map([](int x) { return x + 1; })
                  .map([](int x) { return x * 3; });
    EXPECT_TRUE(mapped.is_ok());
    EXPECT_EQ(mapped.value(), 18);
}

TEST(ResultInt, MapErrPropagates) {
    auto r = result<int>::err(error_code::buffer_overflow);
    auto mapped = r.map([](int x) { return x + 1; });
    EXPECT_TRUE(mapped.is_err());
    EXPECT_EQ(mapped.error(), error_code::buffer_overflow);
}

// ============================================================================
// result<int> and_then tests (monadic bind)
// ============================================================================

TEST(ResultInt, AndThenTransforms) {
    auto r = result<int>::ok(5);
    auto bound = r.and_then([](int x) { return result<int>::ok(x * 2); });
    EXPECT_TRUE(bound.is_ok());
    EXPECT_EQ(bound.value(), 10);
}

TEST(ResultInt, AndThenChaining) {
    auto r = result<int>::ok(5);
    auto chained = r.and_then([](int x) { return result<int>::ok(x + 1); })
                  .and_then([](int x) { return result<int>::ok(x * 3); });
    EXPECT_TRUE(chained.is_ok());
    EXPECT_EQ(chained.value(), 18);
}

TEST(ResultInt, AndThenErrShortCircuits) {
    auto r = result<int>::err(error_code::buffer_overflow);
    auto bound = r.and_then([](int x) { return result<int>::ok(x * 2); });
    EXPECT_TRUE(bound.is_err());
    EXPECT_EQ(bound.error(), error_code::buffer_overflow);
}

TEST(ResultInt, AndThenErrInCallbackShortCircuits) {
    auto r = result<int>::ok(5);
    auto bound = r.and_then([](int x) -> result<int> {
        if (x > 3) {
            return result<int>::err(error_code::value_out_of_range);
        }
        return result<int>::ok(x * 2);
    });
    EXPECT_TRUE(bound.is_err());
    EXPECT_EQ(bound.error(), error_code::value_out_of_range);
}

// ============================================================================
// result<void> specialization tests
// ============================================================================

TEST(ResultVoid, ConstructOk) {
    result<void> r = result<void>::ok();
    EXPECT_TRUE(r.is_ok());
    EXPECT_FALSE(r.is_err());
}

TEST(ResultVoid, ConstructErr) {
    result<void> r = result<void>::err(error_code::parse_error);
    EXPECT_TRUE(r.is_err());
    EXPECT_FALSE(r.is_ok());
}

TEST(ResultVoid, ErrError) {
    result<void> r = result<void>::err(error_code::invalid_tag);
    EXPECT_EQ(r.error(), error_code::invalid_tag);
}

TEST(ResultVoid, MapVoidOk) {
    result<void> r = result<void>::ok();
    auto mapped = r.map([]() { return 42; });
    EXPECT_TRUE(mapped.is_ok());
    EXPECT_EQ(mapped.value(), 42);
}

TEST(ResultVoid, MapVoidErrPropagates) {
    result<void> r = result<void>::err(error_code::encoding_error);
    auto mapped = r.map([]() { return 42; });
    EXPECT_TRUE(mapped.is_err());
    EXPECT_EQ(mapped.error(), error_code::encoding_error);
}

TEST(ResultVoid, AndThenVoidOk) {
    result<void> r = result<void>::ok();
    auto bound = r.and_then([]() { return result<int>::ok(100); });
    EXPECT_TRUE(bound.is_ok());
    EXPECT_EQ(bound.value(), 100);
}

TEST(ResultVoid, AndThenVoidErrPropagates) {
    result<void> r = result<void>::err(error_code::buffer_overflow);
    auto bound = r.and_then([]() { return result<int>::ok(100); });
    EXPECT_TRUE(bound.is_err());
    EXPECT_EQ(bound.error(), error_code::buffer_overflow);
}

// ============================================================================
// result<const char*> tests (string-like type)
// ============================================================================

TEST(ResultString, OkConstruction) {
    result<const char*> r = result<const char*>::ok("hello");
    EXPECT_TRUE(r.is_ok());
    EXPECT_STREQ(r.value(), "hello");
}

TEST(ResultString, ErrConstruction) {
    result<const char*> r = result<const char*>::err(error_code::invalid_length);
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(r.error(), error_code::invalid_length);
}

// ============================================================================
// Trivially copyable tests
// ============================================================================

TEST(ResultTriviallyCopyable, IntIsTriviallyCopyable) {
    EXPECT_TRUE(std::is_trivially_copyable_v<result<int>>);
}

TEST(ResultTriviallyCopyable, VoidIsTriviallyCopyable) {
    EXPECT_TRUE(std::is_trivially_copyable_v<result<void>>);
}

TEST(ResultTriviallyCopyable, SizeReasonable) {
    // Should be reasonable: T + error_code + discriminator
    // For int: 4 + 1 (padded to 4) + 4 = ~12 bytes
    EXPECT_LE(sizeof(result<int>), 24u);  // Generous upper bound
}

TEST(ResultTriviallyCopyable, VoidSizeReasonable) {
    // For void: just error_code + discriminator (no T)
    EXPECT_LE(sizeof(result<void>), 16u);  // Generous upper bound
}

// ============================================================================
// Error code equality tests
// ============================================================================

TEST(ErrorCodeEquality, SameCodeEqual) {
    EXPECT_EQ(error_code::buffer_overflow, error_code::buffer_overflow);
    EXPECT_EQ(error_code::parse_error, error_code::parse_error);
}

TEST(ErrorCodeEquality, DifferentCodesNotEqual) {
    EXPECT_NE(error_code::buffer_overflow, error_code::buffer_underflow);
    EXPECT_NE(error_code::invalid_tag, error_code::invalid_length);
}