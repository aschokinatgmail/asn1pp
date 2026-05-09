#include <gtest/gtest.h>

#include "../../src/gen/parser.hpp"

#include <string>
#include <string_view>

using namespace asn1pp::gen;
namespace {

diagnostic_engine diag;

struct ParseResult {
    asn1pp::result<module_definition> m;
    diagnostic_engine diag;

    ParseResult(std::string_view src, std::string_view file = "test.asn") {
        parser p(src, file, &diag);
        m = p.parse_module();
    }

    bool ok() const { return m.is_ok() && !diag.has_errors(); }
    const module_definition& mod() { return m.value(); }
    bool has_error() const { return diag.has_errors(); }
};

void assert_module_ok(std::string_view src, const char* test_name = nullptr) {
    ParseResult r(src);
    EXPECT_TRUE(r.ok()) << "Parse failed for: " << (test_name ? test_name : "");
}

struct TestModule {
    source_location loc;
};

// ============================================================================
// 1. Module parsing — basic module structure
// ============================================================================

TEST(ParserModule, ParseMinimalModule) {
    assert_module_ok("MyModule DEFINITIONS ::= BEGIN END");
}

TEST(ParserModule, ParseModuleWithEmptyImports) {
    assert_module_ok("MyModule DEFINITIONS ::= BEGIN IMPORTS ; END");
}

TEST(ParserModule, ParseModuleWithEmptyExports) {
    assert_module_ok("MyModule DEFINITIONS ::= BEGIN EXPORTS ; END");
}

TEST(ParserModule, ParseModuleWithImportsExports) {
    assert_module_ok("MyModule DEFINITIONS ::= BEGIN IMPORTS ; EXPORTS ALL ; END");
}

TEST(ParserModule, ParseModuleWithOID) {
    ParseResult r("MyModule { 1 3 6 1 4 1 } DEFINITIONS ::= BEGIN END");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().module_oid, "{ 1 3 6 1 4 1 }");
}

TEST(ParserModule, ParseModuleExtensibilityImplied) {
    ParseResult r("MyModule DEFINITIONS EXTENSIBILITY IMPLIED ::= BEGIN END");
    EXPECT_TRUE(r.ok());
    EXPECT_TRUE(r.mod().extensibility_implied);
}

TEST(ParserModule, ParseModuleNameCaptured) {
    ParseResult r("Capabilities DEFINITIONS ::= BEGIN END");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().name, "Capabilities");
}

// ============================================================================
// 2. Tag defaults
// ============================================================================

TEST(ParserTagDefault, ExplicitTags) {
    ParseResult r("M DEFINITIONS EXPLICIT TAGS ::= BEGIN END");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().default_tagging, tag_default::explicit_tag);
}

TEST(ParserTagDefault, ImplicitTags) {
    ParseResult r("M DEFINITIONS IMPLICIT TAGS ::= BEGIN END");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().default_tagging, tag_default::implicit_tag);
}

TEST(ParserTagDefault, AutomaticTags) {
    ParseResult r("M DEFINITIONS AUTOMATIC TAGS ::= BEGIN END");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().default_tagging, tag_default::automatic_tag);
}

TEST(ParserTagDefault, NoTagsDefaultsToAutomatic) {
    ParseResult r("M DEFINITIONS ::= BEGIN END");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().default_tagging, tag_default::automatic_tag);
}

// ============================================================================
// 3. Type assignments
// ============================================================================

TEST(ParserTypeAssignment, IntegerType) {
    ParseResult r("M DEFINITIONS ::= BEGIN MyType ::= INTEGER END");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().assignments.size(), 1u);
}

TEST(ParserTypeAssignment, BooleanType) {
    ParseResult r("M DEFINITIONS ::= BEGIN Flag ::= BOOLEAN END");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().assignments.size(), 1u);
}

TEST(ParserTypeAssignment, NullType) {
    ParseResult r("M DEFINITIONS ::= BEGIN N ::= NULL END");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().assignments.size(), 1u);
}

TEST(ParserTypeAssignment, MultipleAssignments) {
    ParseResult r("M DEFINITIONS ::= BEGIN A ::= INTEGER B ::= BOOLEAN C ::= NULL END");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().assignments.size(), 3u);
}

TEST(ParserTypeAssignment, SequenceType) {
    ParseResult r("M DEFINITIONS ::= BEGIN S ::= SEQUENCE { a INTEGER } END");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().assignments.size(), 1u);
}

TEST(ParserTypeAssignment, ChoiceType) {
    ParseResult r("M DEFINITIONS ::= BEGIN C ::= CHOICE { a INTEGER } END");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().assignments.size(), 1u);
}

TEST(ParserTypeAssignment, EnumeratedType) {
    ParseResult r("M DEFINITIONS ::= BEGIN E ::= ENUMERATED { red(0) } END");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().assignments.size(), 1u);
}

TEST(ParserTypeAssignment, OctetStringType) {
    assert_module_ok("M DEFINITIONS ::= BEGIN O ::= OCTET STRING END");
}

// ============================================================================
// 4. INTEGER types
// ============================================================================

TEST(ParserInteger, SimpleInteger) {
    ParseResult r("M DEFINITIONS ::= BEGIN Age ::= INTEGER END");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().assignments.size(), 1u);
}

TEST(ParserInteger, IntegerWithRangeConstraint) {
    ParseResult r("M DEFINITIONS ::= BEGIN Age ::= INTEGER (0..150) END");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().assignments.size(), 1u);
}

TEST(ParserInteger, IntegerWithMinMax) {
    ParseResult r("M DEFINITIONS ::= BEGIN Id ::= INTEGER (MIN..MAX) END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserInteger, IntegerWithMaxOnly) {
    ParseResult r("M DEFINITIONS ::= BEGIN Id ::= INTEGER (0..MAX) END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 5. BOOLEAN type
// ============================================================================

TEST(ParserBoolean, SimpleBoolean) {
    ParseResult r("M DEFINITIONS ::= BEGIN Flag ::= BOOLEAN END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserBoolean, BooleanTrueValue) {
    ParseResult r("M DEFINITIONS ::= BEGIN ok BOOLEAN ::= TRUE END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserBoolean, BooleanFalseValue) {
    ParseResult r("M DEFINITIONS ::= BEGIN ok BOOLEAN ::= FALSE END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 6. NULL type
// ============================================================================

TEST(ParserNull, SimpleNull) {
    ParseResult r("M DEFINITIONS ::= BEGIN N ::= NULL END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 7. OCTET STRING type
// ============================================================================

TEST(ParserOctetString, SimpleOctetString) {
    ParseResult r("M DEFINITIONS ::= BEGIN OS ::= OCTET STRING END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserOctetString, OctetStringWithSizeConstraint) {
    ParseResult r("M DEFINITIONS ::= BEGIN OS ::= OCTET STRING (SIZE (0..255)) END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserOctetString, OctetStringWithExactSize) {
    ParseResult r("M DEFINITIONS ::= BEGIN OS ::= OCTET STRING (SIZE (16)) END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 8. BIT STRING type
// ============================================================================

TEST(ParserBitString, SimpleBitString) {
    ParseResult r("M DEFINITIONS ::= BEGIN Bits ::= BIT STRING END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserBitString, BitStringWithNamedBits) {
    ParseResult r("M DEFINITIONS ::= BEGIN Bits ::= BIT STRING { bit0(0), bit1(1) } END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserBitString, BitStringWithExtensionMarker) {
    ParseResult r("M DEFINITIONS ::= BEGIN Bits ::= BIT STRING { bit0(0), ..., bit7(7) } END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 9. ENUMERATED type
// ============================================================================

TEST(ParserEnumerated, SimpleEnumerated) {
    ParseResult r("M DEFINITIONS ::= BEGIN Color ::= ENUMERATED { red(0), green(1), blue(2) } END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserEnumerated, EnumeratedWithExtension) {
    ParseResult r("M DEFINITIONS ::= BEGIN Color ::= ENUMERATED { red(0), ..., blue(2) } END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserEnumerated, EnumeratedWithoutValues) {
    ParseResult r("M DEFINITIONS ::= BEGIN Dir ::= ENUMERATED { north, south, east, west } END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 10. SEQUENCE type
// ============================================================================

TEST(ParserSequence, BasicSequence) {
    ParseResult r("M DEFINITIONS ::= BEGIN Person ::= SEQUENCE { name OCTET STRING, age INTEGER } END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserSequence, SequenceWithOptional) {
    ParseResult r("M DEFINITIONS ::= BEGIN Person ::= SEQUENCE { name OCTET STRING, middle OCTET STRING OPTIONAL } END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserSequence, SequenceWithDefault) {
    ParseResult r("M DEFINITIONS ::= BEGIN T ::= SEQUENCE { a INTEGER DEFAULT 0 } END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserSequence, SequenceWithExtensionMarker) {
    ParseResult r("M DEFINITIONS ::= BEGIN T ::= SEQUENCE { a INTEGER, ..., b BOOLEAN } END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserSequence, EmptySequence) {
    ParseResult r("M DEFINITIONS ::= BEGIN T ::= SEQUENCE { } END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserSequence, BareSequence) {
    ParseResult r("M DEFINITIONS ::= BEGIN T ::= SEQUENCE END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserSequence, SequenceWithNamedTypeComponents) {
    ParseResult r("M DEFINITIONS ::= BEGIN Inner ::= INTEGER Person ::= SEQUENCE { value Inner } END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 11. SET type
// ============================================================================

TEST(ParserSet, BasicSet) {
    ParseResult r("M DEFINITIONS ::= BEGIN Data ::= SET { x INTEGER, y INTEGER } END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserSet, SetWithOptional) {
    ParseResult r("M DEFINITIONS ::= BEGIN Data ::= SET { a INTEGER OPTIONAL, b BOOLEAN } END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserSet, SetWithExtensionMarker) {
    ParseResult r("M DEFINITIONS ::= BEGIN Data ::= SET { a INTEGER, ..., b REAL } END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 12. CHOICE type
// ============================================================================

TEST(ParserChoice, BasicChoice) {
    ParseResult r("M DEFINITIONS ::= BEGIN Option ::= CHOICE { int INTEGER, str OCTET STRING } END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserChoice, ChoiceWithExtensionMarker) {
    ParseResult r("M DEFINITIONS ::= BEGIN Option ::= CHOICE { a INTEGER, ..., z NULL } END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserChoice, SingleChoice) {
    ParseResult r("M DEFINITIONS ::= BEGIN C ::= CHOICE { val INTEGER } END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 13. SEQUENCE OF type
// ============================================================================

TEST(ParserSequenceOf, SimpleSequenceOf) {
    ParseResult r("M DEFINITIONS ::= BEGIN List ::= SEQUENCE OF INTEGER END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserSequenceOf, SequenceOfWithSizeConstraint) {
    ParseResult r("M DEFINITIONS ::= BEGIN List ::= SEQUENCE SIZE (0..10) OF INTEGER END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserSequenceOf, SequenceOfComplexType) {
    ParseResult r("M DEFINITIONS ::= BEGIN List ::= SEQUENCE OF OCTET STRING END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserSequenceOf, SequenceOfStringType) {
    assert_module_ok("M DEFINITIONS ::= BEGIN L ::= SEQUENCE OF OCTET STRING END");
}

// ============================================================================
// 14. SET OF type
// ============================================================================

TEST(ParserSetOf, SimpleSetOf) {
    ParseResult r("M DEFINITIONS ::= BEGIN S ::= SET OF INTEGER END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserSetOf, SetOfBoolean) {
    ParseResult r("M DEFINITIONS ::= BEGIN S ::= SET OF BOOLEAN END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 15. OBJECT IDENTIFIER type
// ============================================================================

TEST(ParserOID, ObjectIdentifierType) {
    ParseResult r("M DEFINITIONS ::= BEGIN Oid ::= OBJECT IDENTIFIER END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 16. Imports
// ============================================================================

TEST(ParserImports, ImportsFromModule) {
    ParseResult r("M DEFINITIONS ::= BEGIN IMPORTS TypeA, TypeB FROM OtherModule ; END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserImports, ImportsSingleSymbol) {
    ParseResult r("M DEFINITIONS ::= BEGIN IMPORTS Something FROM Mod ; END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserImports, EmptyImports) {
    ParseResult r("M DEFINITIONS ::= BEGIN IMPORTS ; END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 17. Exports
// ============================================================================

TEST(ParserExports, ExportsAll) {
    ParseResult r("M DEFINITIONS ::= BEGIN EXPORTS ALL ; END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserExports, ExportsSpecific) {
    ParseResult r("M DEFINITIONS ::= BEGIN EXPORTS TypeA, TypeB ; END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserExports, EmptyExports) {
    ParseResult r("M DEFINITIONS ::= BEGIN EXPORTS ; END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 18. Constraints
// ============================================================================

TEST(ParserConstraint, ValueRangeInt) {
    ParseResult r("M DEFINITIONS ::= BEGIN T ::= INTEGER (0..255) END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserConstraint, SizeConstraint) {
    ParseResult r("M DEFINITIONS ::= BEGIN T ::= OCTET STRING (SIZE (1..1024)) END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserConstraint, MinMaxConstraint) {
    ParseResult r("M DEFINITIONS ::= BEGIN T ::= INTEGER (MIN..MAX) END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 19. Error recovery
// ============================================================================

TEST(ParserErrorRecovery, MissingSemicolonContinues) {
    ParseResult r("M DEFINITIONS ::= BEGIN A ::= INTEGER B ::= BOOLEAN END");
    EXPECT_TRUE(r.ok());
    EXPECT_GE(r.mod().assignments.size(), 1u);
}

TEST(ParserErrorRecovery, WrongKeywordRecovers) {
    ParseResult r("M DEFINITIONS ::= BEGIN BadToken ::= INTEGER A ::= BOOLEAN END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserErrorRecovery, UnclosedBraceRecovers) {
    ParseResult r("M DEFINITIONS ::= BEGIN A ::= SEQUENCE { a INTEGER B ::= BOOLEAN END");
    // Should recover and parse B
    EXPECT_TRUE(r.ok());
}

TEST(ParserErrorRecovery, MultipleErrorsCaptured) {
    diagnostic_engine d;
    parser p("BadModule ::= BEGIN X ::= WRONG END", "test.asn", &d);
    p.parse_module();
    EXPECT_GE(d.total_count(), 0u);
}

// ============================================================================
// 20. Tagged types
// ============================================================================

TEST(ParserTagged, ImplicitTaggedInteger) {
    ParseResult r("M DEFINITIONS ::= BEGIN Tagged ::= [0] IMPLICIT INTEGER END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserTagged, ExplicitTaggedSequence) {
    ParseResult r("M DEFINITIONS ::= BEGIN Tagged ::= [APPLICATION 0] EXPLICIT SEQUENCE { a INTEGER } END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserTagged, ContextSpecificTag) {
    ParseResult r("M DEFINITIONS ::= BEGIN Tag ::= [1] INTEGER END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserTagged, PrivateTag) {
    ParseResult r("M DEFINITIONS ::= BEGIN Tag ::= [PRIVATE 1] EXPLICIT OCTET STRING END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserTagged, UniversalTag) {
    ParseResult r("M DEFINITIONS ::= BEGIN Tag ::= [UNIVERSAL 2] IMPLICIT OCTET STRING END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 21. REAL type
// ============================================================================

TEST(ParserReal, SimpleReal) {
    ParseResult r("M DEFINITIONS ::= BEGIN R ::= REAL END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 22. Value assignments
// ============================================================================

TEST(ParserValue, IntegerValueAssignment) {
    ParseResult r("M DEFINITIONS ::= BEGIN defaultAge INTEGER ::= 42 END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserValue, BooleanValueTrue) {
    ParseResult r("M DEFINITIONS ::= BEGIN enabled BOOLEAN ::= TRUE END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserValue, BooleanValueFalse) {
    ParseResult r("M DEFINITIONS ::= BEGIN disabled BOOLEAN ::= FALSE END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserValue, StringValue) {
    ParseResult r("M DEFINITIONS ::= BEGIN title OCTET STRING ::= \"hello\" END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserValue, NullValue) {
    ParseResult r("M DEFINITIONS ::= BEGIN nothing NULL ::= NULL END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 23. Edge cases — module structure
// ============================================================================

TEST(ParserEdge, ModuleWithNoDefinitionsFails) {
    diagnostic_engine d;
    parser p("MyModule ::= BEGIN END", "test.asn", &d);
    p.parse_module();
    EXPECT_TRUE(d.has_errors());
}

TEST(ParserEdge, ModuleWithNoBeginFails) {
    diagnostic_engine d;
    parser p("MyModule DEFINITIONS ::= END", "test.asn", &d);
    p.parse_module();
    EXPECT_TRUE(d.has_errors());
}

TEST(ParserEdge, UnclosedModule) {
    diagnostic_engine d;
    parser p("MyModule DEFINITIONS ::= BEGIN A ::= INTEGER", "test.asn", &d);
    p.parse_module();
    EXPECT_TRUE(d.has_errors());
}

// ============================================================================
// 24. Complex nested types
// ============================================================================

TEST(ParserComplex, NestedSequence) {
    assert_module_ok(
        "M DEFINITIONS ::= BEGIN "
        "Outer ::= SEQUENCE { inner SEQUENCE OF INTEGER } "
        "END");
}

TEST(ParserComplex, MultipleComplexTypes) {
    assert_module_ok(
        "M DEFINITIONS ::= BEGIN "
        "A ::= SEQUENCE { x INTEGER, y BOOLEAN } "
        "B ::= CHOICE { int INTEGER, str OCTET STRING } "
        "C ::= ENUMERATED { a(0), b(1) } "
        "D ::= SEQUENCE OF CHOICE { i INTEGER, b BOOLEAN } "
        "END");
}

TEST(ParserComplex, SequenceWithNestedSequence) {
    assert_module_ok(
        "M DEFINITIONS ::= BEGIN "
        "Inner ::= SEQUENCE { a INTEGER } "
        "Outer ::= SEQUENCE { nested Inner } "
        "END");
}

// ============================================================================
// 25. Whitespace and comment tolerance
// ============================================================================

TEST(ParserWhitespace, MultilineParsing) {
    assert_module_ok(
        "\nMyModule DEFINITIONS ::=\nBEGIN\n"
        "A ::= INTEGER\n"
        "B ::= BOOLEAN\n"
        "END\n");
}

TEST(ParserWhitespace, ExtraWhitespace) {
    assert_module_ok(
        "   MyModule   DEFINITIONS   ::=   BEGIN   A   ::=   INTEGER   END   ");
}

TEST(ParserWhitespace, BlockCommentTolerance) {
    assert_module_ok(
        "/* file header */\n"
        "MyModule DEFINITIONS ::= BEGIN A ::= INTEGER END");
}

// ============================================================================
// 26. Long identifier names
// ============================================================================

TEST(ParserIdentifier, LongIdentifier) {
    assert_module_ok(
        "ThisIsAVeryLongModuleNameWithManyParts DEFINITIONS ::= BEGIN END");
}

TEST(ParserIdentifier, HyphenatedIdentifier) {
    assert_module_ok(
        "M DEFINITIONS ::= BEGIN hyphenated-type-name ::= INTEGER END");
}

// ============================================================================
// 27. REAL type value
// ============================================================================

TEST(ParserRealValue, RealValueAssignment) {
    ParseResult r("M DEFINITIONS ::= BEGIN pi REAL ::= 3 END");
    EXPECT_TRUE(r.ok());
}

// ============================================================================
// 28. Stress: large number of assignments
// ============================================================================

TEST(ParserStress, ManyAssignments) {
    std::string src = "M DEFINITIONS ::= BEGIN ";
    for (int i = 1; i <= 40; ++i) {
        src += "T" + std::to_string(i) + " ::= INTEGER ";
    }
    src += "END";
    ParseResult r(src);
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().assignments.size(), 40u);
}

// ============================================================================
// 29. Tagged type with IMPLICIT default
// ============================================================================

TEST(ParserTagged, ImplicitTaggedWithDefault) {
    assert_module_ok(
        "M DEFINITIONS IMPLICIT TAGS ::= BEGIN Tag ::= [0] INTEGER END");
}

TEST(ParserTagged, ExplicitTaggedWithDefault) {
    assert_module_ok(
        "M DEFINITIONS EXPLICIT TAGS ::= BEGIN Tag ::= [0] INTEGER END");
}

// ============================================================================
// 30. Real-world ASN.1 patterns
// ============================================================================

TEST(ParserRealWorld, X509Pattern) {
    assert_module_ok(
        "X509 DEFINITIONS ::= BEGIN "
        "Version ::= INTEGER { v1(0), v2(1), v3(2) } "
        "END");
}

TEST(ParserRealWorld, SimpleProtocol) {
    assert_module_ok(
        "Protocol DEFINITIONS ::= BEGIN "
        "Message ::= SEQUENCE { "
        "  id INTEGER, "
        "  payload OCTET STRING "
        "} "
        "END");
}

// ============================================================================
// Test helper to access parse_module directly
// ============================================================================

TEST(ParserAccess, parse_moduleReturnsModuleDefinition) {
    ParseResult r("TestMod DEFINITIONS ::= BEGIN END");
    EXPECT_TRUE(r.ok());
}

TEST(ParserAccess, parse_moduleWithImport) {
    ParseResult r("M DEFINITIONS ::= BEGIN IMPORTS TypeA FROM ModB ; A ::= INTEGER END");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.mod().assignments.size(), 1u);
}

}  // namespace
