#include <stdio.h>
#include <string.h>

#include "codec_api.h"

/*
 * app/main.c is the single entry point shared by every codec binary.
 * It intentionally contains no codec algorithm code: it only parses
 * arguments, opens the input/output paths (delegated to the codec, which
 * uses common/io.c and common/pcm.c), calls the selected encoder or
 * decoder, prints a one-line summary, and returns success/failure.
 *
 * Usage:
 *   <binary> encode <input.pcm>  <output.bit>
 *   <binary> decode <input.bit>  <output.pcm>
 */

static void print_usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <encode|decode> <input_path> <output_path>\n", prog);
    fprintf(stderr, "  encode: <input_path> is raw s16le PCM, <output_path> is the compressed bitstream\n");
    fprintf(stderr, "  decode: <input_path> is the compressed bitstream, <output_path> is raw s16le PCM\n");
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        print_usage(argv[0]);
        return 1;
    }

    const char *mode = argv[1];
    const char *input_path = argv[2];
    const char *output_path = argv[3];

    int frames;
    if (strcmp(mode, "encode") == 0) {
        frames = encode_file(input_path, output_path);
    } else if (strcmp(mode, "decode") == 0) {
        frames = decode_file(input_path, output_path);
    } else {
        fprintf(stderr, "Unknown mode '%s'. Expected 'encode' or 'decode'.\n", mode);
        print_usage(argv[0]);
        return 1;
    }

    if (frames < 0) {
        fprintf(stderr, "%s failed for input '%s'\n", mode, input_path);
        return 1;
    }

    printf("mode=%s input=%s output=%s frames=%d\n", mode, input_path, output_path, frames);
    return 0;
}
