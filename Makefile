EXE=csvquote
EXTRA=csvheader
CFLAGS=-Wall -g -O3

BINDIR?=/usr/local/bin

all:	$(EXE)

$(EXE):
	$(CC) $(CFLAGS) $(EXE).c -o $(EXE)

install: $(EXE) $(EXTRA)
	install -m 755 $(EXE) $(BINDIR)
	install -m 755 $(EXTRA) $(BINDIR)

clean:
	rm -f $(EXE)

test:
	@./$(EXE) testdata.csv > output.testdata.csv.temp
	@cmp output.testdata.csv output.testdata.csv.temp || echo "sanitize test failed"
	@./$(EXE) testdata.csv | ./$(EXE) -u > testdata.csv.temp
	@cmp testdata.csv testdata.csv.temp || echo "round-trip test failed"
	@rm output.testdata.csv.temp testdata.csv.temp
