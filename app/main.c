#include <stdio.h>

#include "codec_api.h"

/*
 * app/main.c is the single entry point shared by every codec binary.
 * It intentionally contains no codec algorithm code: it only parses
 * arguments, calls the codec's encode_file() (which uses common/io.c
 * and common/pcm.c for the actual file I/O), prints a one-line
 * summary, and returns success/failure.
 *
 * This project only exercises/measures the encoder: decoding is
 * assumed to happen off the edge device this project profiles, so
 * there is no decode mode here (see README.md).
 *
 * Usage:
 *   <binary> <input.pcm> <output.bit>
 */

static void print_usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <input_path> <output_path>\n", prog);
    fprintf(stderr, "  input_path:  raw s16le PCM\n");
    fprintf(stderr, "  output_path: compressed bitstream\n");
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char *input_path = argv[1];
    const char *output_path = argv[2];

    int frames = encode_file(input_path, output_path);
    if (frames < 0) {
        fprintf(stderr, "encode failed for input '%s'\n", input_path);
        return 1;
    }

    printf("mode=encode input=%s output=%s frames=%d\n", input_path, output_path, frames);
    return 0;
}
