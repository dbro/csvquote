#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define NDEBUG
#include "dbg.h"

#define READ_BUFFER_SIZE 4096
#define NON_PRINTING_FIELD_SEPARATOR 0x1F
#define NON_PRINTING_RECORD_SEPARATOR 0x1E

/*
TODO: verify that it handles multi-byte characters and unicode and utf-8 etc
*/

int copy_file(FILE *in, const bool restore_mode,
const unsigned char del, const unsigned char quo, const unsigned char rec) {
    unsigned char buffer[READ_BUFFER_SIZE];
    size_t nbytes;
    unsigned char *c, *stopat;
    unsigned char state = 0; // 0 means unquoted, 1 means quoted
    unsigned char trans[2][256]; // 2 states, 256 possible values of char

    debug("copying file with d=%d\tq=%d\tr=%d", del, quo, rec);

    // initialize translation mapping between states
    for (int i=0; i<256; i++) {
	trans[0][i] = i;
	trans[1][i] = i;
    }
    if (restore_mode) {
        trans[0][NON_PRINTING_RECORD_SEPARATOR] = rec;
	trans[0][NON_PRINTING_FIELD_SEPARATOR] = del;
	trans[1][NON_PRINTING_RECORD_SEPARATOR] = rec;
        trans[1][NON_PRINTING_FIELD_SEPARATOR] = del;
    } else { // sanitize mode
        trans[1][rec] = NON_PRINTING_RECORD_SEPARATOR;
        trans[1][del] = NON_PRINTING_FIELD_SEPARATOR;
    }

    while ((nbytes = fread(buffer, sizeof(char), sizeof(buffer), in)) != 0)
    {
        stopat = buffer + (nbytes);
        for (c=buffer; c<stopat; c++) {
            state ^= (*c == quo); // has no effect in restore mode
            *c = trans[state][*c];
        }
        check(fwrite(buffer, sizeof(char), nbytes, stdout) == nbytes,
            "Failed to write %zu bytes\n", nbytes);
    }
    check(ferror(in) == 0, "Failed to read input\n");
    check(fflush(stdout) == 0, "Failed to flush output\n");
    return 0;

error:
    return 1;
}

int main(int argc, char *argv[]) {
    // default parameters
    FILE *input = NULL;
    unsigned char del = ',';
    unsigned char quo = '"';
    unsigned char rec = '\n';
    bool restore_mode = false;

    int opt;
    while ((opt = getopt(argc, argv, "usd:tq:r:")) != -1) {
        switch (opt) {
            case 'u':
                restore_mode = true;
                break;
            case 's':
                restore_mode = false;
                break;
            case 'd':
                del = optarg[0]; // byte
                break;
            case 't':
                del = '\t';
                break;
            case 'q':
                quo = optarg[0]; // byte
                break;
            case 'r':
                rec = optarg[0]; // byte
                break;
            case ':':
                // -d or -q or -r without operand
                fprintf(stderr,
                    "Option -%c requires an operand\n", optopt);
                goto usage;
            case '?':
                goto usage;
            default:
                fprintf(stderr,
                    "Unrecognized option: '-%c'\n", optopt);
                goto usage;
        }
    }

    // Process stdin or file names
    if (optind >= argc) {
        check(copy_file(stdin, restore_mode, del, quo, rec) == 0,
            "failed to copy from stdin");
    } else {
        // supports multiple file names
        int i;
        for (i=optind; i<argc; i++) {
            input = fopen(argv[i], "r");
            check(input != 0, "failed to open file %s", argv[optind]);
            check(copy_file(input, restore_mode, del, quo, rec) == 0,
                "failed to copy from file %s", argv[i]);
            if (input) { fclose(input); }
        }
    }

    return 0;

usage:
    fprintf(stderr, "Usage: %s [OPTION] [files]\n", argv[0]);
    fprintf(stderr, "\tfiles are zero or more filenames. If none given, read from standard input\n");
    fprintf(stderr, "\t-u\tdefault false\trestore mode. replace nonprinting characters with original characters\n");
    fprintf(stderr, "\t-d\tdefault ,\tfield separator character\n");
    fprintf(stderr, "\t-t\tdefault false\tuse tab as the field separator character\n");
    fprintf(stderr, "\t-q\tdefault \"\tfield quoting character\n");
    fprintf(stderr, "\t-r\tdefault \\n\trecord separator character\n");
    return 1;

error:
    if (input) { fclose(input); }
    return 1;
}
