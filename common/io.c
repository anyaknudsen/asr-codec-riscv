#include "io.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

FILE *io_open_read(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        io_report_error("failed to open '%s' for reading: %s", path, strerror(errno));
    }
    return fp;
}

FILE *io_open_write(const char *path)
{
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        io_report_error("failed to open '%s' for writing: %s", path, strerror(errno));
    }
    return fp;
}

long io_file_size(FILE *fp)
{
    long saved_pos = ftell(fp);
    if (saved_pos < 0) {
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        return -1;
    }

    long size = ftell(fp);

    if (fseek(fp, saved_pos, SEEK_SET) != 0) {
        return -1;
    }

    return size;
}

size_t io_read_bytes(FILE *fp, void *buf, size_t count)
{
    return fread(buf, 1, count, fp);
}

size_t io_write_bytes(FILE *fp, const void *buf, size_t count)
{
    return fwrite(buf, 1, count, fp);
}

void io_report_error(const char *fmt, ...)
{
    va_list args;

    fprintf(stderr, "[io] error: ");

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}
