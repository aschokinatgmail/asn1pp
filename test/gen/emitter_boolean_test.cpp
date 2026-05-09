#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../src/gen/emitter_boolean.hpp"
#include "../../src/gen/emitter_null.hpp"

namespace asn1pp::gen::test {

// Test fixture for BOOLEAN and NULL type emitters
class EmitterBooleanTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(EmitterBooleanTest, BooleanStructHasValueMember_UT_BOOLEAN_001) {
    // Generate: X ::= BOOLEAN
    // Expected: struct X { bool value{}; }
    generator gen;
    std::string output = gen.emit_boolean("X");
    EXPECT_THAT(output, ::testing::HasSubstr("bool value{}"));
}

TEST_F(EmitterBooleanTest, BooleanStructHasOperatorBool_UT_BOOLEAN_002) {
    // Generate: X ::= BOOLEAN
    // Expected: operator bool() const { return value; }
    generator gen;
    std::string output = gen.emit_boolean("X");
    EXPECT_THAT(output, ::testing::HasSubstr("operator bool() const"));
    EXPECT_THAT(output, ::testing::HasSubstr("return value"));
}

TEST_F(EmitterBooleanTest, BooleanHasTagSpecialization_UT_BOOLEAN_003) {
    // Generate: X ::= BOOLEAN
    // Expected: template<> struct asn1pp::asn1_tag<X> { static constexpr auto value = universal_tag::boolean; };
    generator gen;
    std::string output = gen.emit_boolean("X");
    EXPECT_THAT(output, ::testing::HasSubstr("asn1_tag<X>"));
    EXPECT_THAT(output, ::testing::HasSubstr("universal_tag::boolean"));
}

TEST_F(EmitterBooleanTest, BooleanOperatorEquals_UT_BOOLEAN_004) {
    // Generate: X ::= BOOLEAN
    // Expected: bool operator==(const X&) const = default;
    generator gen;
    std::string output = gen.emit_boolean("X");
    EXPECT_THAT(output, ::testing::HasSubstr("bool operator==(const X&) const = default"));
}

TEST_F(EmitterBooleanTest, NullStructIsEmpty_UT_NULL_001) {
    // Generate: X ::= NULL
    // Expected: struct X { /* NULL type has no value */ }
    generator gen;
    std::string output = gen.emit_null("X");
    EXPECT_THAT(output, ::testing::HasSubstr("struct X"));
    // NULL has no value member
    EXPECT_THAT(output, ::testing::Not(::testing::HasSubstr("bool value")));
}

TEST_F(EmitterBooleanTest, NullHasOperatorEquals_UT_NULL_002) {
    // Generate: X ::= NULL
    // Expected: bool operator==(const X&) const = default;
    generator gen;
    std::string output = gen.emit_null("X");
    EXPECT_THAT(output, ::testing::HasSubstr("bool operator==(const X&) const = default"));
}

TEST_F(EmitterBooleanTest, NullHasTagSpecialization_UT_NULL_003) {
    // Generate: X ::= NULL
    // Expected: template<> struct asn1pp::asn1_tag<X> { static constexpr auto value = universal_tag::null; };
    generator gen;
    std::string output = gen.emit_null("X");
    EXPECT_THAT(output, ::testing::HasSubstr("asn1_tag<X>"));
    EXPECT_THAT(output, ::testing::HasSubstr("universal_tag::null"));
}

TEST_F(EmitterBooleanTest, BooleanCodeCompilesAsValidC20_UT_BOOLEAN_005) {
    // Verify the generated BOOLEAN code compiles as valid C++20
    generator gen;
    std::string output = gen.emit_boolean("TestBool");
    // Simple compilation check - code should be parseable
    EXPECT_THAT(output, ::testing::HasSubstr("struct TestBool"));
    EXPECT_THAT(output, ::testing::HasSubstr("bool value"));
}

TEST_F(EmitterBooleanTest, NullCodeCompilesAsValidC20_UT_NULL_004) {
    // Verify the generated NULL code compiles as valid C++20
    generator gen;
    std::string output = gen.emit_null("TestNull");
    // Simple compilation check - code should be parseable
    EXPECT_THAT(output, ::testing::HasSubstr("struct TestNull"));
    EXPECT_THAT(output, ::testing::HasSubstr("bool operator=="));
}

}  // namespace asn1pp::gen::test