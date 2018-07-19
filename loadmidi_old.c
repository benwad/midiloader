#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loadmidi_old.h"
#include "events.h"
#include "util.h"


void PrintCharBuffer( unsigned char* buffer, int size )
{
	printf("Buffer: ");
	for (int i=0; i < size; i++)
	{
		printf("%c", buffer[i]);
	}
	printf("\n");
}


Event* GetMetaEventFile(FILE* f)
{
	Event* event = (Event *)malloc(sizeof(Event));
	event->type = fgetc(f);
	event->size = fgetc(f);
	event->data = malloc(event->size);

	printf("GetMetaEvent: 0x%2x\n", event->type);
	printf("Size: %i\n", event->size);

	if (event->size > 0) {
		if (!fread(event->data, sizeof(unsigned char), event->size, f))
		{
			printf("Error reading file!\n");
			return NULL;
		}
	}

	return event;
}

Event* GetF0SysexEventFile(FILE* f)
{
	printf("GetF0SysexEvent\n");

	unsigned int size = fgetc(f);

	unsigned char* buffer = (unsigned char *)malloc(size);

	if (!fread(buffer, size, 1, f))
	{
		printf("Error reading F0 Sysex event!\n");
		free(buffer);
		return NULL;
	}

	printf("Sysex data:\n");
	printf("%s\n", buffer);

	Event* event = (Event *)malloc(sizeof(Event));
	event->type = 0xF0;
	event->size = (unsigned int)size;
	event->data = buffer;

	return event;
}

Event* GetF7SysexEventFile(FILE* f)
{
	printf("GetF7SysexEvent\n");

	unsigned int size = fgetc(f);

	unsigned char* buffer = (unsigned char *)malloc(size);

	if (!fread(buffer, size, 1, f))
	{
		printf("Error reading F7 Sysex event!\n");
		free(buffer);
		return NULL;
	}

	printf("'Any' data:\n");
	printf("%s\n", buffer);

	Event* event = (Event *)malloc(sizeof(Event));
	event->type = 0xF7;
	event->size = (unsigned int)size;
	event->data = buffer;

	return event;
}

unsigned long GetVLenFile( FILE* f )
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

int LoadMidiFileOld( const char* filename )
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

	FileInfo fileInfo;

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

	int finished = 0;

	while (!finished) {
		n = fread(buffer, sizeof(unsigned char)*4, 1, f);
		if (!n) {
			finished = 1;
			break;
		}

		PrintCharBuffer(buffer, 4);

		n = fread(sizebuffer, sizeof(unsigned int), 1, f);

		SwapEndianness32(sizebuffer);

		printf("Track data size: %u\n", *sizebuffer);

		for (int i=0; i < *sizebuffer; ++i)
		{
			printf("Event dTime: %lu\n", GetVLenFile(f));

			unsigned char theByte = fgetc(f);

			Event* event = NULL;

			if (theByte == 0xFF)
				event = GetMetaEventFile(f);
			else if (theByte == 0xF0)
				event = GetF0SysexEventFile(f);
			else if (theByte == 0xF7)
				event = GetF7SysexEventFile(f);
			else {
				printf("Could not resolve event type: 0x%2x. Exiting...\n", theByte);
				return 1;
			}

			PrintEvent(event);

			// if (!(theByte & 0x80))
			// 	printf("End...\n");
			// else if (theByte == 0xFF)
			// 	GetMetaEvent(f);
		}
	}

	fclose(f);

	return 0;
}
