#include "events.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


unsigned int GetTempoBPM(unsigned char* buffer) {
	unsigned int bufferVal =	(buffer[2] +
								(buffer[1] << 8) + 
								(buffer[0] << 16));

	unsigned int msPerMin = 60000000;
	unsigned int bpm = msPerMin / bufferVal;

	return bpm;
}


struct TimeSignature GetTimeSignature(unsigned char* buffer) {
	struct TimeSignature ts;
	ts.numerator = (unsigned short)buffer[0];
	ts.denominator = (unsigned short)buffer[1];
	ts.clocksPerClick = (unsigned short)buffer[2];
	ts.notesPerQuarterNote = (unsigned short)buffer[3];

	return ts;
}


struct KeySignature GetKeySignature(unsigned char* buffer) {
	struct KeySignature ks;
	ks.sf = (signed short)buffer[0];
	ks.mi = (signed short)buffer[1];

	return ks;
}


void PrintFileInfo(FileInfo* fileInfo)
{
	printf("Format type: %i\n", fileInfo->formatType);
	printf("Num tracks: %i\n", fileInfo->numTracks);

	if (fileInfo->timeDivision.ticksPerBeat)
		printf(
			"Time division: %i ticks per beat\n",
			fileInfo->timeDivision.ticksPerBeat
		);
	else
		printf(
			"Time division: %i SMPTE frames per second, %i ticks per frame\n",
			fileInfo->timeDivision.framesPerSecond.smpteFrames,
			fileInfo->timeDivision.framesPerSecond.ticksPerFrame
		);
}


void PrintSMPTEOffset(unsigned char* data)
{
	unsigned int hours = (unsigned int)data[0];
	unsigned int minutes = (unsigned int)data[1];
	unsigned int seconds = (unsigned int)data[2];
	unsigned int frames = (unsigned int)data[3];
	unsigned int subframes = (unsigned int)data[4];

	printf("%uh%um%us + %u frames, %u 1/100 frames\n",
		hours, minutes, seconds, frames, subframes);
}


void PrintMetaEvent(Event* event)
{
	switch(event->subtype) {
		case 0x03: // Track name
		{
			unsigned char* name = malloc(event->size + 1);
			memcpy(name, event->data, event->size);
			name[event->size] = '\0';

			printf("Track name: %s\n", name);
			return;
		}
		case 0x04: // Instrument name
		{
			unsigned char* name = malloc(event->size + 1);
			name[event->size] = '\0';
			memcpy(name, event->data, event->size);

			printf("Instrument name: %s\n", name);
			return;
		}
		case 0x05: // Lyrics
		{
			unsigned char* lyrics = malloc(event->size + 1);
			lyrics[event->size] = '\0';
			memcpy(lyrics, event->data, event->size);

			printf("Lyrics: %s\n", lyrics);
			return;
		}
		case 0x51: // Set tempo
		{
			unsigned int bpm = GetTempoBPM(event->data);
			printf("Set tempo: %u BPM\n", bpm);
			return;
		}
		case 0x54: // SMPTE offset
		{
			printf("SMPTE offset\n");
			PrintSMPTEOffset(event->data);
			return;
		}
		case 0x58: // Set time signature
		{
			struct TimeSignature ts = GetTimeSignature(event->data);

			unsigned int actualDenominator = 1;

			for (int i=0; i < ts.denominator; ++i)	// calculate actual denominator from exponent
			{
				actualDenominator *= 2;
			}

			printf("Time signature: %u/%u (%u clocks per click, %u notes per 1/4 note)\n", ts.numerator, actualDenominator, ts.clocksPerClick, ts.notesPerQuarterNote );
			return;
		}
		case 0x59: // Set key signature
		{
			struct KeySignature ks = GetKeySignature(event->data);
			printf("Key signature: 0x%2x, 0x%2x\n", ks.sf, ks.mi);
			return;
		}
		case 0x2F: // End of track
		{
			printf("End of track!\n");
			return;
		}
		case 0x7F: // Sequencer-specific meta-event
		{
			printf("Sequencer-specific (no default behaviour)\n");
			return;
		}
		case 0x21: // MIDI Port
		{
			printf("MIDI port %i\n", (unsigned int)(*(event->data)));
			return;
		}
		default:
		{
			printf("MetaEvent 0x%2x not found\n", event->subtype);
			return;
		}
	}
}


unsigned char* GetTrackName(Track track)
{
	EventNode* current = track.events;
	while (current) {
		if ((current->event->type == 0xFF) && (current->event->subtype == 0x03)) {
			unsigned char* name = malloc(current->event->size + 1);
			memcpy(name, current->event->data, current->event->size);
			name[current->event->size] = '\0';
			return name;
		}

		current = current->next;
	}

	return NULL;
}


void PrintSysexEvent(Event* event)
{
	printf("%02x SysEx event: 0x", event->type);
	for (int i=0; i < event->size; i++)
	{
		printf("%02x", *(event->data + i));
	}

	printf("\n");
}


void PrintNoteOffEvent(Event event)
{
	unsigned int midiChannel = (unsigned int)(event.type & 0x0F);
	unsigned int key = (unsigned int)event.data[0];
	unsigned int velocity = (unsigned int)event.data[1];
	printf("Note off. Channel: %i, key: %i, velocity: %i\n",
		midiChannel,
		key,
		velocity
	);
}


void PrintNoteOnEvent(Event event)
{
	unsigned int midiChannel = (unsigned int)(event.type & 0x0F);
	unsigned int key = (unsigned int)event.data[0];
	unsigned int velocity = (unsigned int)event.data[1];
	printf("Note on. Channel: %i, key: %i, velocity: %i\n",
		midiChannel,
		key,
		velocity
	);
}


void PrintPolyphonicAftertouchEvent(Event event)
{
	unsigned int midiChannel = (unsigned int)(event.type & 0x0F);
	unsigned int key = (unsigned int)event.data[0];
	unsigned int pressure = (unsigned int)event.data[1];
	printf("Polyphonic aftertouch. Channel: %i, key: %i, pressure: %i\n",
		midiChannel,
		key,
		pressure
	);
}


void PrintControllerChangeEvent(Event event)
{
	unsigned int midiChannel = (unsigned int)(event.type & 0x0F);
	unsigned int controller = (unsigned int)event.data[0];
	unsigned int value = (unsigned int)event.data[1];
	printf("Controller change. Channel: %i, controller: %i, value: %i\n",
		midiChannel,
		controller,
		value
	);
}


void PrintProgramChangeEvent(Event event)
{
	unsigned int midiChannel = (unsigned int)(event.type & 0x0F);
	unsigned int program = (unsigned int)event.data[0];
	printf("Program change. Channel: %i, program: %i\n",
		midiChannel,
		program
	);
}


void PrintChannelAftertouchEvent(Event event)
{
	unsigned int midiChannel = (unsigned int)(event.type & 0x0F);
	unsigned int pressure = (unsigned int)event.data[0];
	printf("Channel aftertouch. Channel: %i, pressure: %i\n",
		midiChannel,
		pressure
	);
}


void PrintPitchBendEvent(Event event)
{
	unsigned int midiChannel = (unsigned int)(event.type & 0x0F);
	unsigned int lsb = (unsigned int)event.data[0];
	unsigned int msb = (unsigned int)event.data[1];
	printf("Pitch bend. Channel: %i, lsb: %i, msb: %i\n",
		midiChannel,
		lsb,
		msb
	);
}


void PrintMidiEvent(Event* event)
{
	unsigned char statusBits = (event->type & 0xF0);
	switch (statusBits) {
		case 0x80: {
			PrintNoteOffEvent(*event);
			break;
		}
		case 0x90: {
			PrintNoteOnEvent(*event);
			break;
		}
		case 0xA0: {
			PrintPolyphonicAftertouchEvent(*event);
			break;
		}
		case 0xB0: {
			PrintControllerChangeEvent(*event);
			break;
		}
		case 0xC0: {
			PrintProgramChangeEvent(*event);
			break;
		}
		case 0xD0: {
			PrintChannelAftertouchEvent(*event);
			break;
		}
		case 0xE0: {
			PrintPitchBendEvent(*event);
			break;
		}
		default: {
			printf("Unknown MIDI event type: %02x\n", event->type);
			break;
		}
	}
}


int IsValidMidiEventType(unsigned char typeByte) {
	unsigned char statusBits = (typeByte & 0xF0);
	unsigned char validStatuses[7] = {0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0};
	for (int i=0; i < 7; i++) {
		if (statusBits == validStatuses[i]) {
			return 1;
		}
	}

	return 0;
}


unsigned int SizeForMidiEvent(Event event)
{
	/* First 4 bits of event type are the 'status' */
	unsigned char statusBits = (event.type & 0xF0);

	switch (statusBits) {
		case 0x80:
			return 2;
		case 0x90:
			return 2;
		case 0xA0:
			return 2;
		case 0xB0:
			return 2;
		case 0xC0:
			return 1;
		case 0xD0:
			return 1;
		case 0xE0:
			return 2;
		default:
			printf("Can't find size for MIDI event type %02x\n", event.type);
			return 0;
	}
}


void PrintEvent(Event* event) {
	printf("dTime: %lu\n", event->time);
	switch(event->type) {
		case 0xFF:
			return PrintMetaEvent(event);
		case 0xF0:
		case 0xF7:
			return PrintSysexEvent(event);
		default: {
			return PrintMidiEvent(event);
			printf("Unrecognised event type: 0x%2x\n", event->type);
		}
	}
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

enum TimeDivType GetTimeDivisionType( unsigned short timeDivData )
{
	if (timeDivData & 0x8000)
		return framesPerSecond;
	else
		return ticksPerBeat;
}

