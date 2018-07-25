#include "util.h"

#include <stdio.h>


void SwapEndianness32( unsigned int *num )
{
	unsigned int swapped = ((*num>>24) & 0x000000ff) |
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

void PrintBytes( unsigned char* data, unsigned int numBytes )
{
	printf("Next %i bytes: 0x", numBytes);
	for (int i=0; i < numBytes; i++) {
		printf("%02x", *(data+i));
	}
	printf("\n");
}
