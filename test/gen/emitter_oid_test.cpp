#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../src/gen/emitter_oid.hpp"
#include "../../src/gen/emitter.hpp"

namespace asn1pp::gen::test {

class EmitterOidTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(EmitterOidTest, ObjectIdentifierHasArcsMember_UT_OID_001) {
    object_identifier_type type;
    std::string output = emit_object_identifier_type(type, "X", {});
    EXPECT_THAT(output, ::testing::HasSubstr("std::vector<uint32_t> arcs"));
}

TEST_F(EmitterOidTest, ObjectIdentifierHasOperatorEquals_UT_OID_002) {
    object_identifier_type type;
    std::string output = emit_object_identifier_type(type, "X", {});
    EXPECT_THAT(output, ::testing::HasSubstr("bool operator==(const X&) const = default"));
}

TEST_F(EmitterOidTest, ObjectIdentifierHasTagSpecialization_UT_OID_003) {
    object_identifier_type type;
    std::string output = emit_object_identifier_type(type, "X", {});
    EXPECT_THAT(output, ::testing::HasSubstr("asn1_tag<X>"));
    EXPECT_THAT(output, ::testing::HasSubstr("universal_tag::object_identifier"));
}

TEST_F(EmitterOidTest, ObjectIdentifierHasFromStringHelper_UT_OID_004) {
    object_identifier_type type;
    std::string output = emit_object_identifier_type(type, "X", {});
    EXPECT_THAT(output, ::testing::HasSubstr("static X from_string(const char* dotted)"));
    EXPECT_THAT(output, ::testing::HasSubstr("std::getline(ss, token, '.')"));
}

TEST_F(EmitterOidTest, RelativeOidHasArcsMember_UT_OID_005) {
    relative_oid_type type;
    std::string output = emit_relative_oid_type(type, "Y", {});
    EXPECT_THAT(output, ::testing::HasSubstr("std::vector<uint32_t> arcs"));
}

TEST_F(EmitterOidTest, RelativeOidHasOperatorEquals_UT_OID_006) {
    relative_oid_type type;
    std::string output = emit_relative_oid_type(type, "Y", {});
    EXPECT_THAT(output, ::testing::HasSubstr("bool operator==(const Y&) const = default"));
}

TEST_F(EmitterOidTest, RelativeOidHasTagSpecialization_UT_OID_007) {
    relative_oid_type type;
    std::string output = emit_relative_oid_type(type, "Y", {});
    EXPECT_THAT(output, ::testing::HasSubstr("asn1_tag<Y>"));
    EXPECT_THAT(output, ::testing::HasSubstr("universal_tag::relative_oid"));
}

TEST_F(EmitterOidTest, RelativeOidHasFromStringHelper_UT_OID_008) {
    relative_oid_type type;
    std::string output = emit_relative_oid_type(type, "Y", {});
    EXPECT_THAT(output, ::testing::HasSubstr("static Y from_string(const char* dotted)"));
    EXPECT_THAT(output, ::testing::HasSubstr("std::getline(ss, token, '.')"));
}

TEST_F(EmitterOidTest, ObjectIdentifierFromStringParsesLargeOid_UT_OID_009) {
    object_identifier_type type;
    std::string output = emit_object_identifier_type(type, "X", {});
    EXPECT_THAT(output, ::testing::HasSubstr("std::stoul(token)"));
}

TEST_F(EmitterOidTest, ObjectIdentifierNameIsUsed_UT_OID_010) {
    object_identifier_type type;
    std::string output = emit_object_identifier_type(type, "MyOid", {});
    EXPECT_THAT(output, ::testing::HasSubstr("struct MyOid"));
    EXPECT_THAT(output, ::testing::HasSubstr("asn1_tag<MyOid>"));
    EXPECT_THAT(output, ::testing::HasSubstr("static MyOid from_string"));
}

TEST_F(EmitterOidTest, RelativeOidNameIsUsed_UT_OID_011) {
    relative_oid_type type;
    std::string output = emit_relative_oid_type(type, "MyRelOid", {});
    EXPECT_THAT(output, ::testing::HasSubstr("struct MyRelOid"));
    EXPECT_THAT(output, ::testing::HasSubstr("asn1_tag<MyRelOid>"));
    EXPECT_THAT(output, ::testing::HasSubstr("static MyRelOid from_string"));
}

}  // namespace asn1pp::gen::test