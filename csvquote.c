#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define NDEBUG
#include "dbg.h"

#define READ_BUFFER_SIZE (64 * 1024)
#define NON_PRINTING_FIELD_SEPARATOR 0x1F
#define NON_PRINTING_RECORD_SEPARATOR 0x1E

int copy_file(FILE *in, const bool restore_mode,
const unsigned char del, const unsigned char quo, const unsigned char rec) {
    unsigned char buffer[READ_BUFFER_SIZE];
    size_t nbytes;
    unsigned char *c, *q_next, *q_start, *q_end, *stopat;
    unsigned char trans[256]; // 256 possible values of char

    // initialize translation mapping to/from sanitized csv format
    for (int i=0; i<256; i++) {
	trans[i] = i;
    }
    if (restore_mode) {
	trans[NON_PRINTING_RECORD_SEPARATOR] = rec;
        trans[NON_PRINTING_FIELD_SEPARATOR] = del;
    } else { // sanitize mode
        trans[rec] = NON_PRINTING_RECORD_SEPARATOR;
        trans[del] = NON_PRINTING_FIELD_SEPARATOR;
    }

    q_next = q_start = q_end = NULL;

    // read chunks from the input file
    while ((nbytes = fread(buffer, sizeof(char), sizeof(buffer), in)) != 0)
    {
        c = buffer;
        stopat = buffer + (nbytes);
	// scan for quote characters in the chunk
	while (c < stopat) {
	    q_next = (unsigned char *) memchr(c, quo, stopat - c);
            if (q_start == NULL) { // unquoted state
		if (q_next == NULL) { // opening quote not yet found
		    //c = stopat; // done with this chunk
		    break;
                } else { // opening quote found
		    q_start = q_next;
		    c = q_start + 1;
		    if (c == stopat) {
		        q_start = buffer - 1;
			break;
	            }
		}
	    } else { // quoted state
                c = q_start + 1; // starting pointer for translation
		if (q_next == NULL) { // closing quote not yet found
	            q_end = stopat;
		    // retain quoted state into the next chunk(s) to be read
		    q_start = buffer - 1; // continue translation at the start of the next chunk
		} else { // closing quote found
		    q_end = q_next;
		    // resume unquoted state after the translation
		    q_start = NULL;
		}
		// translate the characters inside the quoted string
                for (; c<q_end; c++) { // starting value of c already set above
                    *c = trans[*c];
	        }
		c = q_end + 1;
	    }
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
    while ((opt = getopt(argc, argv, "usd:tq:r:b")) != -1) {
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
            case 'b':
                setvbuf (stdout, NULL, _IOLBF, 0);
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
    fprintf(stderr, "\t-b\tdefault false\tuse line buffering for output (slower)\n");
    return 1;

error:
    if (input) { fclose(input); }
    return 1;
}
