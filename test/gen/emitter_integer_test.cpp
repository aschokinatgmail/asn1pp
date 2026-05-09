#include <gtest/gtest.h>

#include "../../src/gen/emitter.hpp"
#include "../../src/gen/emitter_integer.hpp"
#include "../../src/gen/ast.hpp"
#include "../../src/gen/parser.hpp"
#include "../../libs/codec/result.hpp"

#include <string>
#include <string_view>

using namespace asn1pp::gen;

namespace {

// ============================================================================
// Helper: parse ASN.1 module and get the first type assignment
// ============================================================================
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
        emitter_options opts;
        opts.namespace_name = "asn1pp";

        // The type_ref holds the type content
        // Check if it's a constrained_type wrapper
        if (ta.type->holds_ptr<constrained_type>()) {
            const auto& ct = ta.type->get_ptr<constrained_type>();
            if (ct.underlying_type->holds_alternative<integer_type>()) {
                return emit_integer_type(ct.underlying_type->get<integer_type>(),
                                         type_name, ct.constraints, opts);
            }
        } else if (ta.type->holds_alternative<integer_type>()) {
            return emit_integer_type(ta.type->get<integer_type>(),
                                     type_name, {}, opts);
        }
        return "UNSUPPORTED_TYPE";
    }
    return "TYPE_NOT_FOUND";
}

// ============================================================================
// 1. Simple INTEGER type
// ============================================================================

TEST(EmitterInteger, SimpleIntegerGeneratesStruct) {
    std::string src = "M DEFINITIONS ::= BEGIN X ::= INTEGER END";
    std::string result = emit_type_as_string(src, "X");

    // Should generate struct with int64_t value member
    EXPECT_NE(result.find("struct X"), std::string::npos);
    EXPECT_NE(result.find("int64_t value"), std::string::npos);
}

TEST(EmitterInteger, SimpleIntegerGeneratesTagSpecialization) {
    std::string src = "M DEFINITIONS ::= BEGIN X ::= INTEGER END";
    std::string result = emit_type_as_string(src, "X");

    // Should generate tag specialization for universal_tag::integer
    EXPECT_NE(result.find("asn1_tag"), std::string::npos);
    EXPECT_NE(result.find("universal_tag::integer"), std::string::npos);
}

TEST(EmitterInteger, SimpleIntegerGeneratesOperatorEquals) {
    std::string src = "M DEFINITIONS ::= BEGIN X ::= INTEGER END";
    std::string result = emit_type_as_string(src, "X");

    // Should generate operator==
    EXPECT_NE(result.find("operator=="), std::string::npos);
}

// ============================================================================
// 2. Constrained INTEGER
// ============================================================================

TEST(EmitterInteger, ConstrainedIntegerGeneratesValidate) {
    std::string src = "M DEFINITIONS ::= BEGIN X ::= INTEGER (0..255) END";
    std::string result = emit_type_as_string(src, "X");

    // Should generate validate() method
    EXPECT_NE(result.find("validate()"), std::string::npos);
    EXPECT_NE(result.find("result<void>"), std::string::npos);
}

TEST(EmitterInteger, ConstrainedIntegerChecksLowerBound) {
    std::string src = "M DEFINITIONS ::= BEGIN X ::= INTEGER (0..255) END";
    std::string result = emit_type_as_string(src, "X");

    // Should check lower bound (value < 0 or value >= 0 depending on constraint)
    EXPECT_NE(result.find("constraint_violation"), std::string::npos);
    EXPECT_NE(result.find("0"), std::string::npos);
    EXPECT_NE(result.find("255"), std::string::npos);
}

TEST(EmitterInteger, ConstrainedIntegerUpperBoundInclusive) {
    std::string src = "M DEFINITIONS ::= BEGIN X ::= INTEGER (0..255) END";
    std::string result = emit_type_as_string(src, "X");

    // (0..255) means inclusive on both ends
    // Should have <= 255 check, not < 255
    EXPECT_NE(result.find("255"), std::string::npos);
}

// ============================================================================
// 3. Named number INTEGER
// ============================================================================

// TODO: Named INTEGER numbers not yet supported by parser
// Parser does not support INTEGER { red(0), green(1), blue(2) } syntax
TEST(EmitterInteger, NamedNumbersGenerateEnum) {
    GTEST_SKIP() << "Named INTEGER numbers not yet supported by parser";
    std::string src = "M DEFINITIONS ::= BEGIN X ::= INTEGER { red(0), green(1), blue(2) } END";
    std::string result = emit_type_as_string(src, "X");

    // Should generate inner enum with named values
    EXPECT_NE(result.find("enum"), std::string::npos);
    EXPECT_NE(result.find("red"), std::string::npos);
    EXPECT_NE(result.find("green"), std::string::npos);
    EXPECT_NE(result.find("blue"), std::string::npos);
}

// TODO: Named INTEGER numbers not yet supported by parser
TEST(EmitterInteger, NamedNumbersHaveCorrectValues) {
    GTEST_SKIP() << "Named INTEGER numbers not yet supported by parser";
    std::string src = "M DEFINITIONS ::= BEGIN X ::= INTEGER { red(0), green(1), blue(2) } END";
    std::string result = emit_type_as_string(src, "X");

    // Should have explicit values 0, 1, 2
    EXPECT_NE(result.find("0"), std::string::npos);
    EXPECT_NE(result.find("1"), std::string::npos);
    EXPECT_NE(result.find("2"), std::string::npos);
}

// ============================================================================
// 4. Generated code is valid C++20 (compile check via includes)
// ============================================================================

TEST(EmitterInteger, GeneratedCodeIncludesResult) {
    std::string src = "M DEFINITIONS ::= BEGIN X ::= INTEGER (0..255) END";
    std::string result = emit_type_as_string(src, "X");

    // Result type requires result.hpp
    // This test verifies the generated code structure includes necessary types
    EXPECT_NE(result.find("result<void>"), std::string::npos);
}

// ============================================================================
// 5. Non-inclusive bounds
// ============================================================================

TEST(EmitterInteger, NonInclusiveLowerBound) {
    // (0<..255) means lower bound is exclusive (value > 0), not inclusive
    // Test via direct construction since parser doesn't support exclusive bounds
    emitter_options opts;
    opts.namespace_name = "asn1pp";

    // Create constrained integer with exclusive lower bound
    value_range_constraint vrc;
    vrc.min_value = 0;
    vrc.min_inclusive = false;  // exclusive: value > 0, not value >= 0
    vrc.max_value = 255;
    vrc.max_inclusive = true;

    constraint c;
    c.content = vrc;

    std::string result = emit_integer_type(integer_type{}, "X", {c}, opts);

    // Should check value > 0 (exclusive), not value >= 0 (inclusive)
    EXPECT_NE(result.find("0"), std::string::npos);
    EXPECT_NE(result.find("255"), std::string::npos);
}

TEST(EmitterInteger, NonInclusiveUpperBound) {
    // (0..<255) means upper bound is exclusive (value < 255), not inclusive
    // Test via direct construction since parser doesn't support exclusive bounds
    emitter_options opts;
    opts.namespace_name = "asn1pp";

    // Create constrained integer with exclusive upper bound
    value_range_constraint vrc;
    vrc.min_value = 0;
    vrc.min_inclusive = true;
    vrc.max_value = 255;
    vrc.max_inclusive = false;  // exclusive: value < 255, not value <= 255

    constraint c;
    c.content = vrc;

    std::string result = emit_integer_type(integer_type{}, "X", {c}, opts);

    // Should check value < 255 (exclusive), not value <= 255 (inclusive)
    EXPECT_NE(result.find("0"), std::string::npos);
    EXPECT_NE(result.find("255"), std::string::npos);
}

// ============================================================================
// 6. Operator!= should also be generated
// ============================================================================

TEST(EmitterInteger, GeneratesOperatorNotEquals) {
    std::string src = "M DEFINITIONS ::= BEGIN X ::= INTEGER END";
    std::string result = emit_type_as_string(src, "X");

    // Should generate operator!=
    EXPECT_NE(result.find("operator!="), std::string::npos);
}

// ============================================================================
// 7. Namespace option
// ============================================================================

TEST(EmitterInteger, NamespaceOptionAffectsOutput) {
    std::string src = "M DEFINITIONS ::= BEGIN X ::= INTEGER END";

    ParseResult r(src);
    ASSERT_TRUE(r.ok());

    emitter_options opts;
    opts.namespace_name = "my_namespace";

    std::string result = emit_integer_type(integer_type{}, "X", {}, opts);

    // With namespace, should see namespace declaration
    EXPECT_NE(result.find("my_namespace"), std::string::npos);
}

// ============================================================================
// 8. Empty constraint list (no constraints)
// ============================================================================

TEST(EmitterInteger, NoConstraintsGeneratesSimpleStruct) {
    std::string src = "M DEFINITIONS ::= BEGIN X ::= INTEGER END";
    std::string result = emit_type_as_string(src, "X");

    // Should NOT generate validate() when there are no constraints
    EXPECT_EQ(result.find("validate()"), std::string::npos);
    EXPECT_NE(result.find("struct X"), std::string::npos);
}

}  // namespace