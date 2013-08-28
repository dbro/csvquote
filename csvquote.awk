#!/usr/bin/awk -f
# csvquote in awk
# TODO assign the special characters using command line flags
# TODO add header mode

BEGIN {
    # read one line at a time
    FS="";
    OFS=FS;
    delimiter=",";
    recordsep="\n";
    quotechar="\"";
    delimiter_code="\035";
    recordsep_code="\034";
    replace_mode = 1; # true. if restore mode then 0
    # existing_state is a global variable
    existing_state[1] = 0; # false
    existing_state[2] = 0; # false
    existing_state[3] = ""; # empty buffer
}

function replace_special_chars(c) {
    # existing_state is a global variable. it could be passed in as an array
    # (pass-by-ref) but we still need to persist its state from one input
    # line to the next and it needs to be altered by this function, so
    # can't be simple variables
    quote_in_effect = existing_state[1];
    maybe_escaped_quote_char = existing_state[2];
    writebuf = existing_state[3];

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

    existing_state[1] = quote_in_effect;
    existing_state[2] = maybe_escaped_quote_char;
    existing_state[3] = writebuf;
}

function restore_special_chars(c) {
    writebuf = existing_state[3];
    if (c == delimiter_code) {
        writebuf = writebuf delimiter; # concat
    } else if (c == recordsep_code) {
        writebuf = writebuf recordsep; # concat
    } else {
        writebuf = writebuf c; # concat
    }
    existing_state[3] = writebuf;
}

(replace_mode) {
    # do this for every input line
    for (i=1;i<=NF;i++) {
        replace_special_chars($i);
    }
    printf("%s", existing_state[3]);
    existing_state[3] = "";
    replace_special_chars(RS);
}

(!replace_mode) {
    # restore the original special characters
    # do this for every input line
    for (i=1;i<=NF;i++) {
        restore_special_chars($i);
    }
    printf("%s", existing_state[3]);
    existing_state[3] = "";
    restore_special_chars(RS);
}

END {
    printf("%s", existing_state[3]); #flush
}
