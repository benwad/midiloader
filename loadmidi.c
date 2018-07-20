/*
	loadmidi.c : 	Utilities for loading MIDI files, along with
			data structures for storing the data
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loadmidi.h"
#include "events.h"
#include "util.h"


void KeySignature_Print( struct KeySignature ks )
{
	
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

unsigned int GetMetaEvent(unsigned char *data, Event* event)
{
	printf("GetMetaEvent\n");
	unsigned int offset = 0;
	event->type = *(data + offset++);
	event->size = *(data + offset++);
	event->data = malloc(event->size);

	memcpy(event->data, (data + offset), event->size);
	offset += event->size;
	data += event->size;

	return offset;
}

unsigned int GetF0SysexEvent(unsigned char* data, Event* event)
{
	printf("GetF0SysexEvent\n");
	unsigned int offset = 0;
	/*event->type = *(data + offset++);*/
	event->type = 0xF0;
	event->size = *(data + offset++);
	event->data = malloc(event->size);

	memcpy(event->data, (data + offset), event->size);
	offset += event->size;
	data += event->size;

	return offset;
}

unsigned int GetF7SysexEvent(unsigned char* data, Event* event)
{
	printf("GetF7SysexEvent\n");
	return 0;
}

unsigned int GetMidiEvent(unsigned char* data, Event* event)
{
	printf("GetMidiEvent\n");
	unsigned int offset = 0;
	unsigned char status = *(data + offset);
	printf("MIDI event status byte = 0x%2x\n", status);
	return offset;
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
	unsigned char eventType = *data;

	switch(eventType) {
		case 0xFF:
			return GetMetaEvent(data+1, event) + 1;
		case 0xF0:
			return GetF0SysexEvent(data+1, event) + 1;
		case 0xF7:
			return GetF7SysexEvent(data+1, event) + 1;
		default: {
			return GetMidiEvent(data, event) + 1;
		}
	}
}


unsigned int GetTrack( unsigned char* data, Track* track )
{
	int finished = 0;
	Event* currentEvent = NULL;
	unsigned int offset = 0;

	while (!finished) {
		printf("Next 4 bytes: 0x%2x%2x%2x%2x\n",
			(unsigned char)*(data+offset),
			(unsigned char)*(data+offset+1),
			(unsigned char)*(data+offset+2),
			(unsigned char)*(data+offset+3)
		);
		unsigned long dTime;
		unsigned int vlenSize = GetVLen(data+offset, &dTime);
		printf("vlenSize: %i\n", vlenSize);
		printf("dTime: %lu\n", dTime);
		offset += vlenSize;
		Event* newEvent = (Event *)malloc(sizeof(Event));
		unsigned int eventSize = GetEvent((data + offset), newEvent);
		/*printf("Event size: %i\n", eventSize);*/
		/*printf("Event size: %i\n", newEvent->size);*/
		offset += eventSize;
		currentEvent = newEvent;
		PrintEvent(currentEvent);
		printf("\n");
		if (currentEvent->type == 0x2F) {
			// End of track
			finished = 1;
			offset += 1;
		}
	}

	return offset;
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
		Track track;
		unsigned int trackSize = GetTrack(chunk->data, &track);
		printf("Offset created by track: %i\n", trackSize);
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
