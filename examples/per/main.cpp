#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "codec/per/encoder.hpp"
#include "codec/per/decoder.hpp"
#include "codec/per/uper_encoder.hpp"
#include "codec/per/uper_decoder.hpp"
#include "codec/result.hpp"
#include "buffer/buffer_view.hpp"

using namespace asn1pp;

struct Int0To255 {
    static constexpr bool has_range_constraint = true;
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 255;
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 255;
    static constexpr bool extensible = false;
    static constexpr bool has_size_constraint = true;
};

static void print_bytes(const char* label, const uint8_t* data, size_t size) {
    printf("%s (%zu bytes): ", label, size);
    for (size_t i = 0; i < size; ++i) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

int main() {
    constexpr int64_t value = 42;

    uint8_t aper_out[16]{};
    buffer_view aper_obv(aper_out, sizeof(aper_out));

    per::per_aligned_encoder aper_enc;
    auto enc_r = aper_enc.encode_integer<Int0To255>(value, aper_obv);
    if (!enc_r.is_ok()) {
        printf("APER encode failed\n");
        return 1;
    }
    size_t aper_size = (aper_enc.bit_offset() + 7) / 8;
    print_bytes("APER encoded", aper_out, aper_size);

    buffer_view aper_bv(aper_out, aper_size);
    per::per_aligned_decoder aper_dec;
    auto dec_r = aper_dec.decode_integer<Int0To255>(aper_bv);
    if (!dec_r.is_ok()) {
        printf("APER decode failed\n");
        return 1;
    }
    aper_dec.flush_decode();

    printf("APER round-trip: %lld -> %lld\n",
           static_cast<long long>(value), static_cast<long long>(dec_r.value()));

    if (dec_r.value() != value) {
        printf("APER mismatch: expected %lld, got %lld\n",
               static_cast<long long>(value), static_cast<long long>(dec_r.value()));
        return 1;
    }

    uint8_t uper_out[16]{};
    buffer_view uper_obv(uper_out, sizeof(uper_out));

    per::uper_encoder uper_enc;
    enc_r = uper_enc.encode_integer<Int0To255>(value, uper_obv);
    if (!enc_r.is_ok()) {
        printf("UPER encode failed\n");
        return 1;
    }
    uper_enc.flush(uper_obv);
    size_t uper_size = (uper_enc.bit_offset() + 7) / 8;
    print_bytes("UPER encoded", uper_out, uper_size);

    buffer_view uper_bv(uper_out, uper_size);
    per::uper_decoder uper_dec;
    dec_r = uper_dec.decode_integer<Int0To255>(uper_bv);
    if (!dec_r.is_ok()) {
        printf("UPER decode failed\n");
        return 1;
    }
    uper_dec.flush_decode();

    printf("UPER round-trip: %lld -> %lld\n",
           static_cast<long long>(value), static_cast<long long>(dec_r.value()));

    if (dec_r.value() != value) {
        printf("UPER mismatch: expected %lld, got %lld\n",
               static_cast<long long>(value), static_cast<long long>(dec_r.value()));
        return 1;
    }

    printf("\n=== Size Comparison ===\n");
    printf("APER size: %zu bytes\n", aper_size);
    printf("UPER size: %zu bytes\n", uper_size);

    if (uper_size <= aper_size) {
        printf("UPER is %zu byte(s) smaller — no wasted alignment bits\n",
               aper_size - uper_size);
    } else {
        printf("Note: APER used %zu fewer byte(s)\n", uper_size - aper_size);
    }

    printf("\nAll tests passed.\n");
    return 0;
}