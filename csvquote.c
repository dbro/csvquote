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

typedef void (*translator)(const char, const char, const char, char *);
typedef enum { HEADER_MODE, RESTORE_MODE, SANITIZE_MODE } operation_mode;

void restore(const char delimiter, const char quotechar, const char recordsep, char *c) {
    // the quotechar is not needed when restoring, but we include it
    // to keep the function parameters consistent for both translators
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

void sanitize(const char delimiter, const char quotechar, const char recordsep, char *c) {
    // maintain the state of quoting inside this function
    // this is OK because we need to read the file
    // sequentially (not in parallel) because the state
    // at any point depends on all of the previous data
    static bool isQuoteInEffect = false;
    static bool isMaybeEscapedQuoteChar = false;

    if (isMaybeEscapedQuoteChar) {
        if (*c != quotechar) {
            // this is the end of a quoted field
            isQuoteInEffect = false;
        }
        isMaybeEscapedQuoteChar = false;
    } else if (isQuoteInEffect) {
        if (*c == quotechar) {
            // this is either an escaped quote char or the end of a quoted
            // field. need to read one more character to decide which
            isMaybeEscapedQuoteChar = true;
        } else if (*c == delimiter) {
            *c = NON_PRINTING_FIELD_SEPARATOR;
        } else if (*c == recordsep) {
            *c = NON_PRINTING_RECORD_SEPARATOR;
        }
    } else {
        // quote not in effect
        if (*c == quotechar) {
            isQuoteInEffect = true;
        }
    }
    return;
}

int copy_file(FILE *in, const operation_mode op_mode,
const char del, const char quo, const char rec) {
    char buffer[READ_BUFFER_SIZE];
    size_t nbytes;
    char *c, *stopat;

    debug("copying file with d=%d\tq=%d\tr=%d", del, quo, rec);

    translator trans;
    switch (op_mode) {
        case SANITIZE_MODE:
            trans = sanitize;
            break;
        case RESTORE_MODE:
            trans = restore;
            break;
        //case HEADER_MODE:
        //    sentinel("unexpected operating mode");
        default:
            sentinel("unexpected operating mode");
    }

    while ((nbytes = fread(buffer, sizeof(char), sizeof(buffer), in)) != 0)
    {
        stopat = buffer + (nbytes);
        for (c=buffer; c<stopat; c++) {
            (*trans)(del, quo, rec, c); // no error checking inside this loop
        }
        check(fwrite(buffer, sizeof(char), nbytes, stdout) == nbytes,
            "Failed to write %zu bytes\n", nbytes);
    }
    return 0;

error:
    return 1;
}

void create_param(char *tempbuf, char *prefix, const char c) {
    // creates shell-friendly strings to assemble into a system command
    strcpy(tempbuf, prefix);
    switch (c) {
        case '\"':
            strcat(tempbuf, "\'\"\'");
            break;
        case '\'':
            strcat(tempbuf, "\"\'\"");
            break;
        case '\n':
            strcat(tempbuf, "\'\n\'");
            break;
        case '\r':
            strcat(tempbuf, "\'\r\'");
            break;
        default:
            strcat(tempbuf, "\'");
            strncat(tempbuf, &c, 1);
            strcat(tempbuf, "\'");
    }
    return;
}

int main(int argc, char *argv[]) {
    // default parameters
    FILE *input = NULL;
    char del = ',';
    char quo = '"';
    char rec = '\n';
    operation_mode op_mode = SANITIZE_MODE;

    int opt;
    while ((opt = getopt(argc, argv, "husd:tq:r:")) != -1) {
        switch (opt) {
            case 'h':
                op_mode = HEADER_MODE;
                break;
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

    if (op_mode == HEADER_MODE) {
        // assemble a shell command. assumes presence of "head" "tr" "nl"
        // TODO: remove dependency on shell commands. implement this in C instead.
        char header_cmd[4096];
        char *tempbuf = malloc(sizeof(char) * 128);
        check(tempbuf, "unable to allocate memory for tempbuf");

        sprintf(header_cmd, "%s -s", argv[0]);
        if (del == '\t') {
            strcat(header_cmd, " -t");
        } else {
            create_param(tempbuf, " -d", del);
            strcat(header_cmd, tempbuf);
        }
        create_param(tempbuf, " -q", quo);
        strcat(header_cmd, tempbuf);
        create_param(tempbuf, " -r", rec);
        strcat(header_cmd, tempbuf);
        if (optind < argc) {
            strcat(header_cmd, " ");
            strcat(header_cmd, argv[optind]);
        }
        strcat(header_cmd, " | head -n 1 | tr");
        if (del == '\t') {
            strcat(header_cmd, " '\t' '\n' | nl");
        } else {
            create_param(tempbuf, " ", del);
            strcat(header_cmd, tempbuf);
            strcat(header_cmd, " '\n' | nl");
        }
        debug("header mode. running command %s", header_cmd);
        check(system(header_cmd) == 0, "error running header system command %s", header_cmd);
        return 0; // done
    }

    // Process stdin or file names
    if (optind >= argc) {
        check(copy_file(stdin, op_mode, del, quo, rec) == 0,
            "failed to copy from stdin");
    } else {
        // supports multiple file names
        int i;
        for (i=optind; i<argc; i++) {
            input = fopen(argv[i], "r");
            check(input != 0, "failed to open file %s", argv[optind]);
            check(copy_file(input, op_mode, del, quo, rec) == 0,
                "failed to copy from file %s", argv[i]);
            if (input) { fclose(input); }
        }
    }

    return 0;

usage:
    fprintf(stderr, "Usage: %s [-hud:tq:r:] [file]\n", argv[0]);
    return 1;

error:
    if (input) { fclose(input); }
    return 1;
}
