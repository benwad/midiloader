#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char type;
	unsigned int size;
	unsigned char* data;
} Event;

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


void PrintEvent(Event* event) {
	switch(event->type) {
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
		default:
		{
			printf("MetaEvent 0x%2x not found\n", event->type);
			return;
		}
	}
}

#endif
