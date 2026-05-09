#include <gtest/gtest.h>

#include "../../src/gen/diagnostics.hpp"  // Does not exist yet — RED phase

#include <string>
#include <string_view>

namespace {

using namespace asn1pp::gen;

// ============================================================================
// source_location tests (reused from ast.hpp)
// ============================================================================

TEST(SourceLocation, DefaultConstruction) {
    source_location loc;
    EXPECT_TRUE(loc.file.empty());
    EXPECT_EQ(loc.line, 0);
    EXPECT_EQ(loc.column, 0);
}

TEST(SourceLocation, FieldAssignment) {
    source_location loc;
    loc.file = "test.asn";
    loc.line = 42;
    loc.column = 7;
    EXPECT_EQ(loc.file, "test.asn");
    EXPECT_EQ(loc.line, 42);
    EXPECT_EQ(loc.column, 7);
}

TEST(SourceLocation, Equality) {
    source_location a{"test.asn", 10, 5};
    source_location b{"test.asn", 10, 5};
    source_location c{"other.asn", 10, 5};
    source_location d{"test.asn", 20, 5};
    source_location e{"test.asn", 10, 8};

    EXPECT_EQ(a.file, b.file);
    EXPECT_EQ(a.line, b.line);
    EXPECT_EQ(a.column, b.column);

    EXPECT_NE(a.file, c.file);
    EXPECT_NE(a.line, d.line);
    EXPECT_NE(a.column, e.column);
}

// ============================================================================
// severity enum tests
// ============================================================================

TEST(Severity, EnumValuesDistinct) {
    EXPECT_NE(static_cast<int>(severity::error), static_cast<int>(severity::warning));
    EXPECT_NE(static_cast<int>(severity::error), static_cast<int>(severity::note));
    EXPECT_NE(static_cast<int>(severity::warning), static_cast<int>(severity::note));
}

TEST(Severity, ErrorIsHighestPriority) {
    // error should come before warning (lower value = higher priority)
    EXPECT_LT(static_cast<int>(severity::error), static_cast<int>(severity::warning));
}

// ============================================================================
// diagnostic struct tests
// ============================================================================

TEST(Diagnostic, Construction) {
    source_location loc{"test.asn", 10, 5};
    diagnostic diag{severity::error, loc, "unexpected token '}'"};

    EXPECT_EQ(diag.sev, severity::error);
    EXPECT_EQ(diag.loc.file, "test.asn");
    EXPECT_EQ(diag.loc.line, 10);
    EXPECT_EQ(diag.loc.column, 5);
    EXPECT_EQ(diag.message, "unexpected token '}'");
}

TEST(Diagnostic, WarningConstruction) {
    source_location loc{"schema.asn", 3, 1};
    diagnostic diag{severity::warning, loc, "unused import"};

    EXPECT_EQ(diag.sev, severity::warning);
    EXPECT_EQ(diag.message, "unused import");
}

TEST(Diagnostic, NoteConstruction) {
    source_location loc{"types.asn", 7, 12};
    diagnostic diag{severity::note, loc, "type defined here"};

    EXPECT_EQ(diag.sev, severity::note);
    EXPECT_EQ(diag.message, "type defined here");
}

// ============================================================================
// diagnostic_engine: add diagnostic tests
// ============================================================================

class DiagnosticEngineTest : public ::testing::Test {
protected:
    diagnostic_engine engine;
};

TEST_F(DiagnosticEngineTest, StartsEmpty) {
    EXPECT_FALSE(engine.has_errors());
    EXPECT_EQ(engine.error_count(), 0);
    EXPECT_EQ(engine.warning_count(), 0);
    EXPECT_EQ(engine.note_count(), 0);
    EXPECT_EQ(engine.total_count(), 0);
}

TEST_F(DiagnosticEngineTest, AddError) {
    engine.add_error({"test.asn", 1, 1}, "syntax error");
    EXPECT_TRUE(engine.has_errors());
    EXPECT_EQ(engine.error_count(), 1);
    EXPECT_EQ(engine.warning_count(), 0);
    EXPECT_EQ(engine.total_count(), 1);
}

TEST_F(DiagnosticEngineTest, AddWarning) {
    engine.add_warning({"test.asn", 2, 3}, "unused variable");
    EXPECT_FALSE(engine.has_errors());
    EXPECT_EQ(engine.warning_count(), 1);
    EXPECT_EQ(engine.error_count(), 0);
    EXPECT_EQ(engine.total_count(), 1);
}

TEST_F(DiagnosticEngineTest, AddNote) {
    engine.add_note({"test.asn", 3, 5}, "see definition here");
    EXPECT_FALSE(engine.has_errors());
    EXPECT_EQ(engine.note_count(), 1);
    EXPECT_EQ(engine.error_count(), 0);
    EXPECT_EQ(engine.warning_count(), 0);
    EXPECT_EQ(engine.total_count(), 1);
}

TEST_F(DiagnosticEngineTest, HasErrorsTrueAfterError) {
    engine.add_warning({"test.asn", 1, 1}, "warning only");
    EXPECT_FALSE(engine.has_errors());

    engine.add_error({"test.asn", 2, 1}, "now an error");
    EXPECT_TRUE(engine.has_errors());
}

TEST_F(DiagnosticEngineTest, HasErrorsFalseAfterOnlyWarnings) {
    engine.add_warning({"test.asn", 1, 1}, "warning 1");
    engine.add_warning({"test.asn", 2, 1}, "warning 2");
    engine.add_note({"test.asn", 3, 1}, "note 1");
    EXPECT_FALSE(engine.has_errors());
}

TEST_F(DiagnosticEngineTest, ErrorCountIncrements) {
    engine.add_error({"a.asn", 1, 1}, "e1");
    engine.add_error({"a.asn", 2, 1}, "e2");
    engine.add_error({"a.asn", 3, 1}, "e3");
    EXPECT_EQ(engine.error_count(), 3);
    EXPECT_EQ(engine.total_count(), 3);
}

TEST_F(DiagnosticEngineTest, WarningCountIncrements) {
    engine.add_warning({"a.asn", 1, 1}, "w1");
    engine.add_warning({"a.asn", 2, 1}, "w2");
    EXPECT_EQ(engine.warning_count(), 2);
}

TEST_F(DiagnosticEngineTest, MixedCounts) {
    engine.add_error({"a.asn", 1, 1}, "e1");
    engine.add_warning({"a.asn", 2, 1}, "w1");
    engine.add_error({"a.asn", 3, 1}, "e2");
    engine.add_warning({"a.asn", 4, 1}, "w2");
    engine.add_note({"a.asn", 5, 1}, "n1");

    EXPECT_EQ(engine.error_count(), 2);
    EXPECT_EQ(engine.warning_count(), 2);
    EXPECT_EQ(engine.note_count(), 1);
    EXPECT_EQ(engine.total_count(), 5);
}

// ============================================================================
// diagnostic_engine: format tests
// ============================================================================

TEST_F(DiagnosticEngineTest, FormatErrorGccStyle) {
    source_location loc{"test.asn", 10, 5};
    diagnostic diag{severity::error, loc, "unexpected token '}'"};

    std::string formatted = engine.format(diag);
    // GCC-style: file:line:col: severity: message
    EXPECT_EQ(formatted, "test.asn:10:5: error: unexpected token '}'");
}

TEST_F(DiagnosticEngineTest, FormatWarningGccStyle) {
    source_location loc{"schema.asn", 3, 1};
    diagnostic diag{severity::warning, loc, "unused import"};

    std::string formatted = engine.format(diag);
    EXPECT_EQ(formatted, "schema.asn:3:1: warning: unused import");
}

TEST_F(DiagnosticEngineTest, FormatNoteGccStyle) {
    source_location loc{"types.asn", 7, 12};
    diagnostic diag{severity::note, loc, "type defined here"};

    std::string formatted = engine.format(diag);
    EXPECT_EQ(formatted, "types.asn:7:12: note: type defined here");
}

TEST_F(DiagnosticEngineTest, FormatAllMultipleDiagnostics) {
    engine.add_error({"test.asn", 10, 5}, "unexpected token '}'");
    engine.add_warning({"test.asn", 3, 1}, "unused import");
    engine.add_note({"test.asn", 7, 12}, "type defined here");

    std::string all = engine.format_all();
    EXPECT_TRUE(all.find("test.asn:10:5: error: unexpected token '}'") != std::string::npos);
    EXPECT_TRUE(all.find("test.asn:3:1: warning: unused import") != std::string::npos);
    EXPECT_TRUE(all.find("test.asn:7:12: note: type defined here") != std::string::npos);
}

TEST_F(DiagnosticEngineTest, FormatAllEmpty) {
    std::string all = engine.format_all();
    EXPECT_TRUE(all.empty());
}

// ============================================================================
// diagnostic_engine: format_line_with_caret tests
// ============================================================================

TEST_F(DiagnosticEngineTest, FormatLineWithCaret_Basic) {
    std::string_view source_line = "   INTEGER ::= 42 }";
    size_t column = 17;  // Point at the '}' at position 17 (1-indexed)

    std::string result = engine.format_line_with_caret(source_line, column);
    // Should print the line and a caret under the column
    EXPECT_TRUE(result.find(source_line) != std::string::npos);
    EXPECT_TRUE(result.find('^') != std::string::npos);
}

TEST_F(DiagnosticEngineTest, FormatLineWithCaret_FirstColumn) {
    std::string_view source_line = "MODULE-BEGIN";
    size_t column = 1;

    std::string result = engine.format_line_with_caret(source_line, column);
    EXPECT_TRUE(result.find(source_line) != std::string::npos);
    // Caret should be under the first character
    EXPECT_TRUE(result.find("^\n") != std::string::npos || result.find("^") == result.find('\n') + 1);
}

TEST_F(DiagnosticEngineTest, FormatLineWithCaret_ZeroColumn) {
    std::string_view source_line = "hello";
    // column 0 means no caret position
    std::string result = engine.format_line_with_caret(source_line, 0);
    // Should still return the line, but no caret
    EXPECT_TRUE(result.find(source_line) != std::string::npos);
}

TEST_F(DiagnosticEngineTest, FormatLineWithCaret_HandlesTabs) {
    std::string_view source_line = "\t\tvalue = 42";
    size_t column = 5;  // Column 5 with 2 tabs before it

    std::string result = engine.format_line_with_caret(source_line, column);
    EXPECT_TRUE(result.find(source_line) != std::string::npos);
}

// ============================================================================
// diagnostic_engine: clear tests
// ============================================================================

TEST_F(DiagnosticEngineTest, ClearResetsEverything) {
    engine.add_error({"a.asn", 1, 1}, "e1");
    engine.add_warning({"a.asn", 2, 1}, "w1");
    engine.add_note({"a.asn", 3, 1}, "n1");

    EXPECT_EQ(engine.total_count(), 3);

    engine.clear();

    EXPECT_FALSE(engine.has_errors());
    EXPECT_EQ(engine.error_count(), 0);
    EXPECT_EQ(engine.warning_count(), 0);
    EXPECT_EQ(engine.note_count(), 0);
    EXPECT_EQ(engine.total_count(), 0);
}

TEST_F(DiagnosticEngineTest, ClearThenAddAgain) {
    engine.add_error({"a.asn", 1, 1}, "e1");
    engine.clear();
    engine.add_warning({"b.asn", 2, 1}, "w1");

    EXPECT_FALSE(engine.has_errors());
    EXPECT_EQ(engine.error_count(), 0);
    EXPECT_EQ(engine.warning_count(), 1);
    EXPECT_EQ(engine.total_count(), 1);
}

// ============================================================================
// diagnostic_engine: iteration tests
// ============================================================================

TEST_F(DiagnosticEngineTest, IterateEmpty) {
    const diagnostic* b = engine.begin();
    const diagnostic* e = engine.end();
    EXPECT_EQ(b, e);
}

TEST_F(DiagnosticEngineTest, IterateOverDiagnostics) {
    engine.add_error({"a.asn", 1, 1}, "e1");
    engine.add_warning({"a.asn", 2, 1}, "w1");

    size_t count = 0;
    for (const diagnostic* it = engine.begin(); it != engine.end(); ++it) {
        count++;
    }
    EXPECT_EQ(count, 2);
}

// ============================================================================
// diagnostic_engine: accumulation tests
// ============================================================================

TEST_F(DiagnosticEngineTest, AccumulateManyDiagnostics) {
    for (int i = 0; i < 100; i++) {
        engine.add_error({"big.asn", static_cast<size_t>(i + 1), 1}, "error " + std::to_string(i));
    }
    EXPECT_EQ(engine.error_count(), 100);
    EXPECT_EQ(engine.total_count(), 100);
    EXPECT_TRUE(engine.has_errors());
}

TEST_F(DiagnosticEngineTest, DiagnosticsPreservedInOrder) {
    engine.add_error({"a.asn", 1, 1}, "first");
    engine.add_warning({"a.asn", 2, 1}, "second");
    engine.add_note({"a.asn", 3, 1}, "third");

    const diagnostic* it = engine.begin();
    EXPECT_EQ(it->message, "first");
    EXPECT_EQ(it->sev, severity::error);
    ++it;
    EXPECT_EQ(it->message, "second");
    EXPECT_EQ(it->sev, severity::warning);
    ++it;
    EXPECT_EQ(it->message, "third");
    EXPECT_EQ(it->sev, severity::note);
    ++it;
    EXPECT_EQ(it, engine.end());
}

}  // namespace
