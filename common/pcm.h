#ifndef PCM_H
#define PCM_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/*
 * PCM-specific helpers shared by every codec.
 *
 * The dataset is raw, headerless, signed 16-bit little-endian PCM
 * (mono). This matches files produced by:
 *
 *   ffmpeg -i input.wav -ac 1 -ar 16000 -f s16le -acodec pcm_s16le out.pcm
 *
 * or by scripts/gen_dataset.py for synthetic test inputs.
 */

typedef struct {
    int16_t *samples;
    size_t count; /* number of int16_t samples */
} pcm_buffer_t;

/* Reads an entire raw s16le PCM file into memory. Returns 0 on success
 * and a negative value on failure. On success, the caller owns
 * out->samples and must free it with pcm_free(). */
int pcm_read_all(const char *path, pcm_buffer_t *out);

/* Frees the buffer allocated by pcm_read_all(). Safe to call on an
 * already-freed / zeroed buffer. */
void pcm_free(pcm_buffer_t *buf);

/* Writes `count` signed 16-bit samples to `path` as raw s16le PCM.
 * Returns 0 on success and a negative value on failure. */
int pcm_write_all(const char *path, const int16_t *samples, size_t count);

/* Copies up to `frame_samples` samples starting at `offset` from `buf`
 * into `frame`. If the source buffer runs out before `frame_samples`
 * samples have been copied, the remainder of `frame` is zero-padded
 * (silence), which is how the last, possibly-incomplete frame of a file
 * is handled. Returns the number of *real* (non-padded) samples copied,
 * which is <= frame_samples. */
size_t pcm_copy_frame(const pcm_buffer_t *buf, size_t offset,
                       int16_t *frame, size_t frame_samples);

/* Returns the number of frames (including a final partial frame) needed
 * to cover `sample_count` samples at `frame_samples` samples/frame. */
size_t pcm_frame_count(size_t sample_count, size_t frame_samples);

/* Clamps a wide integer sample value into the representable int16_t
 * range. Speech codec prediction/reconstruction math frequently
 * produces intermediate values outside [-32768, 32767]; this centralizes
 * the saturation policy so every codec clips identically. */
int16_t pcm_clip16(long value);

#endif /* PCM_H */
