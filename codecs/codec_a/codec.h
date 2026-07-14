#ifndef CODEC_A_H
#define CODEC_A_H

/*
 * codec_a
 * =======
 *
 * TODO(codec_a): this codec's algorithm is not implemented yet. This
 * header only reserves the layout described in the project README so
 * that build scripts, run scripts, and the shared app/main.c entry
 * point already know how to treat codec_a identically to codec_b and
 * codec_c.
 *
 * Fill in below:
 *   - Sample rate / frame size this codec expects.
 *   - Any other codec-specific constants (bitstream magic bytes,
 *     quantizer tables, filter/LPC order, mode-decision thresholds,
 *     codebook sizes, etc.).
 *   - Any internal structs shared between encoder.c and decoder.c
 *     (put those here, or in a private codec_a-only helper header next
 *     to this file, e.g. codecs/codec_a/internal.h).
 *
 * encode_file()/decode_file() are declared here to mirror
 * common/codec_api.h (the interface app/main.c calls into). Do not
 * change their signatures - build scripts link app/main.c directly
 * against this codec's encoder.c/decoder.c, so these two symbols are
 * the only contract that needs to hold.
 */

/* TODO(codec_a): set the sample rate this codec expects, e.g. 8000 or
 * 16000 (Hz). */
#define CODEC_A_SAMPLE_RATE 0

/* TODO(codec_a): set the number of PCM samples per frame, e.g. 160 for
 * 10 ms @ 16 kHz, or 320 for 20 ms @ 16 kHz. */
#define CODEC_A_FRAME_SAMPLES 0

int encode_file(const char *input_path, const char *output_path);
int decode_file(const char *input_path, const char *output_path);

#endif /* CODEC_A_H */
