CC=clang
CFLAGS=-I

loadmidi: util.c events.c loadmidi_old.c loadmidi.c main.c
	$(CC) -o loadmidi util.c events.c loadmidi.c loadmidi_old.c main.c

clean:
	rm loadmidi
