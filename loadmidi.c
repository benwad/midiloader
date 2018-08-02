/*
	loadmidi.c : 	Utilities for loading MIDI files, along with
			data structures for storing the data
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loadmidi.h"
#include "events.h"
#include "eventlist.h"
#include "util.h"


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
	unsigned int offset = 0;
	
	event->type = 0xFF;
	event->subtype = *(data + offset++);
	event->size = *(data + offset++);
	event->data = malloc(event->size);

	memcpy(event->data, (data + offset), event->size);
	offset += event->size;
	data += event->size;

	return offset;
}

unsigned int GetSysexEvent(unsigned char* data, Event* event)
{
	unsigned int offset = 0;
	event->type = *(data + offset++);
	unsigned long eventSize;
	unsigned long vlenSize = GetVLen(data + offset, &eventSize);
	offset += vlenSize;
	event->size = eventSize;
	event->data = malloc(event->size);

	memcpy(event->data, (data + offset), event->size);
	offset += event->size;

	return offset;
}

unsigned int GetMidiEvent(unsigned char* data, Event* event, unsigned char runningStatus)
{
	unsigned int offset = 0;
	unsigned char typeByte = *(data + offset);

	if (IsValidMidiEventType(typeByte)) {
		event->type = typeByte;
		++offset;
	}
	else {
		if (runningStatus) {
			event->type = runningStatus;
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


Track GetTrack( unsigned char* data )
{
	Track track;
	track.events = NULL;

	int finished = 0;
	Event* currentEvent = NULL;
	unsigned int offset = 0;
	unsigned char runningStatus = '\0';

	while (!finished) {
		unsigned long dTime;
		unsigned int vlenSize = GetVLen(data+offset, &dTime);
		offset += vlenSize;
		Event* newEvent = (Event *)malloc(sizeof(Event));
		newEvent->time = dTime;
		unsigned int eventSize = GetEvent((data + offset), newEvent, runningStatus);
		offset += eventSize;
		if (track.events == NULL) {
			track.events = (EventNode *)malloc(sizeof(EventNode));
			track.events->event = newEvent;
			track.events->next = NULL;
		}
		else {
			PushEvent(track.events, newEvent);
		}
		currentEvent = newEvent;
		runningStatus = currentEvent->type;
		if ((currentEvent->type == 0xFF) && (currentEvent->subtype == 0x2F)) {
			// End of track
			finished = 1;
			offset += 1;
		}
	}

	return track;
}


int PrintChunk(Chunk* chunk)
{
	if (strcmp((const char*)chunk->type, "MThd") == 0) {
		FileInfo *fileInfo = GetHeader(chunk);
		PrintFileInfo(fileInfo);
		return 0;
	}
	else if (strcmp((const char*)chunk->type, "MTrk") == 0) {
		Track track = GetTrack(chunk->data);
		unsigned char* trackName = GetTrackName(track);
		if (trackName) {
			printf("Track name: %s\n", trackName);
		}
		printf("Track has %i events\n", GetNumEvents(track.events));
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

	if (offset > 0) {
		unsigned int sizebuffer[1];
		offset += fread(sizebuffer, sizeof(unsigned int), 1, f);
		SwapEndianness32(sizebuffer);
		chunk->length = *sizebuffer;

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
