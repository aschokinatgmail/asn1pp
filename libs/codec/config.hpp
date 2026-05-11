#pragma once

#include <cstddef>

namespace asn1pp {

#ifdef ASN1PP_EMBEDDED
inline constexpr bool is_embedded = true;
#define ASN1PP_NO_HEAP
static_assert(ASN1PP_MAX_PDU > 0, "ASN1PP_MAX_PDU must be positive in embedded mode");
#else
inline constexpr bool is_embedded = false;
#endif

#ifndef ASN1PP_MAX_PDU
#define ASN1PP_MAX_PDU 2048
#endif

}