CC=clang
CFLAGS=-I

loadmidi: loadmidi.c main.c
	$(CC) -o loadmidi loadmidi.c main.c

clean:
	rm loadmidi
