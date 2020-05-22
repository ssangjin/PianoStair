/* Compile with gcc -lasound -pthread threadaudio.c */

#include <stdio.h>
#include "PianoPlayer.h"

int main(void)
{
	int volume;

	initSound();
	
	while (1) {
		printf("Enter volume 1 through 10. [0 to quit.]: ");
		scanf("%d", &volume);
		if (volume == 0) break;

		play(volume - 1);
	}

	stop();

	return 0;
}
