#ifndef IO_H
#define IO_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/*
 * Generic, codec-agnostic file I/O helpers shared by every codec.
 * Nothing in this file knows about PCM samples, frames, or bitstreams -
 * that lives in common/pcm.h and in each codec's own encoder/decoder.
 */

/* Opens a file for binary reading. Returns NULL and reports an error on
 * failure (never aborts the process). */
FILE *io_open_read(const char *path);

/* Opens a file for binary writing (truncating any existing contents).
 * Returns NULL and reports an error on failure. */
FILE *io_open_write(const char *path);

/* Returns the size in bytes of an already-open file, or -1 on error.
 * Leaves the file position unchanged. */
long io_file_size(FILE *fp);

/* Reads up to `count` bytes into buf. Returns the number of bytes
 * actually read (may be less than count at end-of-file). */
size_t io_read_bytes(FILE *fp, void *buf, size_t count);

/* Writes `count` bytes from buf. Returns the number of bytes actually
 * written. */
size_t io_write_bytes(FILE *fp, const void *buf, size_t count);

/* Writes a formatted, prefixed message to stderr. Does not exit. */
void io_report_error(const char *fmt, ...);

#endif /* IO_H */
