#include <cstdint>
#include <cstddef>
#include <cstring>

#include "codec/config.hpp"
#include "codec/ber/decoder.hpp"
#include "codec/ber/encoder.hpp"
#include "codec/per/decoder.hpp"
#include "codec/per/encoder.hpp"
#include "codec/per/uper_decoder.hpp"
#include "codec/per/uper_encoder.hpp"
#include "codec/oer/decoder.hpp"
#include "codec/oer/encoder.hpp"
#include "codec/result.hpp"
#include "buffer/buffer_view.hpp"
#include "arch/simd.hpp"

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

extern "C" {

__attribute__((used)) void _start() {
    {
        uint8_t ber_int[] = {0x02, 0x01, 0x2A};
        buffer_view bv(ber_int, sizeof(ber_int));
        ber::ber_decoder dec;
        volatile auto r = dec.decode_integer(bv);
        (void)r;
    }
    {
        uint8_t out[16]{};
        buffer_view obv(out, sizeof(out));
        ber::ber_encoder enc;
        enc.encode_integer(42, obv);
    }
    {
        uint8_t per_bytes[] = {0x1A};
        buffer_view bv(per_bytes, sizeof(per_bytes));
        per::per_aligned_decoder dec;
        volatile auto r = dec.decode_integer<Int0To255>(bv);
        (void)r;
        dec.flush_decode();
    }
    {
        uint8_t out[16]{};
        buffer_view obv(out, sizeof(out));
        per::per_aligned_encoder enc;
        enc.encode_integer<Int0To255>(42, obv);
    }
    {
        uint8_t uper_bytes[] = {0x54};
        buffer_view bv(uper_bytes, sizeof(uper_bytes));
        per::uper_decoder dec;
        volatile auto r = dec.decode_integer<Int0To255>(bv);
        (void)r;
        dec.flush_decode();
    }
    {
        uint8_t out[16]{};
        buffer_view obv(out, sizeof(out));
        per::uper_encoder enc;
        enc.encode_integer<Int0To255>(42, obv);
    }
    {
        uint8_t oer_bytes[] = {0x01, 0x2A};
        buffer_view bv(oer_bytes, sizeof(oer_bytes));
        oer::oer_decoder dec;
        volatile auto r = dec.decode_integer<Int0To255>(bv);
        (void)r;
    }
    {
        uint8_t out[16]{};
        buffer_view obv(out, sizeof(out));
        oer::oer_encoder enc;
        enc.encode_integer<Int0To255>(42, obv);
    }

    while (1) {}
}

void _exit(int) { while(1) {} }
void _kill(int) { while(1) {} }
int _getpid() { return 1; }
}
