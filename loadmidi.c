/*
	loadmidi.c : 	Utilities for loading MIDI files, along with
			data structures for storing the data
*/

#include "stdio.h"
#include "loadmidi.h"


enum TimeDivType {
	ticksPerBeat = 0,
	framesPerSecond = 1,
};

struct FramesPerSecond {
	unsigned short smpteFrames;
	unsigned short ticksPerFrame;
} FramesPerSecond;

struct TimeSignature {
	unsigned short numerator;
	unsigned short denominator;
	unsigned short clocksPerClick;
	unsigned short notesPerQuarterNote;
} TimeSignature;

struct KeySignature {
	signed short sf;
	signed short mi;
} KeySignature;

void KeySignature_Print( struct KeySignature ks )
{

}

union TimeDivision {
	struct FramesPerSecond framesPerSecond;
	unsigned short ticksPerBeat;
} TimeDivision;

struct FileInfo {
	unsigned short formatType;
	unsigned short numTracks;
	enum TimeDivType timeDivisionType;
	union TimeDivision timeDivision;
} FileInfo;

void SwapEndianness32( unsigned int *num )
{
	unsigned int swapped = ((*num>>24) & 0x000000ff) |
				((*num<<8) & 0xff000000) |
				((*num>>8) & 0xff000000) |
				((*num<<24) & 0xff000000);

	*num = swapped;
}

void SwapEndianness16( unsigned short *num )
{
	unsigned int swapped = (*num >> 8) | (*num << 8);

	*num = swapped;
}

enum TimeDivType GetTimeDivisionType( unsigned short timeDivData )
{
	if (timeDivData & 0x8000)
		return framesPerSecond;
	else
		return ticksPerBeat;
}

void PrintCharBuffer( unsigned char* buffer, int size )
{
	printf("Buffer: ");
	for (int i=0; i < size; i++)
	{
		printf("%c", buffer[i]);
	}
	printf("\n");
}

union TimeDivision GetTimeDivision( unsigned short tDivData )
{
	enum TimeDivType timeDivisionType = GetTimeDivisionType(tDivData);
	union TimeDivision timeDivision;

	if (timeDivisionType == framesPerSecond)
	{
		struct FramesPerSecond fpsData;
		fpsData.smpteFrames = (tDivData & 0x7F00) >> 8;
		fpsData.ticksPerFrame = (tDivData & 0x00FF);

		timeDivision.framesPerSecond = fpsData;
	}
	else if (timeDivisionType == ticksPerBeat)
	{
		unsigned short numTicks = (tDivData & 0x7FFF);

		timeDivision.ticksPerBeat = numTicks;
	}

	return timeDivision;
}

unsigned long ReadVarLen( FILE* f )
{
	unsigned long value;
	unsigned char c;

	if ((value = fgetc(f)) & 0x80)
	{
		value &= 0x7F;
		do
		{
			value = (value << 7) + ((c = getc(f)) & 0x7F);
		} while (c & 0x80);
	}

	return value;
}

void GetMetaEvent(FILE* f)
{
	unsigned char eventType = fgetc(f);

	if (eventType == 0x03) // Track/sequence name
	{
		unsigned char size = fgetc(f);

		unsigned char buffer[size];

		if (!fread(buffer, size, 1, f))
		{
			printf("Error reading file!\n");
			return;
		}

		printf("Track/sequence name: %s\n", buffer);
	}
	else if (eventType == 0x04) // Instrument name
	{
		unsigned char size = fgetc(f);

		unsigned char buffer[size];

		if (!fread(buffer, size, 1, f))
		{
			printf("Error reading file!\n");
			return;
		}

		printf("Instrument name: %s\n", buffer);
	}
	else if (eventType == 0x05) // Lyrics
	{
		unsigned char size = fgetc(f);

		unsigned char buffer[size];

		if (!fread(buffer, size, 1, f))
		{
			printf("Error reading file!\n");
			return;
		}

		printf("Lyrics:\n");
		printf("%s\n", buffer);
	}
	else if (eventType == 0x51) // Set tempo
	{
		unsigned char size = fgetc(f); // always 3
		if (size != 0x03)
			printf("Expecting 0x03, got 0x%2x\n", size);

		unsigned char meBuffer[3];
		int res = fread( meBuffer, sizeof(unsigned char)*3, 1, f );

		unsigned int bufferVal = (meBuffer[2] +
									(meBuffer[1] << 8) + 
									(meBuffer[0] << 16));

		/*printf("SetTempo data: 0x%2x%2x%2x\n", meBuffer[0], meBuffer[1], meBuffer[2]);*/

		unsigned int msPerMin = 60000000;
		unsigned int bpm = msPerMin / bufferVal;
		printf("bpm: %u\n", bpm);
	}
	else if (eventType == 0x58)	// Time signature
	{
		unsigned char size = fgetc(f);

		if (size != 0x04)
			printf("Time signature size: expecting 0x04, got 0x%2x\n", size);

		unsigned char buffer[size];

		if (!fread(buffer, size, 1, f))
		{
			printf("Error reading file!\n");
			return;
		}

		struct TimeSignature ts;
		ts.numerator = (unsigned short)buffer[0];
		ts.denominator = (unsigned short)buffer[1];
		ts.clocksPerClick = (unsigned short)buffer[2];
		ts.notesPerQuarterNote = (unsigned short)buffer[3];

		unsigned int actualDenominator = 1;

		for (int i=0; i < ts.denominator; ++i)	// calculate actual denominator from exponent
		{
			actualDenominator *= 2;
		}

		printf("Time signature: %u/%u (%u clocks per click, %u notes per 1/4 note)\n", ts.numerator, actualDenominator, ts.clocksPerClick, ts.notesPerQuarterNote );

	}
	else if (eventType == 0x59)	// Key signature
	{
		unsigned char size = fgetc(f);

		if (size != 0x02)
			printf("Key signature size: expecting 0x02, got 0x%2x\n", size);

		unsigned char buffer[size];

		if (!fread(buffer, size, 1, f))
		{
			printf("Error reading Meta event!\n");
			return;
		}

		struct KeySignature ks;
		ks.sf = (signed short)buffer[0];
		ks.mi = (signed short)buffer[1];

		printf("Key signature: 0x%2x, 0x%2x\n", ks.sf, ks.mi);
	}
	else if (eventType == 0x2F) // End of track
	{
		printf("End of track!\n");

		fgetc(f);
	}
	else if (eventType == 0x7F)	// Sequencer-specific meta-event
	{
		printf("Sequencer-specific (no default behaviour)\n");
	}
	
	else
	{
		printf("MetaEvent %2x not found\n", eventType);
	}
}

void GetF0SysexEvent(FILE* f)
{
	unsigned char size = fgetc(f);

	unsigned char buffer[size];

	if (!fread(buffer, size, 1, f))
	{
		printf("Error reading F0 Sysex event!\n");
		return;
	}

	printf("Sysex data:\n");
	printf("%s\n", buffer);
}

void GetF7SysexEvent(FILE* f)
{
	unsigned char size = fgetc(f);

	unsigned char buffer[size];

	if (!fread(buffer, size, 1, f))
	{
		printf("Error reading F7 Sysex event!\n");
		return;
	}

	printf("'Any' data:\n");
	printf("%s\n", buffer);
}

unsigned long GetVLen( FILE* f )
{
	int done = 0;
	unsigned long val = 0;

	printf("VLen data: 0x");

	while (!done)
	{
		unsigned char byte = fgetc(f);
		printf("%2x", byte);
		val |= byte & 0x7F;

		if (byte & 0x80) val <<= 7;
		else done = 1;
	}

	printf("\n");

	return val;
}

int LoadMidiFile( const char* filename )
{
	FILE* f = fopen(filename, "rb");

	unsigned char buffer[sizeof(unsigned char)*4];
	int n;

	if (!f)
	{
		printf("Error opening file!\n");
		return 1;
	}

	n = fread(buffer, sizeof(unsigned char)*4, 1, f);

	printf("Result: %i\n", n);
	PrintCharBuffer(buffer, 4);

	unsigned int sizebuffer[1];

	n = fread(sizebuffer, sizeof(unsigned int), 1, f);

	SwapEndianness32(sizebuffer);

	unsigned short headerInfo[*sizebuffer / sizeof(unsigned short)];
	n = fread(headerInfo, *sizebuffer, 1, f);

	for (int i=0; i < *sizebuffer / sizeof(unsigned short); i++ )
	{
		SwapEndianness16(&headerInfo[i]);
	}

	struct FileInfo fileInfo;

	fileInfo.formatType = headerInfo[0];
	fileInfo.numTracks = headerInfo[1];
	fileInfo.timeDivisionType = GetTimeDivisionType(headerInfo[2]);
	fileInfo.timeDivision = GetTimeDivision(headerInfo[2]);

	printf("Format type: %i\n", fileInfo.formatType);
	printf("Num tracks: %i\n", fileInfo.numTracks);

	if (fileInfo.timeDivision.ticksPerBeat)
		printf("Time division: %i ticks per beat\n", fileInfo.timeDivision.ticksPerBeat);
	else
		printf("Time division: %i SMPTE frames per second, %i ticks per frame", fileInfo.timeDivision.framesPerSecond.smpteFrames,
											fileInfo.timeDivision.framesPerSecond.ticksPerFrame);

	n = fread(buffer, sizeof(unsigned char)*4, 1, f);

	PrintCharBuffer(buffer, 4);

	n = fread(sizebuffer, sizeof(unsigned int), 1, f);

	SwapEndianness32(sizebuffer);

	printf("Track data size: %u\n", *sizebuffer);

	for (int i=0; i < *sizebuffer; ++i)
	{
		printf("Event dTime: %lu\n", GetVLen(f));

		unsigned char theByte = fgetc(f);
		/*printf("0x%2x\n", theByte);*/

		if (theByte == 0xFF)
			GetMetaEvent(f);
		else if (theByte == 0xF0)
			GetF0SysexEvent(f);
		else if (theByte == 0xF7)
			GetF7SysexEvent(f);
		else {
			printf("Could not resolve event type: 0x%2x\n", theByte);
		}

		// if (!(theByte & 0x80))
		// 	printf("End...\n");
		// else if (theByte == 0xFF)
		// 	GetMetaEvent(f);
	}

	fclose(f);

	return 0;

}
