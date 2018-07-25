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


void PrintSysexEvent(Event* event)
{
	printf("%02x SysEx event: 0x", event->type);
	for (int i=0; i < event->size; i++)
	{
		printf("%02x", *(event->data + i));
	}

	printf("\n");
}


void PrintMidiEvent(Event* event)
{
	printf("MIDI event %02x\n", event->type);

	unsigned char statusBits = (event->type & 0xF0);
	switch (statusBits) {
		case 0x80: {
			printf("Note off\n");
			break;
		}
		case 0x90: {
			printf("Note on\n");
			break;
		}
		case 0xA0: {
			printf("Polyphonic aftertouch\n");
			break;
		}
		case 0xB0: {
			printf("Controller change\n");
			break;
		}
		case 0xC0: {
			printf("Program change\n");
			break;
		}
		case 0xD0: {
			printf("Channel aftertouch\n");
			break;
		}
		case 0xE0: {
			printf("Pitch bend\n");
			break;
		}
		default: {
			printf("Unknown MIDI event type: %02x\n", event->type);
			break;
		}
	}
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
			return 0;
	}
}


void PrintEvent(Event* event) {
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

