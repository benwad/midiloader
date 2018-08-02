CC=clang
DEBUGGER=lldb
CFLAGS=-I

loadmidi: util.c events.c eventlist.c loadmidi_old.c loadmidi.c main.c
	$(CC) -g -o loadmidi util.c events.c eventlist.c loadmidi.c loadmidi_old.c main.c

debug: loadmidi util.c events.c eventlist.c loadmidi_old.c loadmidi.c main.c
	$(DEBUGGER) loadmidi shaft.mid -o "breakpoint set --file eventlist.c --line 8" -o "run"

clean:
	rm loadmidi
