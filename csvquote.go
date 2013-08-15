package main

import (
	"flag"
	"fmt"
	"io"
	"log"
	"os"
)

const delimiterNonprintingByte, recordsepNonprintingByte byte = 31, 30

type mapper func(byte, bool, bool) (byte, bool, bool)

func main() {
	headermode := flag.Bool("h", false, "print the index of each element in the first row then quit")
	restoremode := flag.Bool("u", false, "restore the original separator characters")
	delimiter := flag.String("d", ",", "field separator character")
	delimitertab := flag.Bool("t", false, "use tab as field separator (overrides -d parameter)")
	quotechar := flag.String("q", "\"", "field quoting character")
	recordsep := flag.String("r", "\n", "record separator character")
	flag.Parse() // Scans the arg list and sets up flags
	if *delimitertab {
		*delimiter = "\t"
	}
	delimiterByte := byte((*delimiter)[0])
	quotecharByte := byte((*quotechar)[0])
	recordsepByte := byte((*recordsep)[0])
	mapFunction := substituteNonprintingChars(delimiterByte, quotecharByte, recordsepByte)
	if *restoremode && !(*headermode) {
		mapFunction = restoreOriginalChars(delimiterByte, recordsepByte)
	}

	var input *os.File
	if flag.NArg() > 0 {
		var err error
		input, err = os.Open(flag.Arg(0))
		if err != nil {
			log.Fatal(err)
		}
	} else {
		input = os.Stdin
	}

	data := make([]byte, 4096)
	stateQuoteInEffect := false
	stateMaybeEscapedQuoteChar := false

	if *headermode {
		fieldindex := 0
		fields := [][]byte{{}}
		datacharmapped := byte('0')
		_ = datacharmapped // why is this necessary and faster than using _ later?
outerloop:
		for {
			if count, err := input.Read(data); err != nil {
				if err != io.EOF {
					log.Fatal(err)
				}
				break
			} else {
				for i := 0; i < count; i++ {
					datacharmapped, stateQuoteInEffect, stateMaybeEscapedQuoteChar =
						mapFunction(data[i], stateQuoteInEffect, stateMaybeEscapedQuoteChar)
					if fieldindex >= len(fields) {
						newfield := make([]byte, 0)
						fields = append(fields, newfield)
					}
					if !(stateQuoteInEffect) && (data[i] == recordsepByte) {
						// this is the end of the header row
						break outerloop
					} else if !(stateQuoteInEffect) && (data[i] == delimiterByte) {
						fieldindex++
					} else {
						fields[fieldindex] = append(fields[fieldindex], data[i])
					}
				}
			}
		}
		for i := 0; i < len(fields); i++ {
			fmt.Printf(" %d\t: %s\n", (i + 1), fields[i])
		}
	} else {
		// replace mode
		for {
			if count, err := input.Read(data); err != nil {
				if err != io.EOF {
					log.Fatal(err)
				}
				break
			} else {
				for i := 0; i < count; i++ {
					data[i], stateQuoteInEffect, stateMaybeEscapedQuoteChar =
						mapFunction(data[i], stateQuoteInEffect, stateMaybeEscapedQuoteChar)
				}
				os.Stdout.Write(data[:count])
			}
		}
	}

	err := input.Close()
	if err != nil {
		log.Fatal(err)
	}
}

func substituteNonprintingChars(delimiterByte byte, quotecharByte byte, recordsepByte byte) mapper {
	return func(c byte, stateQuoteInEffect bool, stateMaybeEscapedQuoteChar bool) (byte, bool, bool) {
		d := c // default
		if stateMaybeEscapedQuoteChar {
			if c != quotecharByte {
				// this is the end of a quoted field
				stateQuoteInEffect = false
			}
			stateMaybeEscapedQuoteChar = false
		} else if stateQuoteInEffect {
			switch c {
			case quotecharByte:
				// this is either an escaped quote char or the end of a quoted
				// field. need to read one more character to decide which
				stateMaybeEscapedQuoteChar = true
			case delimiterByte:
				d = delimiterNonprintingByte
			case recordsepByte:
				d = recordsepNonprintingByte
			}
		} else {
			// quote not in effect
			if c == quotecharByte {
				stateQuoteInEffect = true
			}
		}
		return d, stateQuoteInEffect, stateMaybeEscapedQuoteChar
	}
}

func restoreOriginalChars(delimiterByte byte, recordsepByte byte) mapper {
	return func(c byte, stateQuoteInEffect bool, stateMaybeEscapedQuoteChar bool) (byte, bool, bool) {
		// need to have same input/output parameters as replaceOriginalChars()
		// so the state variables are included but not used
		switch c {
		case delimiterNonprintingByte:
			return delimiterByte, false, false
		case recordsepNonprintingByte:
			return recordsepByte, false, false
		}
		return c, false, false
	}
}
