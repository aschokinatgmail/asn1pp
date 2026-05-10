#include "emitter_choice.hpp"
#include <sstream>

#include "../../libs/codec/traits.hpp"

namespace asn1pp::gen {

namespace {

std::string cpp_type_for_alternative(const type_ref& type) {
    return std::visit(
        [](const auto& alt) -> std::string {
            using T = std::decay_t<decltype(alt)>;

            if constexpr (std::is_same_v<T, integer_type>)   return "int64_t";
            if constexpr (std::is_same_v<T, boolean_type>)   return "bool";
            if constexpr (std::is_same_v<T, null_type>)      return "std::monostate";
            if constexpr (std::is_same_v<T, real_type>)      return "double";
            if constexpr (std::is_same_v<T, octet_string_type>)    return "std::vector<uint8_t>";
            if constexpr (std::is_same_v<T, object_identifier_type>) return "std::vector<uint32_t>";
            if constexpr (std::is_same_v<T, relative_oid_type>)      return "std::vector<uint32_t>";
            if constexpr (std::is_same_v<T, any_type>)       return "std::vector<uint8_t>";
            if constexpr (std::is_same_v<T, std::string>)    return alt;
            if constexpr (std::is_same_v<T, enumerated_type>) return "int64_t";
            if constexpr (std::is_same_v<T, bit_string_type>) return "std::pair<std::vector<uint8_t>, size_t>";
            if constexpr (std::is_same_v<T, std::unique_ptr<sequence_type>>)  return "int";
            if constexpr (std::is_same_v<T, std::unique_ptr<set_type>>)        return "int";
            if constexpr (std::is_same_v<T, std::unique_ptr<choice_type>>)     return "int";
            if constexpr (std::is_same_v<T, std::unique_ptr<sequence_of_type>>) return "int";
            if constexpr (std::is_same_v<T, std::unique_ptr<set_of_type>>)      return "int";
            if constexpr (std::is_same_v<T, std::unique_ptr<tagged_type>>)
                return cpp_type_for_alternative(*alt->underlying_type);
            if constexpr (std::is_same_v<T, std::unique_ptr<constrained_type>>)
                return cpp_type_for_alternative(*alt->underlying_type);
            return "void";
        },
        type.content);
}

}  // anonymous namespace

std::string emit_choice(std::string_view type_name, const choice_type& choice,
                         tag_default /*tagging*/) {
    std::ostringstream oss;

    const auto& alts = choice.alternatives;
    const size_t num_alts = alts.size();
    const size_t total_variants = num_alts + (choice.has_extension ? 1 : 0);

    oss << "struct " << type_name << " {\n";

    // --- which enum ---
    oss << "    enum class which : size_t {\n";
    for (size_t i = 0; i < num_alts; ++i) {
        oss << "        " << alts[i].name << " = " << i;
        if (i + 1 < num_alts || choice.has_extension) oss << ",";
        oss << "\n";
    }
    if (choice.has_extension) {
        oss << "        _unknown_extension = " << num_alts;
        if (num_alts > 0 && num_alts + 1 < total_variants) oss << ",";
        oss << "\n";
    }
    oss << "    };\n\n";

    // --- value variant ---
    oss << "    std::variant<";
    for (size_t i = 0; i < num_alts; ++i) {
        if (i > 0) oss << ", ";
        oss << cpp_type_for_alternative(*alts[i].type);
    }
    if (choice.has_extension) {
        if (num_alts > 0) oss << ", ";
        oss << "std::vector<uint8_t>";
    }
    oss << "> value;\n\n";

    // --- index() ---
    oss << "    [[nodiscard]] which index() const noexcept {\n";
    oss << "        return static_cast<which>(value.index());\n";
    oss << "    }\n\n";

    // --- is_<name>() ---
    for (const auto& alt : alts) {
        std::string cpp_type = cpp_type_for_alternative(*alt.type);
        oss << "    [[nodiscard]] bool is_" << alt.name << "() const noexcept {\n";
        oss << "        return std::holds_alternative<" << cpp_type << ">(value);\n";
        oss << "    }\n";
    }
    if (choice.has_extension) {
        oss << "    [[nodiscard]] bool is__unknown_extension() const noexcept {\n";
        oss << "        return std::holds_alternative<std::vector<uint8_t>>(value);\n";
        oss << "    }\n";
    }
    oss << "\n";

    // --- get_<name>() mutable ---
    for (const auto& alt : alts) {
        std::string cpp_type = cpp_type_for_alternative(*alt.type);
        oss << "    " << cpp_type << "& get_" << alt.name << "() {\n";
        oss << "        return std::get<" << cpp_type << ">(value);\n";
        oss << "    }\n";
    }
    if (choice.has_extension) {
        oss << "    std::vector<uint8_t>& get__unknown_extension() {\n";
        oss << "        return std::get<std::vector<uint8_t>>(value);\n";
        oss << "    }\n";
    }
    oss << "\n";

    // --- get_<name>() const ---
    for (const auto& alt : alts) {
        std::string cpp_type = cpp_type_for_alternative(*alt.type);
        oss << "    const " << cpp_type << "& get_" << alt.name << "() const {\n";
        oss << "        return std::get<" << cpp_type << ">(value);\n";
        oss << "    }\n";
    }
    if (choice.has_extension) {
        oss << "    const std::vector<uint8_t>& get__unknown_extension() const {\n";
        oss << "        return std::get<std::vector<uint8_t>>(value);\n";
        oss << "    }\n";
    }
    oss << "\n";

    // --- operator== ---
    oss << "    bool operator==(const " << type_name << "&) const = default;\n";

    oss << "};\n";

    // --- tag specialization ---
    oss << "template<> struct asn1pp::asn1_tag<" << type_name
        << "> { static constexpr auto value = make_universal(universal_tag::sequence, true); };\n";

    return oss.str();
}

}  // namespace asn1pp::gen
