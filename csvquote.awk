#!/usr/bin/awk -f
# csvquote in awk
# TODO assign the special characters using command line flags
# TODO add header mode

BEGIN {
    # read one line at a time
    FS="\033"; # not used!
    OFS=FS;
    delimiter=",";
    recordsep="\n";
    quotechar="\"";
    delimiter_code="\035";
    recordsep_code="\034";
    replace_mode = 1; # true. if restore mode then 0
    # these are volatile global variables
    quote_in_effect = 0; # false
    maybe_escaped_quote_char = 0; # false
    writebuf = "";
}

function replace_special_chars(c) {
    if (maybe_escaped_quote_char) {
        if (c != quotechar) {
            # this is the end of a quoted field
            quote_in_effect = 0; # false
        }
        writebuf = writebuf c; # concat
        maybe_escaped_quote_char = 0; # false
    } else if (quote_in_effect) {
        if (c == quotechar) {
            # this is either an escaped quote char or the end of a quoted
            # field. need to read one more character to decide which
            writebuf = writebuf c; # concat
            maybe_escaped_quote_char = 1; # true
        } else if (c == delimiter) {
            writebuf = writebuf delimiter_code; # concat
        } else if (c == recordsep) {
            writebuf = writebuf recordsep_code; # concat
        } else {
            writebuf = writebuf c; # concat
        }
    } else {
        # quote not in effect
        writebuf = writebuf c; # concat
        if (c == quotechar) {
            quote_in_effect = 1; # true
        }
    }
}

(replace_mode) {
    # do this for every input line
    fastrack = 0; # speed optimization
    if (!(quote_in_effect)) {
        quoteposition = index($0, quotechar); # index() is faster than our for loop
        if (quoteposition == 0) {
            # we don't need to scan this line. there are no quote characters in it
            fastrack = 1;
        }
    }
    if (fastrack) {
        print writebuf $0;
        writebuf = "";
    } else {
        # otherwise we scan the line for quote characters
        numchars = length($0);
        for (i=1;i<=numchars;i++) {
            replace_special_chars(substr($0,i,1));
        }
        printf("%s", writebuf);
        writebuf = "";
        replace_special_chars(RS);
    }
}

(!replace_mode) {
    # restore the original special characters
    # do this for every input line
    writebuf = $0;
    sub(/\035/, delimiter, writebuf); # Beware ! delimitercode repeated
    sub(/\034/, recordsep, writebuf); # Beware ! recordsepcode repeated
    print writebuf;
}

END {
    printf("%s", writebuf); #flush
}
