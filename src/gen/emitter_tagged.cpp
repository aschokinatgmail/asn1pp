#include "emitter_tagged.hpp"

#include <sstream>
#include <variant>
#include <string>

#include "../../libs/codec/traits.hpp"

namespace asn1pp::gen {

namespace {

std::string tag_class_name(tag_class cls) {
    using enum tag_class;
    switch (cls) {
        case universal:        return "universal";
        case application:      return "application";
        case context_specific: return "context_specific";
        case private_class:    return "private_class";
    }
    return "universal";
}

std::string underlying_cpp_type(const type_ref& type) {
    return std::visit(
        [](const auto& alt) -> std::string {
            using T = std::decay_t<decltype(alt)>;

            if constexpr (std::is_same_v<T, integer_type>)           return "int64_t";
            if constexpr (std::is_same_v<T, boolean_type>)           return "bool";
            if constexpr (std::is_same_v<T, null_type>)              return "std::monostate";
            if constexpr (std::is_same_v<T, real_type>)              return "double";
            if constexpr (std::is_same_v<T, octet_string_type>)      return "std::vector<uint8_t>";
            if constexpr (std::is_same_v<T, object_identifier_type>) return "std::vector<uint32_t>";
            if constexpr (std::is_same_v<T, relative_oid_type>)      return "std::vector<uint32_t>";
            if constexpr (std::is_same_v<T, any_type>)               return "std::vector<uint8_t>";
            if constexpr (std::is_same_v<T, enumerated_type>)        return "int64_t";
            if constexpr (std::is_same_v<T, bit_string_type>)        return "std::pair<std::vector<uint8_t>, size_t>";
            if constexpr (std::is_same_v<T, std::string>)            return alt;
            if constexpr (std::is_same_v<T, std::unique_ptr<sequence_type>>) return "std::vector<uint8_t>";
            if constexpr (std::is_same_v<T, std::unique_ptr<set_type>>)       return "std::vector<uint8_t>";
            if constexpr (std::is_same_v<T, std::unique_ptr<choice_type>>)    return "std::vector<uint8_t>";
            if constexpr (std::is_same_v<T, std::unique_ptr<sequence_of_type>>) return "std::vector<uint8_t>";
            if constexpr (std::is_same_v<T, std::unique_ptr<set_of_type>>)       return "std::vector<uint8_t>";
            if constexpr (std::is_same_v<T, std::unique_ptr<tagged_type>>)
                return underlying_cpp_type(*alt->underlying_type);
            if constexpr (std::is_same_v<T, std::unique_ptr<constrained_type>>)
                return underlying_cpp_type(*alt->underlying_type);
            return "void";
        },
        type.content);
}

}  // namespace

std::string emit_tagged_type(std::string_view type_name,
                             const tagged_type& tt,
                             const emitter_options& opts) {
    std::ostringstream oss;

    bool constructed = tt.tag_value.constructed;
    const auto& inner = *tt.underlying_type;

    if (tt.implicit) {
        std::string inner_type = underlying_cpp_type(inner);

        oss << "struct " << type_name << " {\n";
        oss << "    " << inner_type << " value{};\n";
        if (opts.include_operators) {
            oss << "    bool operator==(const " << type_name << "&) const = default;\n";
        }
        oss << "};\n";

        oss << "template<> struct asn1pp::tag_for_type<"
            << (opts.namespace_name.empty() ? "" : opts.namespace_name + "::")
            << type_name << "> {\n";
        oss << "    static constexpr asn1pp::tag value = "
            << "asn1pp::tag{asn1pp::tag_class::" << tag_class_name(tt.tag_value.cls)
            << ", " << (constructed ? "true" : "false")
            << ", " << tt.tag_value.number << "};\n";
        oss << "};\n";
    } else {
        std::string inner_type = underlying_cpp_type(inner);

        oss << "struct " << type_name << " {\n";
        oss << "    " << inner_type << " value{};\n";
        if (opts.include_operators) {
            oss << "    bool operator==(const " << type_name << "&) const = default;\n";
        }
        oss << "};\n";

        oss << "template<> struct asn1pp::asn1_explicit_outer_tag<"
            << (opts.namespace_name.empty() ? "" : opts.namespace_name + "::")
            << type_name << "> {\n";
        oss << "    static constexpr asn1pp::tag outer_tag = "
            << "asn1pp::tag{asn1pp::tag_class::" << tag_class_name(tt.tag_value.cls)
            << ", " << (constructed ? "true" : "false")
            << ", " << tt.tag_value.number << "};\n";
        oss << "};\n";
    }

    return oss.str();
}

}  // namespace asn1pp::gen
