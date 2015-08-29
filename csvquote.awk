#!/usr/bin/awk -f
# csvquote in awk

# Examples
#
# echo -e "a,'b\nc,d'" | csvquote.awk replace_mode=1 quotechar="'" |
# cut -d, -f2 | csvquote.awk replace_mode=0 quotechar="'"
# to return the 2nd field of the echoed string.
#
# gawk -b -f csvquote.awk a.csv
# to use GNU awk implementation explicitly to run the script on file a.csv.
# -b option speeds execution, but assumes single-byte-characters (like
# mawk by default).

BEGIN {
    # read one line at a time
    FS=""; # not used!
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

}

function suggest_replacementchar(c) {
    answer = ""; # default to pass-through, that's what empty string means
    if (maybe_escaped_quote_char) {
        if (c != quotechar) {
            # this is the end of a quoted field
            quote_in_effect = 0; # false
        }
        maybe_escaped_quote_char = 0; # false
    } else if (quote_in_effect) {
	    if (c == quotechar) {
	            # this is either an escaped quote char or the end of a quoted
	            # field. need to read one more character to decide which
	            maybe_escaped_quote_char = 1; # true
	        } else if (c == delimiter) {
	            answer = delimiter_code; # override pass-through behavior
	        } else if (c == recordsep) {
	            answer = recordsep_code; # override pass-through behavior
	        }
    } else {
        # quote not in effect
        if (c == quotechar) {
            quote_in_effect = 1; # true
        }
    }
    return answer;
}

(replace_mode) {
    # do this for every input line
    fastrack = 0; # speed optimization
    startat=1;
    if (!(quote_in_effect)) {
        quoteposition = index($0, quotechar); # index() is faster than our for loop
        if (quoteposition == 0) {
            # we don't need to scan this line. there are no quote characters in it
            fastrack = 1;
        } else {
			startat=quoteposition+1
			quote_in_effect = 1;
		}
    }
    if (fastrack) {
        print $0;
    } else {
        for (i=startat;i<=NF;i++) {
            replacementchar = suggest_replacementchar($i);
            if (replacementchar != "") {
                # we need to modify the current character
                $i=replacementchar
            }
        }
        replacementchar = suggest_replacementchar(RS);
        if (replacementchar != "") printf "%s%s", $0, replacementchar
         else printf "%s%s", $0, RS
    }
}

(!replace_mode) {
    # restore the original special characters
    # do this for every input line
    gsub(delimiter_code, delimiter);
    gsub(recordsep_code, recordsep);
    print $0
}
