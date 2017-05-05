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

typedef void (*translator)(const char, const char, const char, const char, char *);
typedef enum { RESTORE_MODE, SANITIZE_MODE } operation_mode;

void restore(const char delimiter, const char quotechar, const char recordsep,
const char escchar, char *c) {
    // the quotechar and escchar are not needed when restoring, but we include
    // them to keep the function parameters consistent for both translators
    switch (*c) {
        case NON_PRINTING_FIELD_SEPARATOR:
            *c = delimiter;
            break;
        case NON_PRINTING_RECORD_SEPARATOR:
            *c = recordsep;
            break;
        // no default case needed
    }
    return;
}

void sanitize(const char delimiter, const char quotechar, const char recordsep,
const char escchar, char *c) {
    // maintain the state of quoting inside this function
    // this is OK because we need to read the file
    // sequentially (not in parallel) because the state
    // at any point depends on all of the previous data
    static bool isQuoteInEffect = false;
    static bool isMaybeEscapedQuoteChar = false;
    static bool isEscapeInEffect = false;

    if (isMaybeEscapedQuoteChar) {
        if (*c != quotechar) {
            // this is the end of a quoted field
            isQuoteInEffect = false;
        }
        isMaybeEscapedQuoteChar = false;
    } else if (isQuoteInEffect) {
        if (*c == quotechar && !isEscapeInEffect) {
            // this is either an escaped quote char or the end of a quoted
            // field. need to read one more character to decide which
            isMaybeEscapedQuoteChar = true;
        } else if (*c == delimiter) {
            *c = NON_PRINTING_FIELD_SEPARATOR;
        } else if (*c == recordsep) {
            *c = NON_PRINTING_RECORD_SEPARATOR;
        }
    } else {
        // quote not in effect and escape not in effect
        if (*c == quotechar && !isEscapeInEffect) {
            isQuoteInEffect = true;
        }
    }
    if (*c == escchar && !isEscapeInEffect) {
        isEscapeInEffect = true;
    } else {
        isEscapeInEffect = false;
    }
    return;
}

int copy_file(FILE *in, const operation_mode op_mode,
const char del, const char quo, const char rec, const char esc) {
    char buffer[READ_BUFFER_SIZE];
    size_t nbytes;
    char *c, *stopat;

    debug("copying file with d=%d\tq=%d\tr=%d\te=%d", del, quo, rec, esc);

    translator trans;
    switch (op_mode) {
        case SANITIZE_MODE:
            trans = sanitize;
            break;
        case RESTORE_MODE:
            trans = restore;
            break;
        default:
            sentinel("unexpected operating mode");
    }

    while ((nbytes = fread(buffer, sizeof(char), sizeof(buffer), in)) != 0)
    {
        stopat = buffer + (nbytes);
        for (c=buffer; c<stopat; c++) {
            (*trans)(del, quo, rec, esc, c); // no error checking inside this loop
        }
        check(fwrite(buffer, sizeof(char), nbytes, stdout) == nbytes,
            "Failed to write %zu bytes\n", nbytes);
    }
    return 0;

error:
    return 1;
}

int main(int argc, char *argv[]) {
    // default parameters
    FILE *input = NULL;
    char del = ',';
    char quo = '"';
    char rec = '\n';
    char esc = '\\';
    operation_mode op_mode = SANITIZE_MODE;

    int opt;
    while ((opt = getopt(argc, argv, "usd:tq:r:")) != -1) {
        switch (opt) {
            case 'u':
                op_mode = RESTORE_MODE;
                break;
            case 's':
                op_mode = SANITIZE_MODE;
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
            case 'e':
                esc = optarg[0]; // byte
                break;
            case ':':
                // -d or -q or -r or -e without operand
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
        check(copy_file(stdin, op_mode, del, quo, rec, esc) == 0,
            "failed to copy from stdin");
    } else {
        // supports multiple file names
        int i;
        for (i=optind; i<argc; i++) {
            input = fopen(argv[i], "r");
            check(input != 0, "failed to open file %s", argv[optind]);
            check(copy_file(input, op_mode, del, quo, rec, esc) == 0,
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
    fprintf(stderr, "\t-e\tdefault \\\tescape character\n");
    return 1;

error:
    if (input) { fclose(input); }
    return 1;
}
