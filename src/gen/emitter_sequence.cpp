#include "emitter_sequence.hpp"
#include "emitter_enumerated.hpp"

#include <cctype>
#include <optional>
#include <sstream>
#include <variant>

#include "../../libs/codec/traits.hpp"

namespace asn1pp::gen {

std::string emit_sequence(std::string_view, const sequence_type&, tag_default);
std::string emit_set(std::string_view, const set_type&, tag_default);

namespace {

struct nested_definition {
    std::string code;
};

std::string sanitize_name(const std::string& name) {
    std::string result;
    for (char ch : name) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') || ch == '_') {
            result += ch;
        } else {
            result += '_';
        }
    }
    return result;
}

std::string capitalize(const std::string& name) {
    if (name.empty()) return name;
    std::string result = name;
    result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
    return result;
}

std::string cpp_type_for_field(const type_ref& type, const std::string& field_name,
                                std::vector<nested_definition>& nested) {
    return std::visit(
        [&](const auto& alt) -> std::string {
            using T = std::decay_t<decltype(alt)>;

            if constexpr (std::is_same_v<T, integer_type>) {
                return "int64_t";
            }
            if constexpr (std::is_same_v<T, boolean_type>) {
                return "bool";
            }
            if constexpr (std::is_same_v<T, null_type>) {
                return "std::monostate";
            }
            if constexpr (std::is_same_v<T, real_type>) {
                return "double";
            }
            if constexpr (std::is_same_v<T, octet_string_type>) {
                return "std::vector<uint8_t>";
            }
            if constexpr (std::is_same_v<T, object_identifier_type>) {
                return "std::vector<uint32_t>";
            }
            if constexpr (std::is_same_v<T, relative_oid_type>) {
                return "std::vector<uint32_t>";
            }
            if constexpr (std::is_same_v<T, any_type>) {
                return "std::vector<uint8_t>";
            }
            if constexpr (std::is_same_v<T, std::string>) {
                return alt;
            }
            if constexpr (std::is_same_v<T, enumerated_type>) {
                std::string nested_name = capitalize(field_name) + "_enum";
                nested.push_back({emit_enumerated(nested_name, alt)});
                return nested_name;
            }
            if constexpr (std::is_same_v<T, bit_string_type>) {
                return "std::pair<std::vector<uint8_t>, size_t>";
            }
            if constexpr (std::is_same_v<T, std::unique_ptr<sequence_type>>) {
                std::string nested_name = capitalize(field_name);
                std::ostringstream ns;
                ns << emit_sequence(nested_name, *alt, tag_default::automatic_tag);
                nested.push_back({"struct " + nested_name + " {\n" + ns.str() + "};\n"});
                return nested_name;
            }
            if constexpr (std::is_same_v<T, std::unique_ptr<set_type>>) {
                std::string nested_name = capitalize(field_name);
                std::ostringstream ns;
                ns << emit_set(nested_name, *alt, tag_default::automatic_tag);
                nested.push_back({"struct " + nested_name + " {\n" + ns.str() + "};\n"});
                return nested_name;
            }
            if constexpr (std::is_same_v<T, std::unique_ptr<choice_type>>) {
                std::ostringstream vs;
                vs << "std::variant<";
                bool first = true;
                for (const auto& alt_c : alt->alternatives) {
                    if (!first) vs << ", ";
                    first = false;
                    std::vector<nested_definition> dummy;
                    vs << cpp_type_for_field(*alt_c.type, alt_c.name, dummy);
                }
                vs << ">";
                return vs.str();
            }
            if constexpr (std::is_same_v<T, std::unique_ptr<sequence_of_type>>) {
                std::vector<nested_definition> dummy;
                std::string elem = cpp_type_for_field(*alt->element_type, field_name + "_elem", dummy);
                return "std::vector<" + elem + ">";
            }
            if constexpr (std::is_same_v<T, std::unique_ptr<set_of_type>>) {
                std::vector<nested_definition> dummy;
                std::string elem = cpp_type_for_field(*alt->element_type, field_name + "_elem", dummy);
                return "std::vector<" + elem + ">";
            }
            if constexpr (std::is_same_v<T, std::unique_ptr<tagged_type>>) {
                std::vector<nested_definition> dummy;
                return cpp_type_for_field(*alt->underlying_type, field_name, dummy);
            }
            if constexpr (std::is_same_v<T, std::unique_ptr<constrained_type>>) {
                std::vector<nested_definition> dummy;
                return cpp_type_for_field(*alt->underlying_type, field_name, dummy);
            }
            if constexpr (std::is_same_v<T, std::unique_ptr<type_instantiation>>) {
                return alt->type_name;
            }
            return "void";
        },
        type.content);
}

std::string default_value_for(const std::string& cpp_type, const std::string& asn1_default) {
    if (cpp_type == "int64_t") {
        return asn1_default;
    }
    if (cpp_type == "bool") {
        return (asn1_default == "TRUE" || asn1_default == "true") ? "true" : "false";
    }
    if (cpp_type == "double") {
        return asn1_default;
    }
    if (cpp_type == "std::monostate") {
        return "{}";
    }
    if (cpp_type.find("std::vector") == 0) {
        return "{" + asn1_default + "}";
    }
    return "{" + asn1_default + "}";
}

struct field_info {
    std::string cpp_type;
    std::string field_name;
    bool optional;
    std::optional<std::string> default_value;
};

template<typename ComponentContainer>
std::string emit_generic(std::string_view type_name, const ComponentContainer& components,
                          bool has_extension, universal_tag tag_kind, tag_default) {
    std::vector<nested_definition> nested_defs;
    std::vector<field_info> fields;

    for (const auto& comp : components) {
        std::string cpp_type = cpp_type_for_field(*comp.type, comp.name, nested_defs);
        std::string fname = sanitize_name(comp.name);
        fields.push_back({cpp_type, fname, comp.optional, comp.default_value});
    }

    std::ostringstream result;

    for (const auto& nested : nested_defs) {
        result << nested.code;
    }

    result << "struct " << type_name << " {\n";

    if (has_extension) {
        result << "    std::vector<uint8_t> _extension_data;\n";
    }

    for (const auto& f : fields) {
        if (f.optional) {
            result << "    std::optional<" << f.cpp_type << "> " << f.field_name << ";\n";
        } else if (f.default_value.has_value()) {
            std::string dv = default_value_for(f.cpp_type, *f.default_value);
            result << "    " << f.cpp_type << " " << f.field_name << "{" << dv << "};\n";
        } else {
            bool needs_brace = (f.cpp_type == "int64_t" || f.cpp_type == "bool" ||
                                f.cpp_type == "double" || f.cpp_type.find("std::vector") == 0 ||
                                f.cpp_type.find("std::variant") == 0 || f.cpp_type.find("std::pair") == 0);
            if (needs_brace) {
                result << "    " << f.cpp_type << " " << f.field_name << "{};\n";
            } else {
                result << "    " << f.cpp_type << " " << f.field_name << ";\n";
            }
        }
    }

    result << "\n    bool operator==(const " << type_name << "&) const = default;\n";
    result << "};\n\n";

    const char* tag_str = (tag_kind == universal_tag::sequence) ? "sequence" : "set";
    result << "template<> struct asn1pp::asn1_tag<" << type_name
           << "> { static constexpr auto value = "
           << "asn1pp::make_universal(asn1pp::universal_tag::"
           << tag_str << ", true); };\n";

    return result.str();
}

}  // namespace

std::string emit_sequence(std::string_view type_name, const sequence_type& seq, tag_default tagging) {
    return emit_generic(type_name, seq.components, seq.has_extension, universal_tag::sequence, tagging);
}

std::string emit_set(std::string_view type_name, const set_type& s, tag_default tagging) {
    return emit_generic(type_name, s.components, s.has_extension, universal_tag::set, tagging);
}

}  // namespace asn1pp::gen
