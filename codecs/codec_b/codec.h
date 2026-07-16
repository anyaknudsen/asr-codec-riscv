#ifndef CODEC_B_H
#define CODEC_B_H

/*
 * codec_b
 * =======
 *
 * codec_b is MP3 (MPEG-1/2 Audio Layer III): encoding via the vendored
 * `shine` fixed-point encoder, decoding via the vendored `minimp3`
 * decoder. See `vendor/README.md` for exactly what was vendored from
 * each upstream project and why.
 *
 * Design: encoder.c/decoder.c are adapters, not reimplementations
 * -----------------------------------------------------------------
 * `encoder.c` and `decoder.c` do NOT implement an MP3 codec from
 * scratch. All of the actual encoding/decoding algorithm lives in
 * `vendor/shine/` and `vendor/minimp3/` (copied verbatim from their
 * upstream projects). `encoder.c`/`decoder.c` are thin adapters that:
 *   - translate between this project's shared conventions
 *     (`pcm_buffer_t` from common/pcm.h, `io_*` helpers from
 *     common/io.h, the frame-loop / `frames=<n>` return-value
 *     convention from common/codec_api.h) and each vendored library's
 *     own API (`shine_*` / `mp3dec_*`), and
 *   - own the on-disk bitstream format for this codec, which is
 *     simply a raw/headerless MP3 elementary stream (the exact bytes
 *     `shine_encode_buffer()`/`shine_flush()` produce, concatenated -
 *     no extra magic/header is added, since MPEG frame headers already
 *     let a decoder self-synchronize and minimp3 already knows how to
 *     parse that format directly).
 *
 * Build integration
 * ------------------
 * Because the actual codec now spans more than just encoder.c/
 * decoder.c, scripts/build_native.sh and scripts/build_riscv.sh
 * compile every *.c file found anywhere under codecs/<name>/ (not just
 * encoder.c/decoder.c) - see the comment in those scripts. That means
 * everything under codec_b/vendor/ gets built automatically; nothing
 * else needed to change. -Icodecs/<name> is already on the include
 * path, and every #include in encoder.c/decoder.c/vendor/ uses a path
 * relative to codecs/codec_b/ (e.g. "vendor/shine/layer3.h"), so no
 * extra -I flags were needed either.
 *
 * Sample rate / frame size
 * -------------------------
 * The shared dataset (dataset/pcm/, see README.md "Dataset") is
 * mono, 16 kHz, s16le PCM. 16000 Hz is one of the samplerates `shine`
 * supports for MPEG-2 (LSF) Layer III (see vendor/shine/layer3.h's
 * `samplerates` table in the comment above `shine_check_config()`),
 * which is why CODEC_B_SAMPLE_RATE is fixed at 16000 rather than a
 * "typical" MP3 rate like 44100 - it has to match the dataset.
 *
 * CODEC_B_FRAME_SAMPLES is `shine`'s own granule size for MPEG-2
 * Layer III (1 granule/frame * GRANULE_SIZE(576) samples, see
 * `shine_samples_per_pass()`/`granules_per_frame[]` in
 * vendor/shine/layer3.c) - i.e. the number of PCM samples `encoder.c`
 * must feed the encoder per `shine_encode_buffer()` call. encoder.c
 * still calls `shine_samples_per_pass()` at runtime rather than
 * trusting this macro blindly, in case shine's internal granule sizing
 * ever changes; this macro exists to document that value and to size
 * the per-frame PCM scratch buffer.
 *
 * CODEC_B_BITRATE_KBPS is a fixed constant bitrate (CBR) - `shine` has
 * no bitrate-selection heuristics of its own, so this project has to
 * pick one. 32 kbps was chosen as a reasonable CBR operating point for
 * 16 kHz mono speech (well above the ~24-32 kbps commonly considered
 * "acceptable" for narrowband mono voice at this samplerate, while
 * still giving a meaningful compression ratio vs. the raw 256 kbps
 * s16le PCM). It must be one of the bitrates valid for MPEG-2 Layer III
 * (see the `bitrates` table in vendor/shine/layer3.h) - encode_file()
 * calls `shine_check_config()` to verify this at runtime.
 *
 * Bitstream/round-trip note
 * --------------------------
 * MP3 is lossy, and decoding introduces encoder delay/padding, so
 * decode_file(encode_file(input)) does NOT reproduce `input` sample-
 * for-sample - that is expected and does not matter for this project:
 * scripts/compare_outputs.sh only checks that native/Spike/gem5 outputs
 * match *each other* for a given input, never that decoded output
 * matches the original PCM. What must hold (and does, verified with
 * `qemu-riscv64` against every dataset/pcm/*.pcm file, since Spike/gem5
 * were not available in this environment - see the note in
 * decoder.c/encoder.c) is that encode_file()/decode_file() are exactly
 * reproducible across runs and across native/RISC-V builds.
 *
 * The one place this actually took work: minimp3 (decoder.c) by
 * default picks an x86 SSE2 intrinsics synthesis-filter path on native
 * x86-64 builds and a portable scalar-C path on RISC-V (no SSE2
 * there); those two paths sum in different orders and produced
 * off-by-one-int16 native-vs-RISC-V decode differences until
 * `vendor/minimp3/minimp3_impl.c` forced `MINIMP3_NO_SIMD` - the same
 * class of issue `-ffp-contract=off` guards against elsewhere in this
 * project (see README.md "Common C portability issues"), just from
 * SIMD-vs-scalar reassociation instead of FMA contraction.
 */

/* Dataset sample rate codec_b encodes at (Hz). Must be one of shine's
 * supported samplerates - see vendor/shine/layer3.h. */
#define CODEC_B_SAMPLE_RATE 16000

/* PCM samples per channel per shine_encode_buffer() call, for MPEG-2
 * Layer III (1 granule/frame * GRANULE_SIZE). See the comment above. */
#define CODEC_B_FRAME_SAMPLES 576

/* Constant bitrate codec_b encodes at (kbit/s). Must be a valid
 * MPEG-2 Layer III bitrate - see vendor/shine/layer3.h. */
#define CODEC_B_BITRATE_KBPS 32

int encode_file(const char *input_path, const char *output_path);
int decode_file(const char *input_path, const char *output_path);

#endif /* CODEC_B_H */
