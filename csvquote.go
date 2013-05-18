package main
 
import (
        "os"
        "io"
        "log"
        "flag"
)
 
var restoremode = flag.Bool("u", false, "restore the original separator characters")
var delimiter = flag.String("d", ",", "field separator character")
var delimitertab = flag.Bool("t", false, "use tab as field separator (overrides -d parameter)")
var quotechar = flag.String("q", "\"", "field quoting character")
var recordsep = flag.String("r", "\n", "record separator character")

var delimiterNonprintingByte byte = 31
var recordsepNonprintingByte byte = 30
var delimiterByte, quotecharByte, recordsepByte byte
 
func main() {
    flag.Parse() // Scans the arg list and sets up flags
    if *delimitertab {
        *delimiter = "\t"
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

    mapFunction := replaceOriginalChars
    if *restoremode {
        mapFunction = restoreOriginalChars
    }
    stateQuoteInEffect := false // only used in replace mode
    stateMaybeEscapedQuoteChar := false // only used in replace mode

    delimiterByte = byte((*delimiter)[0])
    quotecharByte = byte((*quotechar)[0])
    recordsepByte = byte((*recordsep)[0])

    data := make([]byte, 1024)
    for {
        if count, err := input.Read(data); err != nil {
            if err != io.EOF {
                log.Fatal(err)
            }
            break
        } else {
            for i:=0; i<count; i++ {
                data[i], stateQuoteInEffect, stateMaybeEscapedQuoteChar = 
                    mapFunction(data[i], stateQuoteInEffect, stateMaybeEscapedQuoteChar)
            }
            os.Stdout.Write(data[:count])
        }
    }
    err := input.Close()
    if err != nil {
        log.Fatal(err)
    }
}


func replaceOriginalChars(c byte, stateQuoteInEffect bool, stateMaybeEscapedQuoteChar bool) (byte, bool, bool) {
    d := c // default
    if stateMaybeEscapedQuoteChar {
        if c != quotecharByte {
            // this is the end of a quoted field
            stateQuoteInEffect = false
        }
        //d = c
        stateMaybeEscapedQuoteChar = false
    } else if stateQuoteInEffect {
        if c == quotecharByte {
            // this is either an escaped quote char or the end of a quoted
            // field. need to read one more character to decide which
            //d = c
            stateMaybeEscapedQuoteChar = true
        } else if c == delimiterByte {
            d = delimiterNonprintingByte
        } else if c == recordsepByte {
            d = recordsepNonprintingByte
        //} else {
        //    d = c
        }
    } else {
        // quote not in effect
        // d = c
        if c == quotecharByte {
            stateQuoteInEffect = true
        }
    }
    return d, stateQuoteInEffect, stateMaybeEscapedQuoteChar
}

func restoreOriginalChars(c byte, stateQuoteInEffect bool, stateMaybeEscapedQuoteChar bool) (byte, bool, bool) {
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
