#include "emitter_sequence_of.hpp"

#include <sstream>
#include <variant>

#include "../../libs/codec/traits.hpp"

namespace asn1pp::gen {

namespace {

std::string cpp_element_type(const type_ref& type) {
    return std::visit(
        [](const auto& alt) -> std::string {
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
            if constexpr (std::is_same_v<T, std::string>) {
                return alt;
            }
            return "void";
        },
        type.content);
}

bool has_size_constraint(const std::vector<constraint>& constraints) {
    for (const auto& c : constraints) {
        if (std::holds_alternative<size_constraint>(c.content)) {
            return true;
        }
    }
    return false;
}

std::string emit_validate_method(const std::vector<constraint>& constraints) {
    for (const auto& c : constraints) {
        if (std::holds_alternative<size_constraint>(c.content)) {
            const auto& sc = std::get<size_constraint>(c.content);
            std::ostringstream oss;
            bool first = true;
            if (sc.min_size.has_value()) {
                oss << "if (value.size() < " << sc.min_size.value() << ") {\n";
                oss << "    return error({});\n";
                oss << "}\n";
                first = false;
            }
            if (sc.max_size.has_value()) {
                oss << "if (value.size() > " << sc.max_size.value() << ") {\n";
                oss << "    return error({});\n";
                oss << "}\n";
            }
            if (!first || sc.min_size.has_value() || sc.max_size.has_value()) {
                return oss.str();
            }
        }
    }
    return "";
}

}  // namespace

std::string emit_sequence_of(std::string_view type_name,
                              const sequence_of_type& type,
                              const std::vector<constraint>& constraints) {
    std::ostringstream oss;
    std::string cpp_type = cpp_element_type(*type.element_type);
    bool with_validate = has_size_constraint(constraints);

    oss << "struct " << type_name << " {\n";
    oss << "    std::vector<" << cpp_type << "> value;\n";
    oss << "    bool operator==(const " << type_name << "&) const = default;\n";

    if (with_validate) {
        oss << "\n";
        oss << "    asn1pp::result<void> validate() const {\n";
        oss << emit_validate_method(constraints);
        oss << "        return {};\n";
        oss << "    }\n";
    }

    oss << "};\n";
    oss << "\n";
    oss << "template<> struct asn1pp::asn1_tag<" << type_name << "> {\n";
    oss << "    static constexpr auto value = make_universal(universal_tag::sequence, true);\n";
    oss << "};\n";

    return oss.str();
}

std::string emit_set_of(std::string_view type_name,
                         const set_of_type& type,
                         const std::vector<constraint>& constraints) {
    std::ostringstream oss;
    std::string cpp_type = cpp_element_type(*type.element_type);
    bool with_validate = has_size_constraint(constraints);

    oss << "struct " << type_name << " {\n";
    oss << "    std::vector<" << cpp_type << "> value;\n";
    oss << "    bool operator==(const " << type_name << "&) const = default;\n";

    if (with_validate) {
        oss << "\n";
        oss << "    asn1pp::result<void> validate() const {\n";
        oss << emit_validate_method(constraints);
        oss << "        return {};\n";
        oss << "    }\n";
    }

    oss << "};\n";
    oss << "\n";
    oss << "template<> struct asn1pp::asn1_tag<" << type_name << "> {\n";
    oss << "    static constexpr auto value = make_universal(universal_tag::set, true);\n";
    oss << "};\n";

    return oss.str();
}

}  // namespace asn1pp::gen