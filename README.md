csvquote
========
_smart and simple CSV processing on the command line_

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

Without this program, embedded special characters would be incorrectly
interpreted as separators when they are inside quoted data fields.

By using csvquote, you temporarily replace the special characters inside quoted
fields with harmless nonprinting characters that can be processed as data by
regular text tools. At the end of processing the text, these nonprinting
characters are restored to their previous values.

In short, csvquote wraps the pipeline of UNIX commands to let them work on
clean data that is consistently separated, with no ambiguous special
characters present inside the data fields.

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

How it works
------------

We can compare the hexadecimal representation of the data to see how csvquote substitutes the non-printing characters (1e and 1f) for newlines (0a) and commas (2c). The embedded special characters are shown in **bold** font below. The first command shows the original data, the second command shows how the command 'csvquote' sanitizes the delimiters contained inside quoted strings, and the third command shows how the command 'csvquote -u' restores the original data. Note that xxd uses '.' characters to represent unprintable characters in its text representation on the right side below.

<pre>
$ echo 'ab,"cd<b>,</b>ef","hi
jk"' | xxd -g 1 -c 20

00000000: 61 62 2c 22 63 64 <b>2c</b> 65 66 22 2c 22 68 69 <b>0a</b> 6a 6b 22 0a     ab,"cd<b>,</b>ef","hi<b>.</b>jk".


$ echo 'ab,"cd<b>,</b>ef","hi
jk"' | <b>csvquote</b> | xxd -g 1 -c 20

00000000: 61 62 2c 22 63 64 <b>1f</b> 65 66 22 2c 22 68 69 <b>1e</b> 6a 6b 22 0a     ab,"cd<b>.</b>ef","hi<b>.</b>jk".


$ echo 'ab,"cd<b>,</b>ef","hi
jk"' | <b>csvquote | csvquote -u</b> | xxd -g 1 -c 20

00000000: 61 62 2c 22 63 64 <b>2c</b> 65 66 22 2c 22 68 69 <b>0a</b> 6a 6b 22 0a     ab,"cd<b>,</b>ef","hi<b>.</b>jk".
</pre>

Installation
------------

    $ make
    $ sudo make install

This will install the csvquote program as well as csvheader, which is a
convenient script that prints out the field numbers next to the first row
of data.

Depends on the "build-essentials" package.

### via Homebrew (Mac, Linux)

    $ brew install sschlesier/csvutils/csvquote

Known limitations
-----------------

The program does not correctly handle multi-character delimiters, but this
is rare in CSV files. It is able to work with Windows-style line endings that
use /r/n as the record separator.

If you need to search for special characters (commas and newlines)
**within a quoted field**, then csvquote will **PROBABLY NOT** work for you. These
delimiter characters get temporarily replaced with nonprinting characters so
they would not be found. There are two options you could try:

1. Use csvquote as normal, and search for the substitute nonprinting characters
   instead of the regular delimiters.
2. Instead of using csvquote, try a csv-aware text tool such as csvfix.

Some thoughts on CSV as a data storage format
=============================================

CSV is easy to read. This is its essential advantage over other formats. Most
programs that work with data provide methods to read and write CSV data. It
can be understood by humans, which is helpful for validation and auditing.

But CSV has some challenges when used to transfer information among systems
and programs. This program exists to handle the challenge of ambiguity in CSV.
The separator characters can be used as either data (aka text) or metadata
(aka special characters) depending on whether it is quoted or not. This
context sensitivity complicates parsing, because (for example) a newline
cannot be interpreted without its context. If it is surrounded by
double-quotes then it must be handled as text, otherwise it is a separator.

To repeat: the meaning of each character depends on the entire contents of
the file up to that point. This has two negative consequences.

* CSV files must be **parsed sequentially** from start to end. They cannot be
split up and processed in parallel (without first scanning
them to make sure the splits would be in "safe" places).

* CSV files do not have a mechanism to recover from **corruption**. If an extra
double-quote character gets inserted into the file, then all of the remaining
data will be misinterpreted.

Another issue is that the **character encoding** of CSV files is not specified.

Run-time Speed comparison
=========================

How fast is csvquote? Here are some data processing rates measured when running csvquote on an Intel i7 CPU model from 2013. Due to recently introduced optimizations in csvquote, the processing speed depends on how common the quote characters are in the source data.

* 1.9 GB/sec : csvquote reading random csv data with 10% of fields quoted
* 0.5 GB/sec : csvquote reading random csv data with 100% of fields quoted
* 3.7 GB/sec : csvquote reading random csv data with no quoted fields (nothing for csvquote to do!)

A common use of csvquote is as one (or two) steps in a pipeline sequence of commands. When each command can run on a separate processor, the time to complete the overall pipeline sequence will be determined by the slowest step in the chain of dependencies. So as long as csvquote is not the slowest step in the sequence, then its relative speed will not affect the overall run time. This seems likely if some of these commands are involved:

* 3.1 GB/sec : wc -l
* 1.3 GB/sec : grep 'ZZZ'
* 1.0 GB/sec : tr 'a' 'b'
* 0.3 GB/sec : cut -f1

In January 2022, csvquote was rewritten to be approximately 10x faster than before, from optimizing for source data with at least half of its fields not quoted. The inspiration to revisit this old code came from a rewrite by [skeeto@](https://github.com/skeeto/scratch/tree/master/csvquote). His version is especially aimed at modern CPUs (Intel and AMD from about the year 2015) and runs approximately 50% faster using [AVX2 SIMD](https://en.wikipedia.org/wiki/Advanced_Vector_Extensions) instructions. When running skeeto's version on my CPU (without support for AVX2 SIMD) it processes data at a consistent pace of 0.7 GB/sec, and does not vary depending on how many quote characters are in the source data. When running on a CPU with AVX2 SIMD support, it does slow down somewhat less than this version of csvquote does as the frequency of quoted fields increases.
