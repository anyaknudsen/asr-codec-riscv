#include "codec.h"
#include "codec_api.h"
#include "io.h"
#include "pcm.h"

/*
 * TODO(codec_c): implement the encoder here.
 *
 * Typical structure (see common/io.h and common/pcm.h for the shared
 * helpers available to you):
 *
 *   1. pcm_buffer_t pcm; pcm_read_all(input_path, &pcm);
 *        Loads the whole raw s16le PCM input into memory.
 *   2. FILE *out = io_open_write(output_path);
 *        Opens the compressed bitstream output.
 *   3. Write a small header (e.g. magic bytes, sample count, frame
 *      size) so the decoder can validate and size its own buffers.
 *   4. Iterate over pcm.samples in CODEC_C_FRAME_SAMPLES-sample frames
 *      (pcm_frame_count()/pcm_copy_frame() in common/pcm.h handle frame
 *      counting and zero-padding the final partial frame) and run this
 *      codec's actual encoding algorithm per frame.
 *   5. fclose(out); pcm_free(&pcm);
 *   6. Return the number of frames processed on success, or a negative
 *      value on failure (see common/codec_api.h).
 *
 * main.c (app/main.c) prints whatever non-negative value is returned
 * here as "frames=<n>", which is later used to normalize gem5
 * performance stats (cycles/frame, instructions/frame, ...).
 */
int encode_file(const char *input_path, const char *output_path)
{
    (void)input_path;
    (void)output_path;

    io_report_error("codec_c: encode_file() is not implemented yet");
    return -1;
}
