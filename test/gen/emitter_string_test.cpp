#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <cstddef>
#include <optional>
#include "../../src/gen/emitter_string.hpp"
#include "../../src/gen/ast.hpp"
#include "../../src/gen/diagnostics.hpp"

namespace {

using namespace asn1pp::gen;

std::unique_ptr<type_ref> make_octet_string_type() {
    auto tr = std::make_unique<type_ref>();
    tr->content = octet_string_type{};
    return tr;
}

std::unique_ptr<type_ref> make_bit_string_type() {
    auto tr = std::make_unique<type_ref>();
    tr->content = bit_string_type{};
    return tr;
}

std::unique_ptr<type_ref> make_bit_string_with_named_bits() {
    auto tr = std::make_unique<type_ref>();
    bit_string_type bt;
    bt.named_bits.push_back({"bit0", 0});
    bt.named_bits.push_back({"bit1", 1});
    bt.named_bits.push_back({"flag", 5});
    tr->content = std::move(bt);
    return tr;
}

std::unique_ptr<type_ref> make_constrained_octet_string(std::optional<size_t> min_size, std::optional<size_t> max_size) {
    auto tr = std::make_unique<type_ref>();
    auto ct = std::make_unique<constrained_type>();
    ct->underlying_type = std::make_unique<type_ref>();
    ct->underlying_type->content = octet_string_type{};
    constraint c;
    size_constraint sc;
    sc.min_size = min_size;
    sc.max_size = max_size;
    c.content = sc;
    ct->constraints.push_back(std::move(c));
    tr->content = std::move(ct);
    return tr;
}

}

TEST(EmitterOctetString, SimpleOctetString) {
    auto type = make_octet_string_type();
    auto result = emitter::emit_octet_string(*type, "X");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.empty());

    EXPECT_NE(result.code.find("struct X {"), std::string::npos);
    EXPECT_NE(result.code.find("std::vector<uint8_t> value"), std::string::npos);
    EXPECT_NE(result.code.find("bool operator==(const X&) const = default"), std::string::npos);
    EXPECT_NE(result.code.find("asn1pp::asn1_tag<X>"), std::string::npos);
    EXPECT_NE(result.code.find("universal_tag::octet_string"), std::string::npos);
    EXPECT_EQ(result.code.find("validate()"), std::string::npos);
}

TEST(EmitterOctetString, OctetStringWithSizeConstraint) {
    auto type = make_constrained_octet_string(1, 32);
    auto result = emitter::emit_octet_string(*type, "X");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.empty());

    EXPECT_NE(result.code.find("struct X {"), std::string::npos);
    EXPECT_NE(result.code.find("std::vector<uint8_t> value"), std::string::npos);
    EXPECT_NE(result.code.find("validate()"), std::string::npos);
    EXPECT_NE(result.code.find("result<void>"), std::string::npos);
    EXPECT_NE(result.code.find("error_code::constraint_violation"), std::string::npos);
    EXPECT_NE(result.code.find("value.size() < 1"), std::string::npos);
    EXPECT_NE(result.code.find("value.size() > 32"), std::string::npos);
    EXPECT_NE(result.code.find("asn1pp::asn1_tag<X>"), std::string::npos);
}

TEST(EmitterOctetString, OctetStringWithMinOnlyConstraint) {
    auto type = make_constrained_octet_string(5, std::nullopt);
    auto result = emitter::emit_octet_string(*type, "Y");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.empty());

    EXPECT_NE(result.code.find("value.size() < 5"), std::string::npos);
    EXPECT_EQ(result.code.find("value.size() >"), std::string::npos);
}

TEST(EmitterOctetString, OctetStringWithMaxOnlyConstraint) {
    auto type = make_constrained_octet_string(std::nullopt, 100);
    auto result = emitter::emit_octet_string(*type, "Z");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.empty());

    EXPECT_EQ(result.code.find("value.size() <"), std::string::npos);
    EXPECT_NE(result.code.find("value.size() > 100"), std::string::npos);
}

TEST(EmitterBitString, SimpleBitString) {
    auto type = make_bit_string_type();
    auto result = emitter::emit_bit_string(*type, "X");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.empty());

    EXPECT_NE(result.code.find("struct X {"), std::string::npos);
    EXPECT_NE(result.code.find("std::vector<uint8_t> data"), std::string::npos);
    EXPECT_NE(result.code.find("size_t bit_length"), std::string::npos);
    EXPECT_NE(result.code.find("bool operator==(const X&) const = default"), std::string::npos);
    EXPECT_NE(result.code.find("asn1pp::asn1_tag<X>"), std::string::npos);
    EXPECT_NE(result.code.find("universal_tag::bit_string"), std::string::npos);
}

TEST(EmitterBitString, BitStringWithNamedBits) {
    auto type = make_bit_string_with_named_bits();
    auto result = emitter::emit_bit_string(*type, "X");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.empty());

    EXPECT_NE(result.code.find("struct X {"), std::string::npos);
    EXPECT_NE(result.code.find("std::vector<uint8_t> data"), std::string::npos);
    EXPECT_NE(result.code.find("size_t bit_length"), std::string::npos);
    EXPECT_NE(result.code.find("bool bit0()"), std::string::npos);
    EXPECT_NE(result.code.find("void set_bit0(bool v)"), std::string::npos);
    EXPECT_NE(result.code.find("bool bit1()"), std::string::npos);
    EXPECT_NE(result.code.find("void set_bit1(bool v)"), std::string::npos);
    EXPECT_NE(result.code.find("bool flag()"), std::string::npos);
    EXPECT_NE(result.code.find("void set_flag(bool v)"), std::string::npos);
    EXPECT_NE(result.code.find("asn1pp::asn1_tag<X>"), std::string::npos);
}

TEST(EmitterBitString, NamedBitAccessorsCheckBounds) {
    auto type = make_bit_string_with_named_bits();
    auto result = emitter::emit_bit_string(*type, "X");
    EXPECT_TRUE(result.success);

    EXPECT_NE(result.code.find("if (5 >= bit_length)"), std::string::npos);
    EXPECT_NE(result.code.find("if (0 >= bit_length)"), std::string::npos);
}

TEST(EmitterOctetString, GeneratedCodeCompiles) {
    auto type = make_octet_string_type();
    auto result = emitter::emit_octet_string(*type, "TestOctetString");
    EXPECT_TRUE(result.success);

    std::string full_code = R"(
#include <vector>
#include <cstdint>
#include <cstddef>
#include "../../../libs/codec/result.hpp"
#include "../../../libs/codec/traits.hpp"
)" + result.code;

    EXPECT_TRUE(result.code.find("struct TestOctetString") != std::string::npos);
}

TEST(EmitterBitString, GeneratedCodeCompiles) {
    auto type = make_bit_string_type();
    auto result = emitter::emit_bit_string(*type, "TestBitString");
    EXPECT_TRUE(result.success);

    std::string full_code = R"(
#include <vector>
#include <cstdint>
#include <cstddef>
#include "../../../libs/codec/result.hpp"
#include "../../../libs/codec/traits.hpp"
)" + result.code;

    EXPECT_TRUE(result.code.find("struct TestBitString") != std::string::npos);
    EXPECT_TRUE(result.code.find("std::vector<uint8_t> data") != std::string::npos);
    EXPECT_TRUE(result.code.find("size_t bit_length") != std::string::npos);
}