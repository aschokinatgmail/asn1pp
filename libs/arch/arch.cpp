#include <cstddef>

namespace asn1pp::arch {

struct platform_state {
    static platform_state& instance() {
        static platform_state state{};
        return state;
    }

    platform_state(const platform_state&) = delete;
    platform_state& operator=(const platform_state&) = delete;

private:
    platform_state() = default;
};

}  // namespace asn1pp::arch