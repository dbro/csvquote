EXE=csvquote
CFLAGS=-Wall -g

BINDIR=/usr/local/bin

all:	$(EXE)

$(EXE):
	$(CC) $(CFLAGS) $(EXE).c -o $(EXE)

install: $(EXE)
	install -m 755 $(EXE) $(BINDIR)

clean:
	rm -f $(EXE)
