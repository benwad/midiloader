#include <stdio.h>

#include "loadmidi.h"


int main( int argc, char* argv[] )
{
	if (argc < 2)	// For testing: Super Mario Bros theme!
		printf("Usage: ./loadmidi <filename>\n");
	else
		LoadMidiFile(argv[1]);

	return 0;
}
