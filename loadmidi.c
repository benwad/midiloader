#include "stdio.h"

enum TimeDivType {
	ticksPerBeat = 0,
	framesPerSecond = 1,
};

struct FileInfo {
	unsigned short formatType;
	unsigned short numTracks;
	enum TimeDivType timeDivisionType;
	unsigned short timeDivisionValue;
};

void SwapEndianness32( unsigned int *num )
{
	unsigned int swapped = ((*num>>24) & 0xff) |
							((*num<<8) & 0xff0000) |
							((*num>>8) & 0xff00) |
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

	// unsigned int chunkSize = (*sizebuffer >> 8) | (*sizebuffer << 8);
	SwapEndianness32(sizebuffer);

	printf("Result: %i\n", n);
	printf("Size of unsigned int: %i\n", (int)sizeof(unsigned int));
	printf("Size of unsigned short: %i\n", (int)sizeof(unsigned short));
	printf("Chunk size: 0x%x\n", *sizebuffer);

	unsigned short headerInfo[*sizebuffer / sizeof(unsigned short)];
	n = fread(headerInfo, *sizebuffer, 1, f);

	for (int i=0; i < *sizebuffer / sizeof(unsigned short); i++ )
	{
		SwapEndianness16(&headerInfo[i]);
	}

	struct FileInfo fileInfo;

	printf("Result: %i\n", n);
	fileInfo.formatType = headerInfo[0];
	fileInfo.numTracks = headerInfo[1];
	fileInfo.timeDivisionType = GetTimeDivisionType(headerInfo[2]);
	fileInfo.timeDivisionValue = (headerInfo[2] & 0x7FFF);

	printf("Format type: %i\n", fileInfo.formatType);
	printf("Num tracks: %i\n", fileInfo.numTracks);
	printf("Time division: %s\n", (fileInfo.timeDivisionType == framesPerSecond) ? "framesPerSecond" : "ticksPerBeat");

	fclose(f);

	return 0;

}

int main( int argc, char* argv[] )
{
	if (argc < 2)
		LoadMidiFile("/Users/bwadsworth/Documents/Dev/Snippets/midi/SMB1-Theme.mid");
	else
		LoadMidiFile(argv[1]);

	return 0;
}