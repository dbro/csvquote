package main

import (
	"flag"
	"fmt"
	"io"
	"log"
	"os"
)

const delimiterNonprintingByte, recordsepNonprintingByte byte = 31, 30

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

	fieldindex := 0                     // only used in header mode
	fields := [][]byte{{}}              // only used in header mode
	stateQuoteInEffect := false         // only used in replace mode
	stateMaybeEscapedQuoteChar := false // only used in replace mode
	data := make([]byte, 1024)
outerloop:
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
			if *headermode {
				for j := 0; j < count; j++ {
					if fieldindex >= len(fields) {
						newfield := make([]byte, 0)
						fields = append(fields, newfield)
					}
					switch data[j] {
					case recordsepByte:
						// this is the end of the header row
						break outerloop
					case delimiterByte:
						fieldindex++
					case delimiterNonprintingByte:
						fields[fieldindex] = append(fields[fieldindex], delimiterByte)
					case recordsepNonprintingByte:
						fields[fieldindex] = append(fields[fieldindex], recordsepByte)
					default:
						fields[fieldindex] = append(fields[fieldindex], data[j])
					}
				}
			} else {
				os.Stdout.Write(data[:count])
			}
		}
	}
	err := input.Close()
	if err != nil {
		log.Fatal(err)
	}
	if *headermode {
		for i := 0; i < len(fields); i++ {
			fmt.Printf(" %d\t: %s\n", (i + 1), fields[i])
		}
	}
}

func substituteNonprintingChars(delimiterByte byte, quotecharByte byte, recordsepByte byte) func(byte, bool, bool) (byte, bool, bool) {
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

func restoreOriginalChars(delimiterByte byte, recordsepByte byte) func(byte, bool, bool) (byte, bool, bool) {
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
