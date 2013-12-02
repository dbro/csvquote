#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
//#include <string.h>
#include "dbg.h"

#define READ_BUFFER_SIZE 4096
#define NON_PRINTING_FIELD_SEPARATOR 0x1F
#define NON_PRINTING_RECORD_SEPARATOR 0x1E

/*
TODO: handle multi-byte characters and unicode and utf-8 etc
TODO: parse options
*/

typedef int (*translator)(const char, const char, const char, char *);

int restore(const char delimiter, const char quotechar, const char recordsep, char *c) {
    // the quotechar is not needed when restoring, but we include it
    // to keep the function parameters consistent for both translators
    //debug("attempting restoration of %c", *c);
    switch (*c) {
        case NON_PRINTING_FIELD_SEPARATOR:
            *c = delimiter;
            break;
        case NON_PRINTING_RECORD_SEPARATOR:
            *c = recordsep;
            break;
        // no default case needed
    }
    return 0;
}

int sanitize(const char delimiter, const char quotechar, const char recordsep, char *c) {
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
    return 0;
}

int copy_file(FILE *in, FILE *out, const translator trans, const char del, const char quo, const char rec)
{
    char buffer[READ_BUFFER_SIZE];
    size_t nbytes;
    char *c, *stopat;

    while ((nbytes = fread(buffer, sizeof(char), sizeof(buffer), in)) != 0)
    {
        stopat = buffer + (nbytes);
        for (c=buffer; c<stopat; c++) {
            //debug("attempting translation of %c", *c);
            check((*trans)(del, quo, rec, c) == 0,
                "translator returned unexpected result.");
        }
        check(fwrite(buffer, sizeof(char), nbytes, out) == nbytes,
            "Failed to write %zu bytes\n", nbytes);
    }
    return 0;

error:
    return 1;
}

int main(int argc, char *argv[])
{
    //debug("Started main");
    // default parameters
    char del = ',';
    char quo = '"';
    char rec = '\n';
    bool restoremode = false;
    bool headermode = false; // supercedes other parameters

    translator trans = sanitize; // default
    if (restoremode) {
        trans = restore;
    }

    //debug("starting to copy data ...");
    check(copy_file(stdin, stdout, trans, del, quo, rec) == 0, "copy_file failed.");
    //debug("Completed main");
    return 0;

error:
    return 1;
}
