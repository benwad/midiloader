#include <stdio.h>

#include "loadmidi.h"
#include "loadmidi_old.h"


int main( int argc, char* argv[] )
{
	if (argc < 2)	// For testing: Super Mario Bros theme!
		/*LoadMidiFile("/Users/bwadsworth/Documents/Dev/Snippets/midiloader/SMB1-Theme.mid");*/
		printf("Usage: ./loadmidi <filename>\n");
	else
		printf("======= OLD CODE ======\n");
		LoadMidiFileOld(argv[1]);
		printf("======= NEW CODE ======\n");
		LoadMidiFile(argv[1]);

	return 0;
}
