#include <stdlib.h>

#include "codec.h"
#include "codec_api.h"
#include "io.h"
#include "pcm.h"
#include "vendor/shine/layer3.h"

/*
 * codec_b encoder: an adapter around the vendored `shine` fixed-point
 * MP3 encoder (see codec.h and vendor/README.md for the full design
 * rationale). This file owns none of the actual encoding algorithm -
 * it only:
 *
 *   1. Loads the whole input PCM file (common/pcm.h).
 *   2. Configures and initializes a shine encoder for
 *      CODEC_B_SAMPLE_RATE/CODEC_B_BITRATE_KBPS mono.
 *   3. Feeds shine one CODEC_B_FRAME_SAMPLES-sample frame at a time
 *      (via pcm_copy_frame(), which zero-pads the final partial
 *      frame), writing out whatever encoded bytes shine hands back
 *      after each frame.
 *   4. Flushes shine's internal bit reservoir at the end and writes
 *      the remaining bytes.
 *
 * The output file is a raw MP3 elementary stream (just concatenated
 * MPEG frames, no extra header) - see codec.h for why no wrapper
 * format is needed.
 */
int encode_file(const char *input_path, const char *output_path)
{
    pcm_buffer_t pcm = {0};
    if (pcm_read_all(input_path, &pcm) < 0) {
        io_report_error("codec_b: failed to read PCM input '%s'", input_path);
        return -1;
    }

    FILE *out = io_open_write(output_path);
    if (!out) {
        pcm_free(&pcm);
        return -1;
    }

    shine_config_t config;
    shine_set_config_mpeg_defaults(&config.mpeg);
    config.wave.samplerate = CODEC_B_SAMPLE_RATE;
    config.wave.channels = PCM_MONO;
    config.mpeg.mode = MONO;
    config.mpeg.bitr = CODEC_B_BITRATE_KBPS;

    if (shine_check_config(config.wave.samplerate, config.mpeg.bitr) < 0) {
        io_report_error("codec_b: unsupported samplerate/bitrate configuration "
                         "(%d Hz, %d kbps)",
                         config.wave.samplerate, config.mpeg.bitr);
        fclose(out);
        pcm_free(&pcm);
        return -1;
    }

    shine_t s = shine_initialise(&config);
    if (!s) {
        io_report_error("codec_b: shine_initialise() failed");
        fclose(out);
        pcm_free(&pcm);
        return -1;
    }

    int frame_samples = shine_samples_per_pass(s);
    int16_t *frame = malloc((size_t)frame_samples * sizeof(int16_t));
    if (!frame) {
        io_report_error("codec_b: out of memory allocating encode frame buffer");
        shine_close(s);
        fclose(out);
        pcm_free(&pcm);
        return -1;
    }

    size_t nframes = pcm_frame_count(pcm.count, (size_t)frame_samples);
    int ok = 1;

    for (size_t i = 0; i < nframes; i++) {
        pcm_copy_frame(&pcm, i * (size_t)frame_samples, frame, (size_t)frame_samples);

        int16_t *channels[1] = {frame};
        int written = 0;
        unsigned char *data = shine_encode_buffer(s, channels, &written);
        if (written > 0 && io_write_bytes(out, data, (size_t)written) != (size_t)written) {
            io_report_error("codec_b: short write encoding frame %zu", i);
            ok = 0;
            break;
        }
    }

    if (ok) {
        int written = 0;
        unsigned char *data = shine_flush(s, &written);
        if (written > 0 && io_write_bytes(out, data, (size_t)written) != (size_t)written) {
            io_report_error("codec_b: short write flushing encoder");
            ok = 0;
        }
    }

    free(frame);
    shine_close(s);
    fclose(out);
    pcm_free(&pcm);

    return ok ? (int)nframes : -1;
}
