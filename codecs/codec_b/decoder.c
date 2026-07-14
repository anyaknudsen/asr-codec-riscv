#include "codec.h"
#include "codec_api.h"
#include "io.h"
#include "pcm.h"

/*
 * TODO(codec_b): implement the decoder here.
 *
 * Typical structure (mirrors encoder.c):
 *
 *   1. FILE *in = io_open_read(input_path);
 *        Opens the compressed bitstream produced by this codec's
 *        encode_file().
 *   2. Read and validate the header written by the encoder (magic
 *      bytes, sample count, frame size, ...).
 *   3. Allocate an int16_t output buffer sized to the decoded sample
 *      count, then decode frame by frame, reconstructing samples with
 *      this codec's actual decoding algorithm. Use pcm_clip16() from
 *      common/pcm.h to saturate any reconstructed value into the valid
 *      int16_t range.
 *   4. pcm_write_all(output_path, samples, sample_count);
 *        Writes the reconstructed raw s16le PCM output.
 *   5. Return the number of frames processed on success, or a negative
 *      value on failure (see common/codec_api.h).
 *
 * The decoded PCM output from this function is what gets compared
 * across native / Spike / gem5 runs (see scripts/compare_outputs.sh),
 * so decode_file() must be fully deterministic.
 */
int decode_file(const char *input_path, const char *output_path)
{
    (void)input_path;
    (void)output_path;

    io_report_error("codec_b: decode_file() is not implemented yet");
    return -1;
}
