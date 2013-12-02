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

typedef int (*translator) (char *);

translator translatorFactory(
bool restoremode, char *delimiter, char *quotechar, char *recordsep) {
    debug("creating translator with d=%d\tq=%d\tr=%d",
        *delimiter, *quotechar, *recordsep);
    if (restoremode) {
        int trans(char *c) {
            debug("attempting restoration of %c", *c);
            switch (*c) {
                case NON_PRINTING_FIELD_SEPARATOR:
                    *c = *delimiter; //TODO: this is causing segfault
                    break;
                case NON_PRINTING_RECORD_SEPARATOR:
                    *c = '-';//recordsep;
                    break;
                // no default case needed
            }
            return 0;
        }
        debug("returning translator");
        return &trans;
    } else {
        // sanitizing mode
        int trans(char *c) {
            static bool isQuoteInEffect = false;
            static bool isMaybeEscapedQuoteChar = false;

            debug("attempting sanitizing of %c", *c);
            //if (new_c != c) {
            //    // buffer is both input and output. avoid unnecessary changes
            //    buffer[i] = new_c;
            //}
            return 0;
        }
        debug("returning translator");
        return &trans;
    }
}

int copy_file(FILE *in, FILE *out, translator trans)
{
    char buffer[READ_BUFFER_SIZE];
    size_t nbytes;
    char *c;
    int i;

    while ((nbytes = fread(buffer, sizeof(char), sizeof(buffer), in)) != 0)
    {
        for (i=0; i<nbytes; i++) {
            c = &buffer[i];
            debug("attempting translation of %c", *c);
            check((*trans)(c) == 0,
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
    debug("Started main");

    char del = ',';
    char quo = '"';
    char rec = '\n';
    translator trans = translatorFactory(true, &del, &quo, &rec);

    debug("starting to copy data ...");
    check(copy_file(stdin, stdout, trans) == 0, "copy_file failed.");
    debug("Completed main");
    return 0;

error:
    return 1;
}
