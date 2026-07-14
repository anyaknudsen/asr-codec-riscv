#ifndef CODEC_C_H
#define CODEC_C_H

/*
 * codec_c
 * =======
 *
 * TODO(codec_c): this codec's algorithm is not implemented yet. This
 * header only reserves the layout described in the project README so
 * that build scripts, run scripts, and the shared app/main.c entry
 * point already know how to treat codec_c identically to codec_a and
 * codec_b.
 *
 * Fill in below:
 *   - Sample rate / frame size this codec expects.
 *   - Any other codec-specific constants (bitstream magic bytes,
 *     quantizer tables, filter/LPC order, mode-decision thresholds,
 *     codebook sizes, etc.).
 *   - Any internal structs shared between encoder.c and decoder.c
 *     (put those here, or in a private codec_c-only helper header next
 *     to this file, e.g. codecs/codec_c/internal.h).
 *
 * encode_file()/decode_file() are declared here to mirror
 * common/codec_api.h (the interface app/main.c calls into). Do not
 * change their signatures - build scripts link app/main.c directly
 * against this codec's encoder.c/decoder.c, so these two symbols are
 * the only contract that needs to hold.
 *
 * Note: if this codec ends up using floating point, build it with
 * -march=rv64gc (or this environment's rv64gc-equivalent, see
 * scripts/build_riscv.sh) and -mabi=lp64d, and keep
 * -ffp-contract=off in the build flags so native and RISC-V builds
 * round floating-point multiply-adds identically (see README.md,
 * "Common C portability issues").
 */

/* TODO(codec_c): set the sample rate this codec expects, e.g. 8000 or
 * 16000 (Hz). */
#define CODEC_C_SAMPLE_RATE 0

/* TODO(codec_c): set the number of PCM samples per frame, e.g. 160 for
 * 10 ms @ 16 kHz, or 320 for 20 ms @ 16 kHz. */
#define CODEC_C_FRAME_SAMPLES 0

int encode_file(const char *input_path, const char *output_path);
int decode_file(const char *input_path, const char *output_path);

#endif /* CODEC_C_H */
