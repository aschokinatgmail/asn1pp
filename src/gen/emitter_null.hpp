#pragma once

#include <string>
#include <string_view>

namespace asn1pp {
namespace gen {

inline std::string emit_null_standalone(std::string_view name) {
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