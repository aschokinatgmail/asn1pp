#include "emitter_time.hpp"
#include "ast.hpp"

namespace asn1pp::gen {
namespace emitter {

emit_result emit_utc_time(const type_ref& type, const std::string& name) {
    (void)type;
    std::string code;
    code += "struct " + name + " {\n";
    code += "    std::string value;\n";
    code += "    bool operator==(const " + name + "&) const = default;\n";
    code += "};\n";
    code += "template<> struct asn1pp::asn1_tag<" + name + "> { static constexpr auto value = make_universal(universal_tag::utc_time); };\n";
    return {code, true, {}};
}

emit_result emit_generalized_time(const type_ref& type, const std::string& name) {
    (void)type;
    std::string code;
    code += "struct " + name + " {\n";
    code += "    std::string value;\n";
    code += "    bool operator==(const " + name + "&) const = default;\n";
    code += "};\n";
    code += "template<> struct asn1pp::asn1_tag<" + name + "> { static constexpr auto value = make_universal(universal_tag::generalized_time); };\n";
    return {code, true, {}};
}

} // namespace emitter
} // namespace asn1pp::gen