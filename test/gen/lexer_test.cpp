#include <gtest/gtest.h>
#include "../../src/gen/lexer.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace asn1pp::gen;

token single_token(std::string_view src) {
    lexer lex(src, "test.asn");
    return lex.next_token();
}

void assert_token(token t, token_type expected_type, std::string_view expected_value) {
    EXPECT_EQ(t.type, expected_type) << "value=" << t.value;
    EXPECT_EQ(t.value, expected_value);
}

void assert_token_after(lexer& lex, token_type expected_type, std::string_view expected_value) {
    token t = lex.next_token();
    assert_token(t, expected_type, expected_value);
}

void assert_eof(lexer& lex) {
    token t = lex.next_token();
    EXPECT_EQ(t.type, token_type::eof);
    EXPECT_EQ(t.value, "");
}

// ============================================================================
// 1. Keyword tests — every X.680 keyword
// ============================================================================

TEST(LexerKeywords, INTEGER) {
    assert_token(single_token("INTEGER"), token_type::kw_integer, "INTEGER");
}

TEST(LexerKeywords, BOOLEAN) {
    assert_token(single_token("BOOLEAN"), token_type::kw_boolean, "BOOLEAN");
}

TEST(LexerKeywords, NULL_keyword) {
    assert_token(single_token("NULL"), token_type::kw_null, "NULL");
}

TEST(LexerKeywords, REAL) {
    assert_token(single_token("REAL"), token_type::kw_real, "REAL");
}

TEST(LexerKeywords, BIT) {
    assert_token(single_token("BIT"), token_type::kw_bit, "BIT");
}

TEST(LexerKeywords, OCTET) {
    assert_token(single_token("OCTET"), token_type::kw_octet, "OCTET");
}

TEST(LexerKeywords, SEQUENCE) {
    assert_token(single_token("SEQUENCE"), token_type::kw_sequence, "SEQUENCE");
}

TEST(LexerKeywords, SET) {
    assert_token(single_token("SET"), token_type::kw_set, "SET");
}

TEST(LexerKeywords, CHOICE) {
    assert_token(single_token("CHOICE"), token_type::kw_choice, "CHOICE");
}

TEST(LexerKeywords, OF) {
    assert_token(single_token("OF"), token_type::kw_of, "OF");
}

TEST(LexerKeywords, OPTIONAL) {
    assert_token(single_token("OPTIONAL"), token_type::kw_optional, "OPTIONAL");
}

TEST(LexerKeywords, DEFAULT) {
    assert_token(single_token("DEFAULT"), token_type::kw_default, "DEFAULT");
}

TEST(LexerKeywords, COMPONENTS) {
    assert_token(single_token("COMPONENTS"), token_type::kw_components, "COMPONENTS");
}

TEST(LexerKeywords, IMPORTS) {
    assert_token(single_token("IMPORTS"), token_type::kw_imports, "IMPORTS");
}

TEST(LexerKeywords, EXPORTS) {
    assert_token(single_token("EXPORTS"), token_type::kw_exports, "EXPORTS");
}

TEST(LexerKeywords, FROM) {
    assert_token(single_token("FROM"), token_type::kw_from, "FROM");
}

TEST(LexerKeywords, DEFINITIONS) {
    assert_token(single_token("DEFINITIONS"), token_type::kw_definitions, "DEFINITIONS");
}

TEST(LexerKeywords, BEGIN) {
    assert_token(single_token("BEGIN"), token_type::kw_begin, "BEGIN");
}

TEST(LexerKeywords, END) {
    assert_token(single_token("END"), token_type::kw_end, "END");
}

TEST(LexerKeywords, TAGGED) {
    assert_token(single_token("TAGGED"), token_type::kw_tagged, "TAGGED");
}

TEST(LexerKeywords, IMPLICIT) {
    assert_token(single_token("IMPLICIT"), token_type::kw_implicit, "IMPLICIT");
}

TEST(LexerKeywords, EXPLICIT) {
    assert_token(single_token("EXPLICIT"), token_type::kw_explicit, "EXPLICIT");
}

TEST(LexerKeywords, AUTOMATIC) {
    assert_token(single_token("AUTOMATIC"), token_type::kw_automatic, "AUTOMATIC");
}

TEST(LexerKeywords, APPLICATION) {
    assert_token(single_token("APPLICATION"), token_type::kw_application, "APPLICATION");
}

TEST(LexerKeywords, UNIVERSAL) {
    assert_token(single_token("UNIVERSAL"), token_type::kw_universal, "UNIVERSAL");
}

TEST(LexerKeywords, PRIVATE) {
    assert_token(single_token("PRIVATE"), token_type::kw_private, "PRIVATE");
}

TEST(LexerKeywords, ENUMERATED) {
    assert_token(single_token("ENUMERATED"), token_type::kw_enumerated, "ENUMERATED");
}

TEST(LexerKeywords, MIN) {
    assert_token(single_token("MIN"), token_type::kw_min, "MIN");
}

TEST(LexerKeywords, MAX) {
    assert_token(single_token("MAX"), token_type::kw_max, "MAX");
}

TEST(LexerKeywords, SIZE) {
    assert_token(single_token("SIZE"), token_type::kw_size, "SIZE");
}

TEST(LexerKeywords, CONSTRAINT) {
    assert_token(single_token("CONSTRAINT"), token_type::kw_constraint, "CONSTRAINT");
}

TEST(LexerKeywords, WITH) {
    assert_token(single_token("WITH"), token_type::kw_with, "WITH");
}

TEST(LexerKeywords, SELECTED) {
    assert_token(single_token("SELECTED"), token_type::kw_selected, "SELECTED");
}

TEST(LexerKeywords, TRUE) {
    assert_token(single_token("TRUE"), token_type::kw_true, "TRUE");
}

TEST(LexerKeywords, FALSE) {
    assert_token(single_token("FALSE"), token_type::kw_false, "FALSE");
}

TEST(LexerKeywords, EXTENDABILITY) {
    assert_token(single_token("EXTENDABILITY"), token_type::kw_extendability, "EXTENDABILITY");
}

TEST(LexerKeywords, PRESENT) {
    assert_token(single_token("PRESENT"), token_type::kw_present, "PRESENT");
}

TEST(LexerKeywords, ABSENT) {
    assert_token(single_token("ABSENT"), token_type::kw_absent, "ABSENT");
}

TEST(LexerKeywords, ALL) {
    assert_token(single_token("ALL"), token_type::kw_all, "ALL");
}

TEST(LexerKeywords, CLASS) {
    assert_token(single_token("CLASS"), token_type::kw_class, "CLASS");
}

TEST(LexerKeywords, UNIQUE) {
    assert_token(single_token("UNIQUE"), token_type::kw_unique, "UNIQUE");
}

TEST(LexerKeywords, INSTANCE) {
    assert_token(single_token("INSTANCE"), token_type::kw_instance, "INSTANCE");
}

// ============================================================================
// 2. Case insensitivity tests (ASN.1 keywords are case-insensitive)
// ============================================================================

TEST(LexerCaseInsensitive, IntegerLowercase) {
    assert_token(single_token("integer"), token_type::kw_integer, "integer");
}

TEST(LexerCaseInsensitive, IntegerMixed) {
    assert_token(single_token("Integer"), token_type::kw_integer, "Integer");
}

TEST(LexerCaseInsensitive, IntegerUppercase) {
    assert_token(single_token("INTEGER"), token_type::kw_integer, "INTEGER");
}

TEST(LexerCaseInsensitive, BooleanLower) {
    assert_token(single_token("boolean"), token_type::kw_boolean, "boolean");
}

TEST(LexerCaseInsensitive, BooleanMixed) {
    assert_token(single_token("Boolean"), token_type::kw_boolean, "Boolean");
}

TEST(LexerCaseInsensitive, NullLower) {
    assert_token(single_token("null"), token_type::kw_null, "null");
}

TEST(LexerCaseInsensitive, NullMixed) {
    assert_token(single_token("Null"), token_type::kw_null, "Null");
}

TEST(LexerCaseInsensitive, SequenceLower) {
    assert_token(single_token("sequence"), token_type::kw_sequence, "sequence");
}

TEST(LexerCaseInsensitive, SequenceMixed) {
    assert_token(single_token("Sequence"), token_type::kw_sequence, "Sequence");
}

TEST(LexerCaseInsensitive, SetLower) {
    assert_token(single_token("set"), token_type::kw_set, "set");
}

TEST(LexerCaseInsensitive, ChoiceLower) {
    assert_token(single_token("choice"), token_type::kw_choice, "choice");
}

TEST(LexerCaseInsensitive, OptionalLower) {
    assert_token(single_token("optional"), token_type::kw_optional, "optional");
}

TEST(LexerCaseInsensitive, TrueLower) {
    assert_token(single_token("true"), token_type::kw_true, "true");
}

TEST(LexerCaseInsensitive, FalseLower) {
    assert_token(single_token("false"), token_type::kw_false, "false");
}

TEST(LexerCaseInsensitive, BeginLower) {
    assert_token(single_token("begin"), token_type::kw_begin, "begin");
}

TEST(LexerCaseInsensitive, EndLower) {
    assert_token(single_token("end"), token_type::kw_end, "end");
}

// ============================================================================
// 3. Identifier tests
// ============================================================================

TEST(LexerIdentifiers, SimpleName) {
    assert_token(single_token("myModule"), token_type::identifier, "myModule");
}

TEST(LexerIdentifiers, TypeName) {
    assert_token(single_token("MyType"), token_type::identifier, "MyType");
}

TEST(LexerIdentifiers, UnderscoreStart) {
    assert_token(single_token("_underscore_start"), token_type::identifier, "_underscore_start");
}

TEST(LexerIdentifiers, WithHyphen) {
    assert_token(single_token("my-module"), token_type::identifier, "my-module");
}

TEST(LexerIdentifiers, WithNumbers) {
    assert_token(single_token("field123"), token_type::identifier, "field123");
}

TEST(LexerIdentifiers, SingleLetter) {
    assert_token(single_token("x"), token_type::identifier, "x");
}

TEST(LexerIdentifiers, SingleUnderscore) {
    assert_token(single_token("_"), token_type::identifier, "_");
}

TEST(LexerIdentifiers, UppercaseIdentifier) {
    assert_token(single_token("FOO"), token_type::identifier, "FOO");
}

TEST(LexerIdentifiers, IdentifierWithHyphenMiddle) {
    assert_token(single_token("obj-id"), token_type::identifier, "obj-id");
}

// ============================================================================
// 4. Number tests
// ============================================================================

TEST(LexerNumbers, Zero) {
    assert_token(single_token("0"), token_type::number, "0");
}

TEST(LexerNumbers, SingleDigit) {
    assert_token(single_token("7"), token_type::number, "7");
}

TEST(LexerNumbers, MultiDigit) {
    assert_token(single_token("42"), token_type::number, "42");
}

TEST(LexerNumbers, LargeNumber) {
    assert_token(single_token("12345"), token_type::number, "12345");
}

TEST(LexerNumbers, MaxUint32) {
    assert_token(single_token("4294967295"), token_type::number, "4294967295");
}

TEST(LexerNumbers, LeadingZeros) {
    assert_token(single_token("00123"), token_type::number, "00123");
}

TEST(LexerNumbers, SequenceOfNumbers) {
    lexer lex("1 23 456", "test.asn");
    assert_token_after(lex, token_type::number, "1");
    assert_token_after(lex, token_type::number, "23");
    assert_token_after(lex, token_type::number, "456");
    assert_eof(lex);
}

// ============================================================================
// 5. Character string tests
// ============================================================================

TEST(LexerStrings, EmptyString) {
    assert_token(single_token("\"\""), token_type::character_string, "");
}

TEST(LexerStrings, SimpleString) {
    assert_token(single_token("\"hello\""), token_type::character_string, "hello");
}

TEST(LexerStrings, WithSpaces) {
    assert_token(single_token("\"hello world\""), token_type::character_string, "hello world");
}

TEST(LexerStrings, WithEscapedQuote) {
    assert_token(single_token("\"hello \\\"world\\\"\""), token_type::character_string, "hello \"world\"");
}

TEST(LexerStrings, WithEscapedBackslash) {
    assert_token(single_token("\"path\\\\to\\\\file\""), token_type::character_string, "path\\to\\file");
}

TEST(LexerStrings, WithDigits) {
    assert_token(single_token("\"abc123\""), token_type::character_string, "abc123");
}

// ============================================================================
// 6. Binary string tests
// ============================================================================

TEST(LexerBinaryStrings, SimpleBinary) {
    assert_token(single_token("'0101'B"), token_type::binary_string, "0101");
}

TEST(LexerBinaryStrings, AllZeros) {
    assert_token(single_token("'0000'B"), token_type::binary_string, "0000");
}

TEST(LexerBinaryStrings, AllOnes) {
    assert_token(single_token("'11110000'B"), token_type::binary_string, "11110000");
}

TEST(LexerBinaryStrings, SingleBit) {
    assert_token(single_token("'0'B"), token_type::binary_string, "0");
}

TEST(LexerBinaryStrings, LowercaseB) {
    assert_token(single_token("'0101'b"), token_type::binary_string, "0101");
}

// ============================================================================
// 7. Hex string tests
// ============================================================================

TEST(LexerHexStrings, SimpleHex) {
    assert_token(single_token("'A0FF'H"), token_type::hex_string, "A0FF");
}

TEST(LexerHexStrings, ZeroValue) {
    assert_token(single_token("'00'H"), token_type::hex_string, "00");
}

TEST(LexerHexStrings, SingleDigith) {
    assert_token(single_token("'F'H"), token_type::hex_string, "F");
}

TEST(LexerHexStrings, LowercaseH) {
    assert_token(single_token("'a0ff'h"), token_type::hex_string, "a0ff");
}

// ============================================================================
// 8. Operator / symbol tests
// ============================================================================

TEST(LexerOperators, Assignment) {
    assert_token(single_token("::="), token_type::assignment, "::=");
}

TEST(LexerOperators, RangeDoubleDot) {
    assert_token(single_token(".."), token_type::range, "..");
}

TEST(LexerOperators, Ellipsis) {
    assert_token(single_token("..."), token_type::ellipsis, "...");
}

TEST(LexerOperators, LeftBrace) {
    assert_token(single_token("{"), token_type::left_brace, "{");
}

TEST(LexerOperators, RightBrace) {
    assert_token(single_token("}"), token_type::right_brace, "}");
}

TEST(LexerOperators, LeftBracket) {
    assert_token(single_token("["), token_type::left_bracket, "[");
}

TEST(LexerOperators, RightBracket) {
    assert_token(single_token("]"), token_type::right_bracket, "]");
}

TEST(LexerOperators, LeftParen) {
    assert_token(single_token("("), token_type::left_paren, "(");
}

TEST(LexerOperators, RightParen) {
    assert_token(single_token(")"), token_type::right_paren, ")");
}

TEST(LexerOperators, Comma) {
    assert_token(single_token(","), token_type::comma, ",");
}

TEST(LexerOperators, Semicolon) {
    assert_token(single_token(";"), token_type::semicolon, ";");
}

TEST(LexerOperators, Pipe) {
    assert_token(single_token("|"), token_type::pipe, "|");
}

TEST(LexerOperators, Caret) {
    assert_token(single_token("^"), token_type::caret, "^");
}

TEST(LexerOperators, Ampersand) {
    assert_token(single_token("&"), token_type::ampersand, "&");
}

TEST(LexerOperators, Colon) {
    assert_token(single_token(":"), token_type::colon, ":");
}

// ============================================================================
// 9. Comment tests
// ============================================================================

TEST(LexerComments, LineComment) {
    lexer lex("-- this is a comment\nINTEGER", "test.asn");
    assert_token_after(lex, token_type::kw_integer, "INTEGER");
    assert_eof(lex);
}

TEST(LexerComments, LineCommentAtEnd) {
    lexer lex("INTEGER -- comment", "test.asn");
    assert_token_after(lex, token_type::kw_integer, "INTEGER");
    assert_eof(lex);
}

TEST(LexerComments, BlockComment) {
    lexer lex("/* block comment */INTEGER", "test.asn");
    assert_token_after(lex, token_type::kw_integer, "INTEGER");
    assert_eof(lex);
}

TEST(LexerComments, BlockCommentMultiline) {
    lexer lex("/* line 1\n   line 2\n   line 3 */INTEGER", "test.asn");
    assert_token_after(lex, token_type::kw_integer, "INTEGER");
    assert_eof(lex);
}

TEST(LexerComments, BlockCommentWithStars) {
    lexer lex("/*** stars ***/INTEGER", "test.asn");
    assert_token_after(lex, token_type::kw_integer, "INTEGER");
    assert_eof(lex);
}

TEST(LexerComments, SequentialComments) {
    lexer lex("/* first */ /* second */INTEGER", "test.asn");
    assert_token_after(lex, token_type::kw_integer, "INTEGER");
    assert_eof(lex);
}

TEST(LexerComments, OnlyLineComment) {
    lexer lex("-- just a comment", "test.asn");
    assert_eof(lex);
}

TEST(LexerComments, OnlyBlockComment) {
    lexer lex("/* just a comment */", "test.asn");
    assert_eof(lex);
}

// ============================================================================
// 10. Comment interaction — comments between tokens are skipped
// ============================================================================

TEST(LexerCommentInteraction, CommentBetweenTokens) {
    lexer lex("INTEGER -- comment\nBOOLEAN", "test.asn");
    assert_token_after(lex, token_type::kw_integer, "INTEGER");
    assert_token_after(lex, token_type::kw_boolean, "BOOLEAN");
    assert_eof(lex);
}

TEST(LexerCommentInteraction, BlockCommentBetweenTokens) {
    lexer lex("INTEGER /* between */ BOOLEAN", "test.asn");
    assert_token_after(lex, token_type::kw_integer, "INTEGER");
    assert_token_after(lex, token_type::kw_boolean, "BOOLEAN");
    assert_eof(lex);
}

TEST(LexerCommentInteraction, MixedComments) {
    lexer lex("-- line\n/* block */ INTEGER /* block2 */\nBOOLEAN", "test.asn");
    assert_token_after(lex, token_type::kw_integer, "INTEGER");
    assert_token_after(lex, token_type::kw_boolean, "BOOLEAN");
    assert_eof(lex);
}

// ============================================================================
// 11. Error handling tests
// ============================================================================

TEST(LexerErrors, UnterminatedString) {
    diagnostic_engine diag;
    lexer lex("\"unclosed", "err.asn", &diag);
    token t = lex.next_token();
    EXPECT_EQ(t.type, token_type::error);
    EXPECT_TRUE(lex.has_error());
    EXPECT_GT(diag.error_count(), 0);
}

TEST(LexerErrors, InvalidCharacter) {
    diagnostic_engine diag;
    lexer lex("@", "err.asn", &diag);
    token t = lex.next_token();
    EXPECT_EQ(t.type, token_type::error);
    EXPECT_TRUE(lex.has_error());
    EXPECT_GT(diag.error_count(), 0);
}

TEST(LexerErrors, UnterminatedBlockComment) {
    diagnostic_engine diag;
    lexer lex("/* no end", "err.asn", &diag);
    token t = lex.next_token();
    EXPECT_EQ(t.type, token_type::eof);
    EXPECT_TRUE(lex.has_error());
    EXPECT_GT(diag.error_count(), 0);
}

TEST(LexerErrors, BinaryHexNoSuffix) {
    diagnostic_engine diag;
    lexer lex("'0101'", "err.asn", &diag);
    token t = lex.next_token();
    EXPECT_EQ(t.type, token_type::error);
    EXPECT_TRUE(lex.has_error());
}

TEST(LexerErrors, BinaryHexUnterminated) {
    diagnostic_engine diag;
    lexer lex("'0101", "err.asn", &diag);
    token t = lex.next_token();
    EXPECT_EQ(t.type, token_type::error);
    EXPECT_TRUE(lex.has_error());
}

TEST(LexerErrors, LoneDotIsError) {
    diagnostic_engine diag;
    lexer lex(".", "err.asn", &diag);
    token t = lex.next_token();
    EXPECT_EQ(t.type, token_type::error);
    EXPECT_TRUE(lex.has_error());
}

TEST(LexerErrors, HasErrorFalseWhenNoErrors) {
    lexer lex("INTEGER", "ok.asn");
    lex.next_token();
    EXPECT_FALSE(lex.has_error());
}

TEST(LexerErrors, NullDiagnosticDoesNotCrash) {
    lexer lex("\"bad", "");
    token t = lex.next_token();
    EXPECT_EQ(t.type, token_type::error);
    EXPECT_TRUE(lex.has_error());
}

// ============================================================================
// 12. Peek tests
// ============================================================================

TEST(LexerPeek, PeekReturnsNextWithoutAdvancing) {
    lexer lex("INTEGER BOOLEAN", "test.asn");
    token t1 = lex.peek_token();
    EXPECT_EQ(t1.type, token_type::kw_integer);
    token t2 = lex.peek_token();
    EXPECT_EQ(t2.type, token_type::kw_integer);
    token t3 = lex.next_token();
    EXPECT_EQ(t3.type, token_type::kw_integer);
    token t4 = lex.next_token();
    EXPECT_EQ(t4.type, token_type::kw_boolean);
}

TEST(LexerPeek, PeekThenNextSameType) {
    lexer lex("SEQUENCE", "test.asn");
    token peeked = lex.peek_token();
    token actual = lex.next_token();
    EXPECT_EQ(peeked.type, actual.type);
    EXPECT_EQ(peeked.value, actual.value);
}

TEST(LexerPeek, PeekAfterNext) {
    lexer lex("INTEGER BOOLEAN", "test.asn");
    lex.next_token();  // consume INTEGER
    token peeked = lex.peek_token();
    EXPECT_EQ(peeked.type, token_type::kw_boolean);
    token actual = lex.next_token();
    EXPECT_EQ(actual.type, token_type::kw_boolean);
}

TEST(LexerPeek, PeekEof) {
    lexer lex("INTEGER", "test.asn");
    lex.next_token();
    token t = lex.peek_token();
    EXPECT_EQ(t.type, token_type::eof);
    token t2 = lex.next_token();
    EXPECT_EQ(t2.type, token_type::eof);
}

TEST(LexerPeek, MultiplePeeksSameValue) {
    lexer lex("\"hello\"", "test.asn");
    for (int i = 0; i < 5; ++i) {
        token t = lex.peek_token();
        EXPECT_EQ(t.type, token_type::character_string);
        EXPECT_EQ(t.value, "hello");
    }
}

// ============================================================================
// 13. Source location tests
// ============================================================================

TEST(LexerLocations, Line1Column1) {
    lexer lex("INTEGER", "test.asn");
    token t = lex.next_token();
    EXPECT_EQ(t.loc.line, 1);
    EXPECT_EQ(t.loc.column, 1);
    EXPECT_EQ(t.loc.file, "test.asn");
}

TEST(LexerLocations, SecondToken) {
    lexer lex("INTEGER BOOLEAN", "test.asn");
    lex.next_token();
    token t = lex.next_token();
    EXPECT_EQ(t.loc.line, 1);
    EXPECT_GT(t.loc.column, 1);
    EXPECT_EQ(t.loc.file, "test.asn");
}

TEST(LexerLocations, NewlineTracking) {
    lexer lex("INTEGER\nBOOLEAN", "test.asn");
    lex.next_token();
    token t = lex.next_token();
    EXPECT_EQ(t.loc.line, 2);
    EXPECT_EQ(t.loc.column, 1);
}

TEST(LexerLocations, MultipleLines) {
    lexer lex("A\nB\nC", "multi.asn");
    token t1 = lex.next_token();
    EXPECT_EQ(t1.loc.line, 1);
    token t2 = lex.next_token();
    EXPECT_EQ(t2.loc.line, 2);
    token t3 = lex.next_token();
    EXPECT_EQ(t3.loc.line, 3);
}

TEST(LexerLocations, CurrentLocationAtStart) {
    lexer lex("INTEGER", "test.asn");
    source_location loc = lex.current_location();
    EXPECT_EQ(loc.line, 1);
    EXPECT_EQ(loc.column, 1);
}

TEST(LexerLocations, EmptyFilename) {
    lexer lex("INTEGER", "");
    token t = lex.next_token();
    EXPECT_TRUE(t.loc.file.empty());
    EXPECT_EQ(t.loc.line, 1);
}

// ============================================================================
// 14. EOF tests
// ============================================================================

TEST(LexerEOF, EofAtEnd) {
    lexer lex("INTEGER", "test.asn");
    lex.next_token();
    assert_eof(lex);
}

TEST(LexerEOF, EmptyInput) {
    lexer lex("", "test.asn");
    token t = lex.next_token();
    EXPECT_EQ(t.type, token_type::eof);
    EXPECT_EQ(t.value, "");
}

TEST(LexerEOF, EofAfterWhitespaceOnly) {
    lexer lex("   \n  \t  \n", "test.asn");
    token t = lex.next_token();
    EXPECT_EQ(t.type, token_type::eof);
}

TEST(LexerEOF, RepeatedEof) {
    lexer lex("", "test.asn");
    EXPECT_EQ(lex.next_token().type, token_type::eof);
    EXPECT_EQ(lex.next_token().type, token_type::eof);
    EXPECT_EQ(lex.next_token().type, token_type::eof);
}

TEST(LexerEOF, EofAfterComment) {
    lexer lex("-- comment\n", "test.asn");
    token t = lex.next_token();
    EXPECT_EQ(t.type, token_type::eof);
}

// ============================================================================
// 15. Integration / real ASN.1 snippets
// ============================================================================

TEST(LexerIntegration, SimpleTypeAssignment) {
    lexer lex("MyInt ::= INTEGER", "test.asn");
    assert_token_after(lex, token_type::identifier, "MyInt");
    assert_token_after(lex, token_type::assignment, "::=");
    assert_token_after(lex, token_type::kw_integer, "INTEGER");
    assert_eof(lex);
}

TEST(LexerIntegration, ModuleHeader) {
    lexer lex("MyModule DEFINITIONS AUTOMATIC TAGS ::= BEGIN", "test.asn");
    assert_token_after(lex, token_type::identifier, "MyModule");
    assert_token_after(lex, token_type::kw_definitions, "DEFINITIONS");
    assert_token_after(lex, token_type::kw_automatic, "AUTOMATIC");
    assert_token_after(lex, token_type::identifier, "TAGS");
    assert_token_after(lex, token_type::assignment, "::=");
    assert_token_after(lex, token_type::kw_begin, "BEGIN");
    assert_eof(lex);
}

TEST(LexerIntegration, SequenceType) {
    lexer lex("SEQUENCE { field INTEGER, opt BOOLEAN OPTIONAL }", "test.asn");
    assert_token_after(lex, token_type::kw_sequence, "SEQUENCE");
    assert_token_after(lex, token_type::left_brace, "{");
    assert_token_after(lex, token_type::identifier, "field");
    assert_token_after(lex, token_type::kw_integer, "INTEGER");
    assert_token_after(lex, token_type::comma, ",");
    assert_token_after(lex, token_type::identifier, "opt");
    assert_token_after(lex, token_type::kw_boolean, "BOOLEAN");
    assert_token_after(lex, token_type::kw_optional, "OPTIONAL");
    assert_token_after(lex, token_type::right_brace, "}");
    assert_eof(lex);
}

TEST(LexerIntegration, EnumeratedType) {
    lexer lex("ENUMERATED { red(0), green(1), blue(2) }", "test.asn");
    assert_token_after(lex, token_type::kw_enumerated, "ENUMERATED");
    assert_token_after(lex, token_type::left_brace, "{");
    assert_token_after(lex, token_type::identifier, "red");
    assert_token_after(lex, token_type::left_paren, "(");
    assert_token_after(lex, token_type::number, "0");
    assert_token_after(lex, token_type::right_paren, ")");
    assert_token_after(lex, token_type::comma, ",");
    assert_token_after(lex, token_type::identifier, "green");
    assert_token_after(lex, token_type::left_paren, "(");
    assert_token_after(lex, token_type::number, "1");
    assert_token_after(lex, token_type::right_paren, ")");
    assert_token_after(lex, token_type::comma, ",");
    assert_token_after(lex, token_type::identifier, "blue");
    assert_token_after(lex, token_type::left_paren, "(");
    assert_token_after(lex, token_type::number, "2");
    assert_token_after(lex, token_type::right_paren, ")");
    assert_token_after(lex, token_type::right_brace, "}");
    assert_eof(lex);
}

TEST(LexerIntegration, ConstrainedType) {
    lexer lex("INTEGER (0..255)", "test.asn");
    assert_token_after(lex, token_type::kw_integer, "INTEGER");
    assert_token_after(lex, token_type::left_paren, "(");
    assert_token_after(lex, token_type::number, "0");
    assert_token_after(lex, token_type::range, "..");
    assert_token_after(lex, token_type::number, "255");
    assert_token_after(lex, token_type::right_paren, ")");
    assert_eof(lex);
}

TEST(LexerIntegration, ChoiceType) {
    lexer lex("CHOICE { a INTEGER, b BOOLEAN }", "test.asn");
    assert_token_after(lex, token_type::kw_choice, "CHOICE");
    assert_token_after(lex, token_type::left_brace, "{");
    assert_token_after(lex, token_type::identifier, "a");
    assert_token_after(lex, token_type::kw_integer, "INTEGER");
    assert_token_after(lex, token_type::comma, ",");
    assert_token_after(lex, token_type::identifier, "b");
    assert_token_after(lex, token_type::kw_boolean, "BOOLEAN");
    assert_token_after(lex, token_type::right_brace, "}");
    assert_eof(lex);
}

TEST(LexerIntegration, WithComments) {
    lexer lex("-- Header comment\nMyType ::= INTEGER -- inline\nEND", "test.asn");
    assert_token_after(lex, token_type::identifier, "MyType");
    assert_token_after(lex, token_type::assignment, "::=");
    assert_token_after(lex, token_type::kw_integer, "INTEGER");
    assert_token_after(lex, token_type::kw_end, "END");
    assert_eof(lex);
}

TEST(LexerIntegration, ImportsClause) {
    lexer lex("IMPORTS MyType, OtherType FROM OtherModule;", "test.asn");
    assert_token_after(lex, token_type::kw_imports, "IMPORTS");
    assert_token_after(lex, token_type::identifier, "MyType");
    assert_token_after(lex, token_type::comma, ",");
    assert_token_after(lex, token_type::identifier, "OtherType");
    assert_token_after(lex, token_type::kw_from, "FROM");
    assert_token_after(lex, token_type::identifier, "OtherModule");
    assert_token_after(lex, token_type::semicolon, ";");
    assert_eof(lex);
}

TEST(LexerIntegration, MultipleNewlinesBetweenTokens) {
    lexer lex("INTEGER\n\n\nBOOLEAN", "test.asn");
    assert_token_after(lex, token_type::kw_integer, "INTEGER");
    assert_token_after(lex, token_type::kw_boolean, "BOOLEAN");
    assert_eof(lex);
}

TEST(LexerIntegration, TaggedSyntax) {
    lexer lex("[APPLICATION 1] IMPLICIT INTEGER", "test.asn");
    assert_token_after(lex, token_type::left_bracket, "[");
    assert_token_after(lex, token_type::kw_application, "APPLICATION");
    assert_token_after(lex, token_type::number, "1");
    assert_token_after(lex, token_type::right_bracket, "]");
    assert_token_after(lex, token_type::kw_implicit, "IMPLICIT");
    assert_token_after(lex, token_type::kw_integer, "INTEGER");
    assert_eof(lex);
}

}  // namespace
