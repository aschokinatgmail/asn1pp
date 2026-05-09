#include "emitter_enumerated.hpp"
#include <sstream>
#include <stdexcept>

namespace asn1pp::gen {

std::string emit_enumerated(std::string_view type_name, const enumerated_type& enum_type) {
    std::ostringstream oss;

    oss << "struct " << type_name << " {\n";
    oss << "    enum class value_type : int64_t {\n";

    bool first = true;
    for (const auto& item : enum_type.values) {
        if (!first) {
            oss << ",\n";
        }
        first = false;

        if (enum_type.has_extension && !item.value.has_value()) {
            oss << "        _extension_marker_";
        } else if (item.value.has_value()) {
            oss << "        " << item.name << " = " << item.value.value();
        } else {
            oss << "        " << item.name;
        }
    }

    oss << "\n    };\n";
    oss << "    value_type value{};\n";
    oss << "    bool operator==(const " << type_name << "&) const = default;\n";
    oss << "};\n";
    oss << "template<> struct asn1pp::asn1_tag<" << type_name << "> { static constexpr auto value = asn1pp::universal_tag::enumerated; };\n";

    oss << "\nconstexpr const char* to_string(" << type_name << "::value_type v) {\n";
    oss << "    switch (v) {\n";

    for (const auto& item : enum_type.values) {
        if (enum_type.has_extension && !item.value.has_value()) {
            continue;
        }
        oss << "        case " << type_name << "::value_type::" << item.name << ": return \"" << item.name << "\";\n";
    }

    oss << "        default: return \"unknown\";\n";
    oss << "    }\n";
    oss << "}\n";

    return oss.str();
}
}  // namespace
