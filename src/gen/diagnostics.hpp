#pragma once

#include "ast.hpp"  // for source_location

#include <string>
#include <string_view>
#include <vector>

namespace asn1pp::gen {

enum class severity { error, warning, note };

struct diagnostic {
    severity sev;
    source_location loc;
    std::string message;
};

class diagnostic_engine {
public:
    void add_error(source_location loc, std::string message);
    void add_warning(source_location loc, std::string message);
    void add_note(source_location loc, std::string message);

    [[nodiscard]] bool has_errors() const noexcept;
    [[nodiscard]] size_t error_count() const noexcept;
    [[nodiscard]] size_t warning_count() const noexcept;
    [[nodiscard]] size_t note_count() const noexcept;
    [[nodiscard]] size_t total_count() const noexcept;

    [[nodiscard]] std::string format(const diagnostic& diag) const;
    [[nodiscard]] std::string format_all() const;
    [[nodiscard]] std::string format_line_with_caret(
        std::string_view source_line, size_t column) const;

    void clear() noexcept;

    [[nodiscard]] const diagnostic* begin() const noexcept;
    [[nodiscard]] const diagnostic* end() const noexcept;

private:
    std::vector<diagnostic> diagnostics_;
};

inline void diagnostic_engine::add_error(source_location loc, std::string message) {
    diagnostics_.push_back(diagnostic{severity::error, loc, std::move(message)});
}

inline void diagnostic_engine::add_warning(source_location loc, std::string message) {
    diagnostics_.push_back(diagnostic{severity::warning, loc, std::move(message)});
}

inline void diagnostic_engine::add_note(source_location loc, std::string message) {
    diagnostics_.push_back(diagnostic{severity::note, loc, std::move(message)});
}

inline bool diagnostic_engine::has_errors() const noexcept {
    for (const auto& d : diagnostics_) {
        if (d.sev == severity::error) { return true; }
    }
    return false;
}

inline size_t diagnostic_engine::error_count() const noexcept {
    size_t count = 0;
    for (const auto& d : diagnostics_) {
        if (d.sev == severity::error) { ++count; }
    }
    return count;
}

inline size_t diagnostic_engine::warning_count() const noexcept {
    size_t count = 0;
    for (const auto& d : diagnostics_) {
        if (d.sev == severity::warning) { ++count; }
    }
    return count;
}

inline size_t diagnostic_engine::note_count() const noexcept {
    size_t count = 0;
    for (const auto& d : diagnostics_) {
        if (d.sev == severity::note) { ++count; }
    }
    return count;
}

inline size_t diagnostic_engine::total_count() const noexcept {
    return diagnostics_.size();
}

inline std::string diagnostic_engine::format(const diagnostic& diag) const {
    const char* sev_str = "";
    switch (diag.sev) {
        case severity::error:   sev_str = "error"; break;
        case severity::warning: sev_str = "warning"; break;
        case severity::note:    sev_str = "note"; break;
    }
    std::string result;
    result.reserve(diag.loc.file.size() + diag.message.size() + 32);
    result.append(diag.loc.file);
    result.push_back(':');
    result.append(std::to_string(diag.loc.line));
    result.push_back(':');
    result.append(std::to_string(diag.loc.column));
    result.append(": ");
    result.append(sev_str);
    result.append(": ");
    result.append(diag.message);
    return result;
}

inline std::string diagnostic_engine::format_all() const {
    if (diagnostics_.empty()) { return {}; }
    std::string result;
    for (const auto& d : diagnostics_) {
        result.append(format(d));
        result.push_back('\n');
    }
    return result;
}

inline std::string diagnostic_engine::format_line_with_caret(
    std::string_view source_line, size_t column) const {
    std::string result;
    result.append(source_line);
    if (!result.empty() && result.back() != '\n') {
        result.push_back('\n');
    }
    if (column > 0 && column <= source_line.size() + 1) {
        result.append(column - 1, ' ');
        result.push_back('^');
        result.push_back('\n');
    }
    return result;
}

inline void diagnostic_engine::clear() noexcept {
    diagnostics_.clear();
}

inline const diagnostic* diagnostic_engine::begin() const noexcept {
    return diagnostics_.data();
}

inline const diagnostic* diagnostic_engine::end() const noexcept {
    return diagnostics_.data() + diagnostics_.size();
}

}  // namespace asn1pp::gen
