csvquote
========

Dan Brown, May 2013  
https://github.com/dbro/csvquote

Are you looking for a way to process CSV data with standard UNIX shell commands?

Are you running into problems with embedded commas and newlines that mess
everything up?

Do you wish there was some way to add some CSV intelligence to these UNIX tools?

* awk, sed
* cut, join
* head, tail
* sort, uniq
* wc, split

This program can be used at the start and end of a text processing pipeline
so that regular unix command line tools can properly handle CSV data that
contain commas and newlines inside quoted data fields.

Without this program, embedded special characters lead to undesirable
interpretation of commas and newlines within data fields as field and record
separators.

This program temporarily replaces the special characters inside quoted data
fields with harmless nonprinting characters that can be processed as data by
regular text tools. At the end of processing the text, these nonprinting
characters are restored to their previous values.

In short, csvquote wraps the pipeline of UNIX commands to let them work on
clean data that is consistently separated by commas and newlines.

By default, the program expects to use these as special characters:

    " quote character  
    , field delimiter  
    \n record separator  

It is possible to specify different characters for the field and record
separators, such as tabs or pipe symbols.

Note that the quote character can be contained inside a quoted field
by repeating it twice, eg.

    field1,"field2, has a comma in it","field 3 has a ""Quoted String"" in it"

Typical usage of csvquote is as part of a command line pipe, to permit
the regular unix text-manipulating commands to avoid misinterpreting
special characters found inside fields. eg.

    csvquote foobar.csv | cut -d ',' -f 5 | sort | uniq -c | csvquote -u

or taking input from stdin,

    cat foobar.csv | csvquote | cut -d ',' -f 7,4,2 | csvquote -u

other examples:

    csvquote -t foobar.tsv | wc -l

    csvquote -q "'" foobar.csv | sort -t, -k3 | csvquote -u

    csvquote foobar.csv | awk -F, '{sum+=$3} END {print sum}'

Requirements
------------

python

Known limitations
-----------------

The program does not correctly handle multi-character delimiters, but this
should not prevent it from working with Windows-style line endings.
