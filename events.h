#ifndef __EVENTS_H__
#define __EVENTS_H__


typedef struct {
	unsigned char type;
	unsigned int size;
	unsigned char* data;
} Event;

typedef struct {
	unsigned char* type;
	unsigned int length;
	unsigned char* data;
} Chunk;

typedef struct {
	Event* events;
	unsigned int numEvents;
} Track;

enum TimeDivType {
	ticksPerBeat = 0,
	framesPerSecond = 1,
};

struct FramesPerSecond {
	unsigned short smpteFrames;
	unsigned short ticksPerFrame;
} FramesPerSecond;

union TimeDivision {
	struct FramesPerSecond framesPerSecond;
	unsigned short ticksPerBeat;
} TimeDivision;

typedef struct {
	unsigned short formatType;
	unsigned short numTracks;
	enum TimeDivType timeDivisionType;
	union TimeDivision timeDivision;
} FileInfo;



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


unsigned int GetTempoBPM(unsigned char* buffer);
struct TimeSignature GetTimeSignature(unsigned char* buffer);
struct KeySignature GetKeySignature(unsigned char* buffer);
void PrintFileInfo(FileInfo* fileInfo);
void PrintEvent(Event* event);
union TimeDivision GetTimeDivision(unsigned short tDivData);
enum TimeDivType GetTimeDivisionType(unsigned short timeDivData);


#endif
