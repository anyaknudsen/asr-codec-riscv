#ifndef CODEC_API_H
#define CODEC_API_H

/*
 * Shared interface implemented by every codec in codecs/<name>/.
 *
 * Each codec provides its own encoder.c and decoder.c which define these
 * two symbols. app/main.c is linked against exactly one codec's object
 * files at build time (see scripts/build_native.sh and
 * scripts/build_riscv.sh), so main.c never needs to know which codec it
 * was built with.
 *
 * Both functions return the number of frames processed on success, or a
 * negative value on failure. main.c reports the frame count so it can be
 * used later to normalize gem5 performance stats (cycles/frame,
 * instructions/frame, etc.).
 */

int encode_file(const char *input_path, const char *output_path);
int decode_file(const char *input_path, const char *output_path);

#endif /* CODEC_API_H */
