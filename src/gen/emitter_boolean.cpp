#include "emitter_boolean.hpp"

namespace asn1pp {
namespace gen {

std::string generator::emit_boolean(std::string_view name) {
    std::string result;
    result += "struct ";
    result += std::string{name};
    result += " {\n";
    result += "    bool value{};\n";
    result += "    operator bool() const { return value; }\n";
    result += "    bool operator==(const ";
    result += std::string{name};
    result += "&) const = default;\n";
    result += "};\n";
    result += "template<> struct asn1pp::asn1_tag<";
    result += std::string{name};
    result += "> { static constexpr auto value = universal_tag::boolean; };\n";
    return result;
}

std::string generator::emit_null(std::string_view name) {
    std::string result;
    result += "struct ";
    result += std::string{name};
    result += " {\n";
    result += "    bool operator==(const ";
    result += std::string{name};
    result += "&) const = default;\n";
    result += "};\n";
    result += "template<> struct asn1pp::asn1_tag<";
    result += std::string{name};
    result += "> { static constexpr auto value = universal_tag::null; };\n";
    return result;
}

}  // namespace gen
}  // namespace asn1pp