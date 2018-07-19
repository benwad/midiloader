CC=clang
CFLAGS=-I

loadmidi: loadmidi.c events.c main.c
	$(CC) -o loadmidi loadmidi.c events.c main.c

clean:
	rm loadmidi
