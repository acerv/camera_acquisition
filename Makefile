CFLAGS = -Wall
INFILES = camera.c main.c
OUTFILE = tester

all:
	$(CC) $(INFILES) -o $(OUTFILE) $(CFLAGS)

debug:
	$(CC) $(INFILES) -o $(OUTFILE) $(CFLAGS) -g

clean:
	rm -f $(OUTFILE)
