#include "pcm.h"

#include <stdlib.h>
#include <string.h>

#include "io.h"

/*
 * Byte order note: the dataset format is fixed as little-endian s16 by
 * convention (see pcm.h). We convert byte pairs to int16_t manually
 * instead of doing a raw fread() into an int16_t buffer so behavior does
 * not depend on the host's native endianness (native x86-64 and RISC-V
 * are both little-endian, but this keeps the format well-defined either
 * way and avoids any unaligned-access assumptions).
 */
static int16_t decode_s16le(const unsigned char bytes[2])
{
    uint16_t u = (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
    return (int16_t)u;
}

static void encode_s16le(int16_t sample, unsigned char bytes[2])
{
    uint16_t u = (uint16_t)sample;
    bytes[0] = (unsigned char)(u & 0xFF);
    bytes[1] = (unsigned char)((u >> 8) & 0xFF);
}

int pcm_read_all(const char *path, pcm_buffer_t *out)
{
    out->samples = NULL;
    out->count = 0;

    FILE *fp = io_open_read(path);
    if (fp == NULL) {
        return -1;
    }

    long size = io_file_size(fp);
    if (size < 0) {
        io_report_error("failed to determine size of '%s'", path);
        fclose(fp);
        return -1;
    }

    size_t sample_count = (size_t)size / 2; /* 2 bytes per s16 sample */
    if (sample_count == 0) {
        fclose(fp);
        out->samples = NULL;
        out->count = 0;
        return 0;
    }

    unsigned char *raw = (unsigned char *)malloc(sample_count * 2);
    if (raw == NULL) {
        io_report_error("out of memory reading '%s'", path);
        fclose(fp);
        return -1;
    }

    size_t got = io_read_bytes(fp, raw, sample_count * 2);
    fclose(fp);

    if (got != sample_count * 2) {
        io_report_error("short read on '%s' (expected %zu bytes, got %zu)",
                         path, sample_count * 2, got);
        free(raw);
        return -1;
    }

    int16_t *samples = (int16_t *)malloc(sample_count * sizeof(int16_t));
    if (samples == NULL) {
        io_report_error("out of memory decoding '%s'", path);
        free(raw);
        return -1;
    }

    for (size_t i = 0; i < sample_count; i++) {
        samples[i] = decode_s16le(&raw[i * 2]);
    }

    free(raw);

    out->samples = samples;
    out->count = sample_count;
    return 0;
}

void pcm_free(pcm_buffer_t *buf)
{
    if (buf == NULL) {
        return;
    }
    free(buf->samples);
    buf->samples = NULL;
    buf->count = 0;
}

int pcm_write_all(const char *path, const int16_t *samples, size_t count)
{
    FILE *fp = io_open_write(path);
    if (fp == NULL) {
        return -1;
    }

    int rc = 0;
    if (count > 0) {
        unsigned char *raw = (unsigned char *)malloc(count * 2);
        if (raw == NULL) {
            io_report_error("out of memory writing '%s'", path);
            fclose(fp);
            return -1;
        }

        for (size_t i = 0; i < count; i++) {
            encode_s16le(samples[i], &raw[i * 2]);
        }

        size_t written = io_write_bytes(fp, raw, count * 2);
        free(raw);

        if (written != count * 2) {
            io_report_error("short write on '%s' (expected %zu bytes, wrote %zu)",
                             path, count * 2, written);
            rc = -1;
        }
    }

    fclose(fp);
    return rc;
}

size_t pcm_copy_frame(const pcm_buffer_t *buf, size_t offset,
                       int16_t *frame, size_t frame_samples)
{
    size_t remaining = (offset < buf->count) ? (buf->count - offset) : 0;
    size_t real_samples = remaining < frame_samples ? remaining : frame_samples;

    if (real_samples > 0) {
        memcpy(frame, &buf->samples[offset], real_samples * sizeof(int16_t));
    }
    if (real_samples < frame_samples) {
        memset(&frame[real_samples], 0,
               (frame_samples - real_samples) * sizeof(int16_t));
    }

    return real_samples;
}

size_t pcm_frame_count(size_t sample_count, size_t frame_samples)
{
    if (frame_samples == 0) {
        return 0;
    }
    return (sample_count + frame_samples - 1) / frame_samples;
}

int16_t pcm_clip16(long value)
{
    if (value > 32767L) {
        return 32767;
    }
    if (value < -32768L) {
        return -32768;
    }
    return (int16_t)value;
}
