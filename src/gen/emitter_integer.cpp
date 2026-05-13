#include "emitter_integer.hpp"

#include <sstream>

namespace asn1pp::gen {

inline std::string code_emitter::emit_header_guard(std::string_view name) {
    std::ostringstream oss;
    for (char c : name) {
        if (c >= 'A' && c <= 'Z') oss << '_' << c;
        else if (c >= 'a' && c <= 'z') oss << c;
        else if (c >= '0' && c <= '9') oss << c;
        else oss << '_';
    }
    std::string result = oss.str();
    std::transform(result.begin(), result.end(), result.begin(),
                   ::toupper);
    return "ASN1PP_" + result + "_HPP";
}

inline std::string code_emitter::emit_includes(const std::vector<std::string>& includes) {
    std::ostringstream oss;
    for (const auto& inc : includes) {
        if (inc.front() == '<') oss << "#include " << inc << "\n";
        else oss << "#include \"" << inc << "\"\n";
    }
    return oss.str();
}

inline std::string code_emitter::emit_namespace_open(const std::string& ns) {
    if (ns.empty()) return {};
    return "namespace " + ns + " {\n";
}

inline std::string code_emitter::emit_namespace_close(const std::string& ns) {
    if (ns.empty()) return {};
    return "}  // namespace " + ns + "\n";
}

static bool has_value_range_constraint(const std::vector<constraint>& constraints) {
    for (const auto& c : constraints) {
        if (std::holds_alternative<value_range_constraint>(c.content)) {
            return true;
        }
    }
    return false;
}

static const value_range_constraint* get_value_range_constraint(
    const std::vector<constraint>& constraints) {
    for (const auto& c : constraints) {
        if (std::holds_alternative<value_range_constraint>(c.content)) {
            return &std::get<value_range_constraint>(c.content);
        }
    }
    return nullptr;
}

std::string emit_integer_type(const integer_type&,
                              const std::string& name,
                              const std::vector<constraint>& constraints,
                              const emitter_options& opts) {
    std::ostringstream oss;

    oss << "struct " << name << " {\n";
    oss << "    int64_t value{};\n";

    bool has_range = has_value_range_constraint(constraints);

    if (opts.include_validate && has_range) {
        oss << "\n    result<void> validate() const {\n";

        const auto* vrc = get_value_range_constraint(constraints);

        if (vrc) {
            if (vrc->min_value.has_value()) {
                if (vrc->min_inclusive) {
                    oss << "        if (value < " << *vrc->min_value << ") return result<void>::err(error_code::constraint_violation);\n";
                } else {
                    oss << "        if (value <= " << *vrc->min_value << ") return result<void>::err(error_code::constraint_violation);\n";
                }
            }
            if (vrc->max_value.has_value()) {
                if (vrc->max_inclusive) {
                    oss << "        if (value > " << *vrc->max_value << ") return result<void>::err(error_code::constraint_violation);\n";
                } else {
                    oss << "        if (value >= " << *vrc->max_value << ") return result<void>::err(error_code::constraint_violation);\n";
                }
            }
        }

        oss << "        return result<void>::ok();\n";
        oss << "    }\n";
    }

    if (opts.include_operators) {
        oss << "\n    bool operator==(const " << name << "&) const = default;\n";
        oss << "    bool operator!=(const " << name << "&) const = default;\n";
    }

    oss << "};\n";

    if (!opts.namespace_name.empty()) {
        oss << "template<> struct asn1_tag<" << opts.namespace_name << "::" << name
            << "> { static constexpr auto value = universal_tag::integer; };\n";
    } else {
        oss << "template<> struct asn1_tag<" << name
            << "> { static constexpr auto value = universal_tag::integer; };\n";
    }

    return oss.str();
}

}  // namespace asn1pp::gen
