/* Compile with gcc -lasound -pthread threadaudio.c */

#include <stdio.h>
#include "PianoPlayer.h"



int main(void)
{
	int volume;

	initSound();
	
	while (1) {
		printf("Enter file index. [0 to quit.]: ");
		scanf("%d", &volume);
		if (volume == 0) break;

		printWaveHeader(volume - 1);
	}

	stop();

	return 0;
}
