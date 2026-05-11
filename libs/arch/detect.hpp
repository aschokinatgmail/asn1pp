#pragma once

#include <mutex>

namespace asn1pp::arch {

enum class simd_level {
    scalar,
    sse42,
    avx2,
    neon
};

struct cpu_features {
    bool has_sse42 = false;
    bool has_avx2  = false;
    bool has_neon  = false;
};

#if defined(__x86_64__) || defined(_M_X64)

inline cpu_features detect_cpu() {
    cpu_features f{};
    f.has_sse42 = __builtin_cpu_supports("sse4.2");
    f.has_avx2  = __builtin_cpu_supports("avx2");
    return f;
}

#elif defined(__aarch64__) || defined(__ARM_NEON)

// ARM platform detection
#if defined(__linux__)
#include <sys/auxv.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#endif

inline cpu_features detect_cpu() {
    cpu_features f{};
#if defined(__linux__)
    unsigned long hwcap = getauxval(AT_HWCAP);
    f.has_neon = (hwcap & HWCAP_ASIMD) != 0;
#elif defined(__APPLE__)
    int neon_present = 0;
    size_t size = sizeof(neon_present);
    if (sysctlbyname("hw.optional.arm.FEAT_NEON", &neon_present, &size, nullptr, 0) == 0) {
        f.has_neon = (neon_present != 0);
    }
#endif
    return f;
}

#else

// Unknown platform — fallback to scalar
inline cpu_features detect_cpu() {
    return cpu_features{};
}

#endif

inline simd_level available_simd_level() {
    static simd_level cached = simd_level::scalar;
    static std::once_flag flag;
    std::call_once(flag, [&] {
        auto features = detect_cpu();
        if (features.has_avx2)       cached = simd_level::avx2;
        else if (features.has_sse42) cached = simd_level::sse42;
        else if (features.has_neon)  cached = simd_level::neon;
        else                         cached = simd_level::scalar;
    });
    return cached;
}

}  // namespace asn1pp::arch
