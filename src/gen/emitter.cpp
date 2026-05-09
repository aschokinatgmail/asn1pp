#include "emitter.hpp"

#include <algorithm>
#include <cctype>

namespace asn1pp::gen {

std::string code_emitter::emit_header_guard(std::string_view name) {
    std::string result;
    for (char c : name) {
        if (c >= 'A' && c <= 'Z') result += '_', result += c;
        else if (c >= 'a' && c <= 'z') result += c;
        else if (c >= '0' && c <= '9') result += c;
        else result += '_';
    }
    std::transform(result.begin(), result.end(), result.begin(),
                   [](char c) { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); });
    return "ASN1PP_" + result + "_HPP";
}

std::string code_emitter::emit_includes(const std::vector<std::string>& includes) {
    std::string result;
    for (const auto& inc : includes) {
        if (!inc.empty() && inc.front() == '<') result += "#include " + inc + "\n";
        else result += "#include \"" + inc + "\"\n";
    }
    return result;
}

std::string code_emitter::emit_namespace_open(const std::string& ns) {
    if (ns.empty()) return {};
    return "namespace " + ns + " {\n";
}

std::string code_emitter::emit_namespace_close(const std::string& ns) {
    if (ns.empty()) return {};
    return "}  // namespace " + ns + "\n";
}

}  // namespace asn1pp::gen