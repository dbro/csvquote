EXE=csvquote
EXTRA=csvheader
CFLAGS=-Wall -g

BINDIR?=/usr/local/bin

all:	$(EXE)

$(EXE):
	$(CC) $(CFLAGS) $(EXE).c -o $(EXE)

install: $(EXE) $(EXTRA)
	install -m 755 $(EXE) $(BINDIR)
	install -m 755 $(EXTRA) $(BINDIR)

clean:
	rm -f $(EXE)
