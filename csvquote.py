#!/usr/bin/python
# coding=utf-8
# Dan Brown
# http://github.com/dbro/csvquote
# Note: The code characters \035 and \036 should never appear in the data
# test with this command:
# ./csvquote testdata.csv | ./csvquote -u | cmp - testdata.csv

USAGE = """csvquote 
    Intended to be used at the start and end of a text processing pipeline
    so that regular unix command line tools can properly handle data
    formatted as CSV that contains commas and newlines inside the data fields.
    Without this program, embedded special characters prevents usage of
    programs like head, tail, and cut.

    accepts input from stdin or a filename; output sent to stdout

    Usage: 
    Default special characters are
    " quote character
    , field delimiter
    \\n record separator

    note that the quote character can be contained inside a quoted field
    by repeating it twice, eg.
    field1,"field2, has a comma in it","field 3 has a ""Quoted String"" in it"

    typical usage is expected to be as part of a command line pipe, to permit
    the regular unix text-manipulating commands to avoid misinterpreting
    special characters found inside fields. eg.
    cat foobar.csv | csvquote | cut -d ',' -f 7,4,2 | csvquote -u
    csvquote foobar.csv | head -n 100 | csvquote -u

    csvquote foobar.csv
    replace the field and line delimiter special characters that appear inside 
    quoted fields in foobar.csv with nonprinting characters
    
    csvquote -u foobar.csv
    restore the field and line delimiter special characters that appear inside 
    quoted fields in foobar.csv with regular printing characters

    csvquote -q "'" -d "|" -r "\r\n" foobar.csv
    replace the pipe and CRLF special characters that appear inside 
    single-quoted fields in foobar.csv with nonprinting characters.
 
    csvquote -u -t foobar.tsv
    restore the field and line delimiter special characters that appear inside 
    foobar.tsv with regular printing characters, with tab as the field delimiter
"""

class Error(Exception):
    """Base class for exceptions in this module."""
    pass

class InputError(Error):
    """Exception raised for errors in the input.

    Attributes:
        expr -- input expression in which the error occurred
        msg  -- explanation of the error
    """

    def __init__(self, expr, msg):
        self.expr = expr
        self.msg = msg


# Should use something better than getopt, but this works...
import sys, csv, getopt
opts, args = getopt.getopt(sys.argv[1:], "d:q:r:ut?", [])
 
delimiter = ','
recordsep = '\n'
quotechar = '"'
delimiter_code = '\035'
recordsep_code = '\034'
show_usage = False
replace_mode = True
 
if opts:
    opts = dict(opts)
    show_usage = ('-?' in opts) or ('--help' in opts)
    if '-t' in opts:
        delimiter = "\t"
    elif '-d' in opts:
        delimiter = opts['-d']
        if len(delimiter) != 1:
            raise InputError(delimiter, "field delimiter must be exactly one character")
    if '-q' in opts:
        quotechar = opts['-q']
        if len(quotechar) != 1:
            raise InputError(quotechar, "quote character must be exactly one character")
    if '-r' in opts:
        recordsep = opts['-r']
        if len(recordsep) != 1:
            raise InputError(recordsep, "record separator must be exactly one character")
    if '-u' in opts:
        replace_mode = False
 
if show_usage:
    print USAGE
    sys.exit()

if args:
    i = open(args[0],"rb")
else:
    i = sys.stdin

BATCH_READ_COUNT = 1024 # TODO: would increasing this improve speed?

def replace_special_chars(existing_state, c):
    quote_in_effect = existing_state[0]
    maybe_escaped_quote_char = existing_state[1]
    writebuf = existing_state[2]
    if maybe_escaped_quote_char:
        if (c != quotechar):
            # this is the end of a quoted field
            quote_in_effect = False
        writebuf += c
        maybe_escaped_quote_char = False
    elif quote_in_effect:
        if (c == quotechar):
            # this is either an escaped quote char or the end of a quoted
            # field. need to read one more character to decide which
            writebuf += c
            maybe_escaped_quote_char = True
        elif (c == delimiter):
            writebuf += delimiter_code
        elif (c == recordsep):
            writebuf += recordsep_code
        else:
            writebuf += c
    else:
        # quote not in effect
        writebuf += c
        if (c == quotechar):
            quote_in_effect = True
    return (quote_in_effect, maybe_escaped_quote_char, writebuf)

if replace_mode:
    # the program is replacing quoted special chars with nonprinting chars
    quote_in_effect = False
    maybe_escaped_quote_char = False
    cs = i.read(BATCH_READ_COUNT)
    while cs != "":
        quote_in_effect, maybe_escaped_quote_char, writebuf = \
            reduce(replace_special_chars, cs, (quote_in_effect, maybe_escaped_quote_char, ""))
        sys.stdout.write(writebuf)
        cs = i.read(BATCH_READ_COUNT)
else:
    # the program is restoring the special characters
    # we assume that the code characters never appear in the original text
    #for row in i:
    #    sys.stdout.write(row.replace(delimiter_code, delimiter).replace(recordsep_code, recordsep))
    cs = i.read(BATCH_READ_COUNT)
    writebuf = ""
    while cs != "":
        for c in cs:
            if (c == delimiter_code):
                writebuf += delimiter
            elif (c == recordsep_code):
                writebuf += recordsep
            else:
                writebuf += c
        sys.stdout.write(writebuf)
        writebuf = ""
        cs = i.read(BATCH_READ_COUNT)

i.close() # TODO: prefer "with ..." syntax?
