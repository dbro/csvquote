EXE=csvquote
EXTRA=csvheader
CFLAGS=-Wall -g

all:	$(EXE)

$(EXE):
	$(CC) $(CFLAGS) $(EXE).c -o $(EXE)

install: $(EXE) $(EXTRA)
	install -m 755 $(EXE) $(PREFIX)
	install -m 755 $(EXTRA) $(PREFIX)

clean:
	rm -f $(EXE)
