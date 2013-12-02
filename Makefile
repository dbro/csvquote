EXE=csvquote
CFLAGS=-Wall -g

all:	$(EXE)

$(EXE):
	$(CC) $(CFLAGS) $(EXE).c -o $(EXE)

clean:
	rm -f $(EXE)
