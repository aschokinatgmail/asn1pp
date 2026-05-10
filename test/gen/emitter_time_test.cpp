#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <cstddef>
#include <optional>
#include "../../src/gen/emitter_time.hpp"
#include "../../src/gen/ast.hpp"
#include "../../src/gen/diagnostics.hpp"

namespace {

using namespace asn1pp::gen;

std::unique_ptr<type_ref> make_utc_time_type() {
    auto tr = std::make_unique<type_ref>();
    tr->content = std::string{"UTCTime"};
    return tr;
}

std::unique_ptr<type_ref> make_generalized_time_type() {
    auto tr = std::make_unique<type_ref>();
    tr->content = std::string{"GeneralizedTime"};
    return tr;
}

}

TEST(EmitterUTCTime, SimpleUTCTime) {
    auto type = make_utc_time_type();
    auto result = emitter::emit_utc_time(*type, "T1");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.empty());

    EXPECT_NE(result.code.find("struct T1 {"), std::string::npos);
    EXPECT_NE(result.code.find("std::string value"), std::string::npos);
    EXPECT_NE(result.code.find("bool operator==(const T1&) const = default"), std::string::npos);
    EXPECT_NE(result.code.find("asn1pp::asn1_tag<T1>"), std::string::npos);
    EXPECT_NE(result.code.find("make_universal(universal_tag::utc_time)"), std::string::npos);
}

TEST(EmitterUTCTime, UTCTimeWithCustomName) {
    auto type = make_utc_time_type();
    auto result = emitter::emit_utc_time(*type, "MyUTCTime");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.empty());

    EXPECT_NE(result.code.find("struct MyUTCTime {"), std::string::npos);
    EXPECT_NE(result.code.find("bool operator==(const MyUTCTime&) const = default"), std::string::npos);
    EXPECT_NE(result.code.find("asn1pp::asn1_tag<MyUTCTime>"), std::string::npos);
}

TEST(EmitterGeneralizedTime, SimpleGeneralizedTime) {
    auto type = make_generalized_time_type();
    auto result = emitter::emit_generalized_time(*type, "T2");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.empty());

    EXPECT_NE(result.code.find("struct T2 {"), std::string::npos);
    EXPECT_NE(result.code.find("std::string value"), std::string::npos);
    EXPECT_NE(result.code.find("bool operator==(const T2&) const = default"), std::string::npos);
    EXPECT_NE(result.code.find("asn1pp::asn1_tag<T2>"), std::string::npos);
    EXPECT_NE(result.code.find("make_universal(universal_tag::generalized_time)"), std::string::npos);
}

TEST(EmitterGeneralizedTime, GeneralizedTimeWithCustomName) {
    auto type = make_generalized_time_type();
    auto result = emitter::emit_generalized_time(*type, "Timestamp");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.empty());

    EXPECT_NE(result.code.find("struct Timestamp {"), std::string::npos);
    EXPECT_NE(result.code.find("bool operator==(const Timestamp&) const = default"), std::string::npos);
    EXPECT_NE(result.code.find("asn1pp::asn1_tag<Timestamp>"), std::string::npos);
}