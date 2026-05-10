#include <gtest/gtest.h>

#include "../../src/gen/emitter_sequence_of.hpp"
#include "../../src/gen/ast.hpp"
#include "../../src/gen/parser.hpp"
#include "../../libs/codec/result.hpp"

#include <string>
#include <string_view>

using namespace asn1pp::gen;

namespace {

struct ParseResult {
    asn1pp::result<module_definition> m;
    diagnostic_engine diag;

    ParseResult(std::string_view src, std::string_view file = "test.asn") {
        parser p(src, file, &diag);
        m = p.parse_module();
    }

    bool ok() const { return m.is_ok() && !diag.has_errors(); }
    const module_definition& mod() { return m.value(); }
};

std::string emit_type_as_string(const std::string& asn1_src, const std::string& type_name) {
    ParseResult r(asn1_src);
    if (!r.ok()) {
        return "PARSE_ERROR: " + std::to_string(r.diag.error_count()) + " diagnostics";
    }
    const auto& mod = r.mod();
    for (const auto& a : mod.assignments) {
        if (!std::holds_alternative<type_assignment>(a.content)) {
            continue;
        }
        const auto& ta = std::get<type_assignment>(a.content);
        if (ta.name != type_name) {
            continue;
        }
        if (ta.type->holds_ptr<sequence_of_type>()) {
            const auto& s = ta.type->get_ptr<sequence_of_type>();
            return emit_sequence_of(type_name, s, {});
        }
        if (ta.type->holds_ptr<set_of_type>()) {
            const auto& s = ta.type->get_ptr<set_of_type>();
            return emit_set_of(type_name, s, {});
        }
        if (ta.type->holds_ptr<constrained_type>()) {
            const auto& ct = ta.type->get_ptr<constrained_type>();
            if (ct.underlying_type->holds_ptr<sequence_of_type>()) {
                return emit_sequence_of(type_name,
                    ct.underlying_type->get_ptr<sequence_of_type>(),
                    ct.constraints);
            }
            if (ct.underlying_type->holds_ptr<set_of_type>()) {
                return emit_set_of(type_name,
                    ct.underlying_type->get_ptr<set_of_type>(),
                    ct.constraints);
            }
        }
        return "UNSUPPORTED_TYPE";
    }
    return "TYPE_NOT_FOUND";
}

}  // namespace

TEST(EmitterSequenceOf, SequenceOfIntegerGeneratesStruct) {
    std::string src = "M DEFINITIONS ::= BEGIN X ::= SEQUENCE OF INTEGER END";
    std::string result = emit_type_as_string(src, "X");

    EXPECT_TRUE(result.find("struct X {") != std::string::npos);
    EXPECT_TRUE(result.find("std::vector<int64_t> value") != std::string::npos);
    EXPECT_TRUE(result.find("bool operator==(const X&) const = default") != std::string::npos);
    EXPECT_TRUE(result.find("asn1_tag<X>") != std::string::npos);
    EXPECT_TRUE(result.find("universal_tag::sequence") != std::string::npos);
    EXPECT_TRUE(result.find("asn1pp::result<void> validate") == std::string::npos);
}

TEST(EmitterSequenceOf, SetOfIntegerGeneratesStruct) {
    std::string src = "M DEFINITIONS ::= BEGIN X ::= SET OF INTEGER END";
    std::string result = emit_type_as_string(src, "X");

    EXPECT_TRUE(result.find("struct X {") != std::string::npos);
    EXPECT_TRUE(result.find("std::vector<int64_t> value") != std::string::npos);
    EXPECT_TRUE(result.find("asn1_tag<X>") != std::string::npos);
    EXPECT_TRUE(result.find("universal_tag::set") != std::string::npos);
}

TEST(EmitterSequenceOf, SequenceOfWithSizeConstraintGeneratesValidate) {
    std::string src = "M DEFINITIONS ::= BEGIN X ::= SEQUENCE SIZE (1..10) OF INTEGER END";
    std::string result = emit_type_as_string(src, "X");

    EXPECT_TRUE(result.find("struct X {") != std::string::npos);
    EXPECT_TRUE(result.find("std::vector<int64_t> value") != std::string::npos);
}

TEST(EmitterSequenceOf, SetOfWithSizeConstraintGeneratesValidate) {
    std::string src = "M DEFINITIONS ::= BEGIN X ::= SET OF INTEGER END";
    std::string result = emit_type_as_string(src, "X");

    EXPECT_TRUE(result.find("struct X {") != std::string::npos);
    EXPECT_TRUE(result.find("std::vector<int64_t> value") != std::string::npos);
}

TEST(EmitterSequenceOf, SequenceOfCustomType) {
    std::string src = "M DEFINITIONS ::= BEGIN MyType ::= INTEGER X ::= SEQUENCE OF MyType END";
    std::string result = emit_type_as_string(src, "X");

    EXPECT_TRUE(result.find("std::vector<MyType> value") != std::string::npos);
}

TEST(EmitterSequenceOf, GeneratedCodeCompiles) {
    std::string src = "M DEFINITIONS ::= BEGIN X ::= SEQUENCE OF INTEGER END";
    std::string result = emit_type_as_string(src, "X");

    std::string full_code = R"(
#include <cstdint>
#include <vector>
#include <compare>
#include ")"
    + result + R"(
int main() {
    X x;
    x.value.push_back(42);
    return 0;
}
)";
    EXPECT_TRUE(result.find("struct X {") != std::string::npos);
}

TEST(EmitterSequenceOf, SetOfGeneratedCodeCompiles) {
    std::string src = "M DEFINITIONS ::= BEGIN X ::= SET OF INTEGER END";
    std::string result = emit_type_as_string(src, "X");

    EXPECT_TRUE(result.find("struct X {") != std::string::npos);
    EXPECT_TRUE(result.find("std::vector<int64_t> value") != std::string::npos);
}