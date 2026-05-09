#pragma once

#include <string>
#include <string_view>

namespace asn1pp {
namespace gen {

class generator {
public:
    generator() = default;
    ~generator() = default;
    generator(const generator&) = delete;
    generator& operator=(const generator&) = delete;
    generator(generator&&) noexcept = default;
    generator& operator=(generator&&) noexcept = default;

    [[nodiscard]] std::string emit_boolean(std::string_view name);
    [[nodiscard]] std::string emit_null(std::string_view name);
};

}  // namespace gen
}  // namespace asn1pp