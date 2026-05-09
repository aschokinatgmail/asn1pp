#include <gtest/gtest.h>
#include "../../src/gen/emitter_enumerated.hpp"
#include "../../src/gen/ast.hpp"

#include <string>
#include <string_view>

namespace {

using namespace asn1pp::gen;

std::string emit(std::string_view type_name, std::vector<enumeration_item> values, bool has_extension = false) {
    enumerated_type e;
    e.values = std::move(values);
    e.has_extension = has_extension;
    return emit_enumerated(type_name, e);
}

std::string emit_simple(std::string_view type_name, std::vector<enumeration_item> values) {
    return emit(type_name, std::move(values), false);
}

}  // namespace

TEST(EmitterEnumerated, BasicEnumGeneratesCorrectValues) {
    std::vector<enumeration_item> values = {
        {"red", 0},
        {"green", 1},
        {"blue", 2}
    };
    std::string result = emit_simple("E", values);

    EXPECT_NE(result.find("struct E {"), std::string::npos);
    EXPECT_NE(result.find("enum class value_type : int64_t {"), std::string::npos);
    EXPECT_NE(result.find("red = 0"), std::string::npos);
    EXPECT_NE(result.find("green = 1"), std::string::npos);
    EXPECT_NE(result.find("blue = 2"), std::string::npos);
}

TEST(EmitterEnumerated, PreservesExplicitValues) {
    std::vector<enumeration_item> values = {
        {"a", 10},
        {"b", 20}
    };
    std::string result = emit_simple("E", values);

    EXPECT_NE(result.find("a = 10"), std::string::npos);
    EXPECT_NE(result.find("b = 20"), std::string::npos);
}

TEST(EmitterEnumerated, ExtensionMarkerHandled) {
    std::vector<enumeration_item> values = {
        {"red", 0},
        {"green", 1},
        {"_extension_marker_", std::nullopt},
        {"blue", 5}
    };
    std::string result = emit("E", values, true);

    EXPECT_NE(result.find("red = 0"), std::string::npos);
    EXPECT_NE(result.find("_extension_marker_"), std::string::npos);
    EXPECT_NE(result.find("blue = 5"), std::string::npos);
}

TEST(EmitterEnumerated, Asn1TagIsEnumerated) {
    std::vector<enumeration_item> values = {
        {"a", 0}
    };
    std::string result = emit_simple("E", values);

    EXPECT_NE(result.find("asn1pp::asn1_tag<E>"), std::string::npos);
    EXPECT_NE(result.find("asn1pp::universal_tag::enumerated"), std::string::npos);
}

TEST(EmitterEnumerated, OperatorEqualsGenerated) {
    std::vector<enumeration_item> values = {
        {"a", 0}
    };
    std::string result = emit_simple("E", values);

    EXPECT_NE(result.find("operator==(const E&) const = default"), std::string::npos);
}

TEST(EmitterEnumerated, ToStringGenerated) {
    std::vector<enumeration_item> values = {
        {"red", 0},
        {"green", 1}
    };
    std::string result = emit_simple("E", values);

    EXPECT_NE(result.find("constexpr const char* to_string(E::value_type v)"), std::string::npos);
    EXPECT_NE(result.find("switch (v)"), std::string::npos);
    EXPECT_NE(result.find("case E::value_type::red: return \"red\""), std::string::npos);
    EXPECT_NE(result.find("case E::value_type::green: return \"green\""), std::string::npos);
    EXPECT_NE(result.find("default: return \"unknown\""), std::string::npos);
}

TEST(EmitterEnumerated, GeneratedCodeCompilesAsValidCpp20) {
    std::vector<enumeration_item> values = {
        {"a", 0},
        {"b", 1}
    };
    std::string result = emit_simple("E", values);

    result += R"(
static_assert(true);
int main() {
    E e;
    e.value = E::value_type::a;
    E e2;
    e2.value = E::value_type::b;
    bool eq = (e == e2);
    (void)eq;
    const char* s = to_string(E::value_type::a);
    (void)s;
    return 0;
}
)";

    EXPECT_TRUE(std::string(result).find("#include") == std::string::npos);
}
