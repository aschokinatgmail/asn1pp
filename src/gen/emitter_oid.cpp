#include "emitter_oid.hpp"

#include <sstream>

namespace asn1pp::gen {

std::string emit_object_identifier_type(const object_identifier_type&,
                                        const std::string& name,
                                        const std::vector<constraint>&,
                                        const emitter_options&) {
    std::ostringstream oss;
    oss << "struct " << name << " {\n";
    oss << "    std::vector<uint32_t> arcs;\n";
    oss << "    bool operator==(const " << name << "&) const = default;\n";
    oss << "};\n\n";
    oss << "template<> struct asn1pp::asn1_tag<" << name << "> {\n";
    oss << "    static constexpr auto value = make_universal(universal_tag::object_identifier);\n";
    oss << "};\n\n";
    oss << "static " << name << " from_string(const char* dotted) {\n";
    oss << "    std::vector<uint32_t> arcs;\n";
    oss << "    std::stringstream ss(dotted);\n";
    oss << "    std::string token;\n";
    oss << "    while (std::getline(ss, token, '.')) {\n";
    oss << "        if (!token.empty()) {\n";
    oss << "            arcs.push_back(static_cast<uint32_t>(std::stoul(token)));\n";
    oss << "        }\n";
    oss << "    }\n";
    oss << "    return " << name << "{std::move(arcs)};\n";
    oss << "}\n";
    return oss.str();
}

std::string emit_relative_oid_type(const relative_oid_type&,
                                   const std::string& name,
                                   const std::vector<constraint>&,
                                   const emitter_options&) {
    std::ostringstream oss;
    oss << "struct " << name << " {\n";
    oss << "    std::vector<uint32_t> arcs;\n";
    oss << "    bool operator==(const " << name << "&) const = default;\n";
    oss << "};\n\n";
    oss << "template<> struct asn1pp::asn1_tag<" << name << "> {\n";
    oss << "    static constexpr auto value = make_universal(universal_tag::relative_oid);\n";
    oss << "};\n\n";
    oss << "static " << name << " from_string(const char* dotted) {\n";
    oss << "    std::vector<uint32_t> arcs;\n";
    oss << "    std::stringstream ss(dotted);\n";
    oss << "    std::string token;\n";
    oss << "    while (std::getline(ss, token, '.')) {\n";
    oss << "        if (!token.empty()) {\n";
    oss << "            arcs.push_back(static_cast<uint32_t>(std::stoul(token)));\n";
    oss << "        }\n";
    oss << "    }\n";
    oss << "    return " << name << "{std::move(arcs)};\n";
    oss << "}\n";
    return oss.str();
}

}  // namespace asn1pp::gen