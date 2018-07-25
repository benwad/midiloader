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

unsigned int GetMetaEvent(unsigned char *data, Event* event)
{
	printf("GetMetaEvent\n");
	unsigned int offset = 0;
	
	event->type = 0xFF;
	event->subtype = *(data + offset++);
	event->size = *(data + offset++);
	printf("event->type: 0x%02x\n", event->type);
	printf("event->subtype: 0x%02x\n", event->subtype);
	printf("event->size: 0x%02x\n", event->size);
	event->data = malloc(event->size);

	memcpy(event->data, (data + offset), event->size);
	offset += event->size;
	data += event->size;

	return offset;
}

unsigned int GetSysexEvent(unsigned char* data, Event* event)
{
	printf("GetSysexEvent\n");
	unsigned int offset = 0;
	event->type = *(data + offset++);
	printf("Sysex event type: %02x\n", event->type);
	unsigned long eventSize;
	unsigned long vlenSize = GetVLen(data + offset, &eventSize);
	printf("vlenSize (SysEx): %lu\n", vlenSize);
	offset += vlenSize;
	event->size = eventSize;
	event->data = malloc(event->size);

	PrintBytes(data + offset, 4);

	memcpy(event->data, (data + offset), event->size);
	offset += event->size;

	return offset;
}

unsigned int GetMidiEvent(unsigned char* data, Event* event, unsigned char runningStatus)
{
	printf("GetMidiEvent\n");
	unsigned int offset = 0;
	unsigned char typeByte = *(data + offset);

	if (IsValidMidiEventType(typeByte)) {
		event->type = typeByte;
		++offset;
	}
	else {
		if (runningStatus) {
			event->type = runningStatus;
			printf("Using running status %02x\n", runningStatus);
		}
		else {
			printf("runningStatus is null, but no valid status found\n");
		}
	}

	event->subtype = '\0';
	event->size = SizeForMidiEvent(*event);
	if (event->size == 0) {
		printf("Could not resolve MIDI event size\n");
	}

	event->data = malloc(event->size);
	memcpy(event->data, data + offset, event->size);
	offset += event->size;

	printf("MIDI event: 0x%02x\n", event->type);
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


unsigned int GetEvent( unsigned char* data, Event* event, unsigned char runningStatus )
{
	unsigned char eventType = *data;

	switch(eventType) {
		case 0xFF:
			return GetMetaEvent(data+1, event) + 1;
		case 0xF0:
		case 0xF7:
			return GetSysexEvent(data, event);
		default: {
			return GetMidiEvent(data, event, runningStatus); /* only MIDI events can have running status */
		}
	}
}


unsigned int GetTrack( unsigned char* data, Track* track )
{
	int finished = 0;
	Event* currentEvent = NULL;
	unsigned int offset = 0;
	unsigned char runningStatus = '\0';

	while (!finished) {
		printf("Track offset: %i\n", offset);
		PrintBytes(data + offset, 4);

		unsigned long dTime;
		unsigned int vlenSize = GetVLen(data+offset, &dTime);
		printf("vlenSize: %i\n", vlenSize);
		printf("dTime: %lu\n", dTime);
		offset += vlenSize;
		Event* newEvent = (Event *)malloc(sizeof(Event));
		unsigned int eventSize = GetEvent((data + offset), newEvent, runningStatus);
		offset += eventSize;
		currentEvent = newEvent;
		runningStatus = currentEvent->type;
		PrintEvent(currentEvent);
		printf("\n");
		if ((currentEvent->type == 0xFF) && (currentEvent->subtype == 0x2F)) {
			// End of track
			printf("Track ended\n");
			finished = 1;
			offset += 1;
		}
	}

	return offset;
}


int PrintChunk(Chunk* chunk)
{
	printf("Chunk type: %s, size: %i\n", chunk->type, chunk->length);
	if (strcmp((const char*)chunk->type, "MThd") == 0) {
		FileInfo *fileInfo = GetHeader(chunk);
		PrintFileInfo(fileInfo);
		return 0;
	}
	else if (strcmp((const char*)chunk->type, "MTrk") == 0) {
		Track* track = (Track *)malloc(sizeof(Track));
		unsigned int trackSize = GetTrack(chunk->data, track);
		return 0;
	}
	else {
		printf("Unknown chunk type: %s\n", chunk->type);
		return 1;
	}
}


unsigned int LoadChunk( FILE* f, Chunk* chunk)
{
	chunk->type = (unsigned char *)malloc(sizeof(unsigned char)*5);
	chunk->type[4] = '\0';

	int offset = 0;
	offset += fread(chunk->type, sizeof(unsigned char), 4, f);

	printf("Chunk type: %s\n", chunk->type);
	printf("Offset from chunk type: %i\n", offset);

	if (offset > 0) {
		unsigned int sizebuffer[1];
		offset += fread(sizebuffer, sizeof(unsigned int), 1, f);
		SwapEndianness32(sizebuffer);
		chunk->length = *sizebuffer;

		printf("Chunk length: %i\n", chunk->length);

		unsigned char* chunkData[chunk->length];
		chunk->data = (unsigned char*)malloc(sizeof(unsigned char)*chunk->length);
		offset += fread(chunk->data, sizeof(unsigned char), chunk->length, f);
	}

	return offset;
}


int LoadMidiFile( const char* filename )
{
	FILE* f = fopen(filename, "rb");

	Chunk* currentChunk = NULL;
	int finished = 0;
	unsigned int offset = 0;
	int res = 0;

	while (!finished) {
		Chunk* chunk = (Chunk *)malloc(sizeof(Chunk));
		unsigned int loadRes = LoadChunk(f, chunk);
		if (loadRes > 0) {
			offset += loadRes;
			printf("File offset: %i\n", offset);
			int res = PrintChunk(chunk);
			currentChunk = chunk;
		}
		else {
			res = 1;
		}
		if ((!currentChunk) || (res != 0)) {
			finished = 1;
		}
	}

	fclose(f);

	return 0;
}
