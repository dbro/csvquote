csvquote
========
_smart and simple CSV processing on the command line_

Dan Brown, May 2013
Jarno Suni, August 2015
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
interpretated as separators when they are inside quoted data fields.

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

Known limitations
-----------------

The program does not correctly handle multi-character delimiters, but this
is rare in CSV files. It is able to work with Windows-style line endings that
use /r/n as the record separator.

If you need to search for special characters (commas and newlines)
*within a quoted field*, then csvquote will *PROBABLY NOT* work for you. These
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

* CSV files must be *parsed sequentially* from start to end. They cannot be
split up and processed in parallel (without first scanning
them to make sure the splits would be in "safe" places).

* CSV files do not have a mechanism to recover from *corruption*. If an extra
double-quote character gets inserted into the file, then all of the remaining
data will be misinterpreted.

Another issue is that the *character encoding* of CSV files is not specified.

Digression on programming languages
===================================

I'm a novice programmer dabbling in various programming languages. When
starting to learn a new language, I try to port this program into it. This
gives me a real-world exercise and a basis for comparison. Here are a few
remarks based on this limited experience.

* *python* code is quick to write, but slow to run. The python version relies
on the built-in CSV library, but the other versions of this program do not use
any special libraries.

* *go* has some convenient improvements relative to C. In cases where runtime
speed is important, and either concurrency or modern libraries would be
helpful, this is a good choice. For this program, neither of these applied.
When thinking about dependencies, go has the benefit of static linking into an
independent binary; but it's not a widely used language so may not be suitable
for broad distribution.

* *awk* is a good fit for reading and writing text. Relatively fast to write
and run. It's available on all UNIX systems, but can be slightly different.

* *C* is fastest to run, and requires some machine-level understanding of
memory management so takes a bit more care to write the code. It's easy to do
dangerous things in this language, so a bit of guidance can be very helpful.
I believe C is more likely to be useful to other people because most systems
will be able to compile it without installing additional requirements.

Based on these reasons, this project will use the C version in the main branch
and put the other versions in a different branch for reference.

Run-time Speed comparison
-------------------------

Time spent processing a 100 MB CSV file on my laptop.

* python ~ 100 seconds
* awk ~ 14
* go ~ 1.2
* C ~ 1.0

