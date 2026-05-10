#include "emitter_ioc.hpp"

#include <sstream>
#include <string>

namespace asn1pp::gen {

namespace {

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

std::string cpp_type_from_asn1(const std::string& asn1_type) {
    if (asn1_type == "INTEGER") return "int64_t";
    if (asn1_type == "BOOLEAN") return "bool";
    if (asn1_type == "REAL") return "double";
    if (asn1_type == "NULL") return "std::monostate";
    if (asn1_type == "OCTET_STRING" || asn1_type == "BIT_STRING") {
        return "std::vector<uint8_t>";
    }
    if (asn1_type == "OBJECT_IDENTIFIER") return "std::vector<uint32_t>";
    return asn1_type;
}

std::string value_to_string(const value_ref& vr) {
    if (std::holds_alternative<int64_t>(vr.content)) {
        return std::to_string(std::get<int64_t>(vr.content));
    }
    if (std::holds_alternative<bool>(vr.content)) {
        return std::get<bool>(vr.content) ? "true" : "false";
    }
    if (std::holds_alternative<null_value>(vr.content)) {
        return "std::monostate{}";
    }
    if (std::holds_alternative<std::string>(vr.content)) {
        return "\"" + std::get<std::string>(vr.content) + "\"";
    }
    return "{}";
}

}  // namespace

std::string emit_class(const class_type& ct) {
    std::ostringstream oss;

    oss << "struct " << sanitize_name(ct.name) << " {\n";
    for (const auto& f : ct.fields) {
        std::string field_name = sanitize_name(f.name.substr(1));
        std::string cpp_type = f.type_name
            ? cpp_type_from_asn1(*f.type_name)
            : "std::variant<std::monostate>";

        if (f.optional) {
            oss << "    std::optional<" << cpp_type << "> " << field_name << ";\n";
        } else {
            oss << "    " << cpp_type << " " << field_name << "{};\n";
        }
    }
    oss << "};\n";

    return oss.str();
}

std::string emit_information_object(
    const information_object& obj,
    const std::unordered_map<std::string, class_type>&) {
    std::ostringstream oss;

    oss << "constexpr auto " << sanitize_name(obj.name)
        << " = " << sanitize_name(obj.class_name) << "{\n";
    for (size_t i = 0; i < obj.field_values.size(); ++i) {
        std::string clean_name = sanitize_name(obj.field_values[i].field_name.substr(1));
        oss << "    ." << clean_name << " = "
            << value_to_string(obj.field_values[i].value);
        if (i + 1 < obj.field_values.size()) {
            oss << ",\n";
        } else {
            oss << "\n";
        }
    }
    oss << "};\n";

    return oss.str();
}

std::string emit_object_set(const object_set& os) {
    std::ostringstream oss;

    oss << "constexpr std::array<"
        << sanitize_name(os.class_name)
        << ", " << os.objects.size()
        << "> " << sanitize_name(os.name) << "{{\n";

    for (size_t i = 0; i < os.objects.size(); ++i) {
        oss << "    " << sanitize_name(os.objects[i]);
        if (i + 1 < os.objects.size()) {
            oss << ",\n";
        } else {
            oss << "\n";
        }
    }

    oss << "}};\n";

    return oss.str();
}

}  // namespace asn1pp::gen
