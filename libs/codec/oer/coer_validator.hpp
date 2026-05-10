#pragma once

#include <cstdint>
#include <cstddef>
#include <span>

#include "codec/result.hpp"
#include "buffer/buffer_view.hpp"

namespace asn1pp::oer {

class coer_validator {
public:
    template<typename OerMeta>
    static result<void> validate_integer(const buffer_view& encoded);

    template<typename OerMeta>
    static result<void> validate_octet_string(const buffer_view& encoded);

    static result<void> validate_length_determinant(const buffer_view& encoded);

    template<typename OerMeta>
    static result<void> validate(const buffer_view& encoded);

private:
    static bool is_integer_bytes_minimal(const uint8_t* data, size_t size);
};

template<typename OerMeta>
result<void> coer_validator::validate_integer(const buffer_view& encoded) {
    if (encoded.empty()) {
        return result<void>::err(error_code::constraint_violation);
    }

    if (!OerMeta::has_range_constraint) {
        size_t len_bytes = 0;
        size_t content_start = 0;
        
        uint8_t first = encoded.data()[0];
        if ((first & 0x80) == 0) {
            len_bytes = 1;
            content_start = 1;
        } else if ((first & 0xC0) == 0x80) {
            size_t num_octets = (first & 0x3F);
            if (num_octets == 0 || num_octets > 4) {
                return result<void>::err(error_code::constraint_violation);
            }
            len_bytes = 1 + num_octets;
            content_start = 1 + num_octets;
        } else {
            return result<void>::err(error_code::constraint_violation);
        }

        if (encoded.size() < len_bytes) {
            return result<void>::err(error_code::buffer_underflow);
        }

        size_t content_len = 0;
        if ((first & 0x80) == 0) {
            content_len = first;
        } else {
            for (size_t i = 0; i < (first & 0x3F); ++i) {
                content_len = (content_len << 8) | encoded.data()[1 + i];
            }
        }

        if (encoded.size() < content_start + content_len) {
            return result<void>::err(error_code::buffer_underflow);
        }

        if (content_len == 0) {
            return result<void>::err(error_code::constraint_violation);
        }

        if (!is_integer_bytes_minimal(encoded.data() + content_start, content_len)) {
            return result<void>::err(error_code::constraint_violation);
        }
    }
    return result<void>::ok();
}

template<typename OerMeta>
result<void> coer_validator::validate_octet_string(const buffer_view& encoded) {
    if (OerMeta::has_size_constraint) {
        return result<void>::ok();
    }
    return validate_length_determinant(encoded);
}

template<typename OerMeta>
result<void> coer_validator::validate(const buffer_view& encoded) {
    return validate_integer<OerMeta>(encoded);
}

bool coer_validator::is_integer_bytes_minimal(const uint8_t* data, size_t size) {
    if (size == 0) return false;
    if (size == 1) return true;

    const uint8_t first = data[0];
    const uint8_t second = data[1];

    if (first == 0x00 && (second & 0x80) == 0) {
        return false;
    }
    if (first == 0xFF && (second & 0x80) != 0) {
        return false;
    }
    return true;
}

result<void> coer_validator::validate_length_determinant(const buffer_view& encoded) {
    if (encoded.empty()) {
        return result<void>::err(error_code::constraint_violation);
    }

    uint8_t first = encoded.data()[0];

    if ((first & 0x80) == 0) {
        return result<void>::ok();
    }

    if ((first & 0xC0) == 0x80) {
        size_t num_octets = (first & 0x3F);
        if (num_octets == 0) {
            return result<void>::err(error_code::constraint_violation);
        }
        if (encoded.size() < 1 + num_octets) {
            return result<void>::err(error_code::buffer_underflow);
        }
        if (encoded.data()[1] == 0) {
            return result<void>::err(error_code::constraint_violation);
        }

        size_t len = 0;
        for (size_t i = 0; i < num_octets; ++i) {
            len = (len << 8) | encoded.data()[1 + i];
        }

        if (len < 128) {
            return result<void>::err(error_code::constraint_violation);
        }

        return result<void>::ok();
    }

    return result<void>::err(error_code::constraint_violation);
}

} // namespace asn1pp::oer
