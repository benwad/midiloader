/*
	loadmidi.c : 	Utilities for loading MIDI files, along with
			data structures for storing the data
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loadmidi.h"
#include "events.h"


void KeySignature_Print( struct KeySignature ks )
{
	
}

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

unsigned int GetMetaEvent(unsigned char *data, Event* event)
{
	printf("GetMetaEvent\n");
	unsigned int offset = 0;
	event->type = *(data++);
	event->size = *(data++);
	event->data = malloc(event->size);

	memcpy(event->data, data, event->size);
	offset += event->size;

	return offset;
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

unsigned int GetF0SysexEvent(unsigned char* data, Event* event)
{
	printf("GetF0SysexEvent\n");
	return 0;
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

unsigned int GetF7SysexEvent(unsigned char* data, Event* event)
{
	printf("GetF7SysexEvent\n");
	return 0;
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


unsigned long GetVLen( unsigned char* data, unsigned long* result )
{
	int done = 0;
	unsigned long val = 0;
	unsigned int offset = 0;

	while (!done)
	{
		unsigned char byte = data[offset];
		val |= byte & 0x7F;
		offset += sizeof(unsigned char);

		if (byte & 0x80) val <<= 7;
		else done = 1;
	}

	*result = val;

	return offset;
}


FileInfo* GetHeader( Chunk* chunk )
{
	unsigned short headerInfo[chunk->length / sizeof(unsigned short)];
	memcpy(headerInfo, chunk->data, chunk->length);

	for (int i=0; i < chunk->length / sizeof(unsigned short); i++ )
	{
		SwapEndianness16(&headerInfo[i]);
	}

	FileInfo* fileInfo = (FileInfo *)malloc(sizeof(FileInfo));

	fileInfo->formatType = headerInfo[0];
	fileInfo->numTracks = headerInfo[1];
	fileInfo->timeDivisionType = GetTimeDivisionType(headerInfo[2]);
	fileInfo->timeDivision = GetTimeDivision(headerInfo[2]);

	return fileInfo;
}


unsigned int GetEvent( unsigned char* data, Event* event )
{
	unsigned char eventType = *(data++);

	switch(eventType) {
		case 0xFF:
			return GetMetaEvent(data, event);
		case 0xF0:
			return GetF0SysexEvent(data, event);
		case 0xF7:
			return GetF7SysexEvent(data, event);
		default: {
			printf("Could not resolve event type: 0x%2x\n", eventType);
			return 0;
		}
	}
}


Track* GetTrack( Chunk* chunk )
{
	int finished = 0;
	Event* currentEvent = NULL;
	unsigned int offset = 0;

	while (!finished) {
		unsigned long dTime;
		offset += GetVLen(chunk->data + (offset * sizeof(unsigned char)), &dTime);
		printf("dTime: %lu\n", dTime);
		printf("offset: %i\n", offset);
		Event* newEvent = (Event *)malloc(sizeof(Event));
		unsigned int eventSize = GetEvent(chunk->data + (offset * sizeof(unsigned char)), newEvent);
		offset += eventSize;
		currentEvent = newEvent;
		PrintEvent(currentEvent);
		if ((!eventSize) || (offset >= chunk->length)) {
			finished = 1;
		}
	}

	return NULL;
}


Chunk* LoadChunk( FILE* f )
{
	Chunk* chunk = (Chunk *)malloc(sizeof(Chunk));
	chunk->type = (unsigned char *)malloc(sizeof(unsigned char)*5);
	chunk->type[4] = '\0';

	int n;
	n = fread(chunk->type, sizeof(unsigned char), 4, f);

	printf("Chunk type: %s\n", chunk->type);

	unsigned int sizebuffer[1];
	n = fread(sizebuffer, sizeof(unsigned int), 1, f);
	SwapEndianness32(sizebuffer);
	chunk->length = *sizebuffer;

	unsigned char* chunkData[chunk->length];
	chunk->data = (unsigned char*)malloc(sizeof(unsigned char)*chunk->length);
	n = fread(chunk->data, sizeof(unsigned char), chunk->length, f);

	if (strcmp((const char *)chunk->type, "MThd") == 0) {
		// Header chunk
		printf("Header chunk\n");
		FileInfo* fileInfo = GetHeader(chunk);
		PrintFileInfo(fileInfo);
	}
	else if (strcmp((const char *)chunk->type, "MTrk") == 0) {
		// Track chunk
		printf("Track chunk, size: %i\n", chunk->length);
		Track* track = GetTrack(chunk);
	}
	else {
		printf("Unknown chunk type: %s\n", chunk->type);
		return NULL;
	}

	return chunk;
}


int LoadMidiFile( const char* filename )
{
	FILE* f = fopen(filename, "rb");

	Chunk* currentChunk = NULL;
	int finished = 0;

	while (!finished) {
		currentChunk = LoadChunk(f);
		if (!currentChunk) {
			finished = 1;
		}
	}

	fclose(f);

	return 0;
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
