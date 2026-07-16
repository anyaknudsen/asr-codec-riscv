#include <stdlib.h>

#include "codec.h"
#include "codec_api.h"
#include "io.h"
#include "pcm.h"
#include "vendor/minimp3/minimp3.h"

/*
 * codec_b decoder: an adapter around the vendored `minimp3` decoder
 * (see codec.h and vendor/README.md for the full design rationale).
 * This file owns none of the actual decoding algorithm - it only:
 *
 *   1. Reads the whole compressed input file (the raw MP3 elementary
 *      stream encoder.c produced) into memory.
 *   2. Drives minimp3's frame loop (mp3dec_decode_frame()) over that
 *      buffer, appending each frame's decoded PCM samples to a growable
 *      output buffer.
 *   3. Writes the concatenated PCM samples out (common/pcm.h).
 *
 * See codec.h "Bitstream/round-trip note" for why the decoded sample
 * count does not need to match the original encoder input's sample
 * count - it never will exactly, for any MP3 encoder/decoder pair, and
 * that is not what this project's correctness checks rely on.
 */

/* Growable int16_t output buffer, doubling capacity as needed. Kept
 * local to this file since no other codec needs a generic growable
 * PCM buffer (common/pcm.h's pcm_buffer_t is sized once up front by
 * pcm_read_all(), which doesn't fit the "decode a variable, a priori
 * unknown number of MP3 frames" shape here). */
typedef struct {
    int16_t *data;
    size_t count;
    size_t capacity;
} growable_pcm_t;

static int growable_pcm_reserve(growable_pcm_t *buf, size_t extra)
{
    if (buf->count + extra <= buf->capacity)
        return 0;

    size_t new_capacity = buf->capacity ? buf->capacity * 2 : 4096;
    while (new_capacity < buf->count + extra)
        new_capacity *= 2;

    int16_t *grown = realloc(buf->data, new_capacity * sizeof(int16_t));
    if (!grown)
        return -1;

    buf->data = grown;
    buf->capacity = new_capacity;
    return 0;
}

int decode_file(const char *input_path, const char *output_path)
{
    FILE *in = io_open_read(input_path);
    if (!in)
        return -1;

    long size = io_file_size(in);
    if (size < 0) {
        io_report_error("codec_b: failed to determine size of '%s'", input_path);
        fclose(in);
        return -1;
    }

    uint8_t *mp3_buf = malloc((size_t)size > 0 ? (size_t)size : 1);
    if (!mp3_buf) {
        io_report_error("codec_b: out of memory reading '%s'", input_path);
        fclose(in);
        return -1;
    }

    size_t mp3_size = io_read_bytes(in, mp3_buf, (size_t)size);
    fclose(in);

    mp3dec_t dec;
    mp3dec_init(&dec);

    growable_pcm_t pcm = {0};
    int frames = 0;
    size_t pos = 0;
    int ok = 1;

    while (pos < mp3_size) {
        mp3dec_frame_info_t info;
        int16_t frame_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
        int samples = mp3dec_decode_frame(&dec, mp3_buf + pos, (int)(mp3_size - pos),
                                           frame_pcm, &info);

        if (info.frame_bytes <= 0)
            break; /* not enough trailing bytes left for another frame */

        pos += (size_t)info.frame_bytes;

        if (samples > 0) {
            size_t n = (size_t)samples * (size_t)info.channels;
            if (growable_pcm_reserve(&pcm, n) < 0) {
                io_report_error("codec_b: out of memory growing decoded PCM buffer");
                ok = 0;
                break;
            }
            for (size_t i = 0; i < n; i++)
                pcm.data[pcm.count + i] = frame_pcm[i];
            pcm.count += n;
            frames++;
        }
    }

    free(mp3_buf);

    if (ok && pcm_write_all(output_path, pcm.data, pcm.count) < 0)
        ok = 0;

    free(pcm.data);

    return ok ? frames : -1;
}
