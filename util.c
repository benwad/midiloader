#include "util.h"


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

