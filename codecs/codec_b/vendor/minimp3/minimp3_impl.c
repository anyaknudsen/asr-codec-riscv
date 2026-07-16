/*
 * minimp3 is a single-header library: minimp3.h's declarations are
 * always visible, but its implementation is gated behind
 * MINIMP3_IMPLEMENTATION so it can be included from multiple
 * translation units without duplicate-symbol errors. This is the one
 * translation unit (per codec_b build) that compiles it in.
 *
 * decoder.c includes minimp3.h directly (without defining
 * MINIMP3_IMPLEMENTATION) to see the declarations it calls
 * (mp3dec_init(), mp3dec_decode_frame()); the actual function bodies
 * come from here.
 *
 * MINIMP3_NO_SIMD forces minimp3's portable scalar C synthesis-filter
 * path on every target, instead of letting it pick an x86 SSE2
 * intrinsics path on native builds and a plain-C path on RISC-V (no
 * SSE2 there). Those two paths sum the polyphase synthesis filter in
 * different orders, which - like any floating-point reassociation -
 * can round differently and produce off-by-one-ULP (occasionally
 * off-by-one-int16-after-truncation) PCM differences between native
 * and RISC-V decode output. That's the same class of native-vs-RISC-V
 * floating-point nondeterminism -ffp-contract=off guards against
 * elsewhere in this project (see README.md "Common C portability
 * issues"), so it gets the same fix here: force one deterministic
 * code path on every target rather than let the compiler pick a
 * platform-dependent one.
 */
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_SIMD
#include "minimp3.h"
