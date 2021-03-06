/*
 *  hc_sr04.c:
 *    Simple test program to test the wiringPi functions
 *    hc_sr04 test
 */

#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/resource.h>
#include <wiringPi.h>
#include <fstream>
#include <iostream>
#include <alsa/asoundlib.h>
#include "PianoPlayer.h"

using namespace std;

#define GPIO_TRIGGER 0 
#define GPIO_ECHO1 1 
#define GPIO_ECHO2 2 
#define GPIO_ECHO3 3 
#define GPIO_ECHO4 4 
#define GPIO_ECHO5 5
#define GPIO_ECHO6 6
#define GPIO_ECHO7 7
#define GPIO_ECHO8 10

#define GPIO_LED 9

int gpio[] = {GPIO_ECHO1,
	GPIO_ECHO2,
	GPIO_ECHO3,
	GPIO_ECHO4,
	GPIO_ECHO5,
	GPIO_ECHO7};

#define C 0
#define D 1
#define E 2
#define F 3
#define G 4
#define A 5
#define B 6

int song[] = {
							 //  
							 C, C, G, G, G, A, A, G, F, F, E, E, D, D, C, 
							 G, G, F, F, E, E, D, G, G, F, F, E, E, D, 
							 C, C, G, G, A, A, G, F, F, E, E, D, D, C,
							 
							 // 
							 G, G, A, A, G, G, E, G, G, E, E, D,
							 G, G, A, A, G, G, E, G, E, D, E, C,

							 //   
							 C, C, C, D, E, E, D, E, F, G, 
							 C, C, C, G, G, G, E, E, E, C, C, C, G, F, E, D, C, 
							 
							 // 
							 E, G, G, E, G, G, A, A, A, A, A,
							 G, G, G, G, F, F, F, F, E, E, E, E, E, 
							 E, G, G, G, E, G, G, A, A, E, E, G, 
							 F, F, F, F, F, E, E, E, E, D, D, G, G, C, 

							// 
							G, E, E, G, E, C, D, E, D, C, E, G, C, G, C, G, C, G, E, G, D, F, E, D, C, 

							// 
							G, E, G, E, D, C, D, C,
							D, D, D, E, F, D, 
							E, E, E, F, G, E, 
							G, E, G, E, F, E, D, C, 

							// 
							G, E, E, F, E, E, C, D, E, F, G, G, G, 
							G, D, D, D, F, D, D, C, E, G, G, E, E, E, 
							D, D, D, D, D, E, F, E, E, E, E, E, F, G, 
							G, E, E, F, D, D, C, E, G, G, E, E, E, 
							
							// 
							E, E, E, E, E, E, E, G, C, D, E, F, F, F, F, F, E, E, E, D, D, E, D, G, 
							E, E, E, E, E, E, E, G, C, D, E, F, F, F, F, F, E, E, G, G, F, D, C
							 };
							 
							 
int playIndex = 0;

int randIndex[8] = {0, 3, 1, 4, 2, 5, 3, 7};
int playing[8] = {-1};

int overLimit(int i, float distance)
{
	float diff = 60 - distance;

	if (diff > 0 && playing[i] < 0) {
		playing[i] = 1;
		return 1;
	}

	if (diff < -10) {
		playing[i] = -1;
	}

	return 0;
}

int wait_state(int port, int state)
{
	int count = 0;
	while ( digitalRead( port) == state ) {
		count++;
		if (count > 30000) {
			//if (count > 14574) {
			return -1;
		}
	}
	return 0;
}

float get_distance(int port)
{
	digitalWrite( GPIO_TRIGGER, LOW);
	delayMicroseconds( 2 );
	// Send 10us pulse to trigger
	digitalWrite( GPIO_TRIGGER, HIGH);
	delayMicroseconds( 10 );
	digitalWrite( GPIO_TRIGGER, LOW);
	if (wait_state(port, LOW) == -1) {
		return 999;
	}

	long start = micros();

	if (wait_state(port, HIGH) == -1) {
		return 999;
	}
	long stop = micros();

	return (float)(stop - start) / 58.8235;
}

float getAverageDist(float dist[5])
{
	int i;
	float sum = 0;
	float max = 0;
	int count = 0;
	for (i = 0; i < 5; i++) {
		if (dist[i] > 0) {
			sum += dist[i];
			count++;
		}
		if (dist[i] > max) {
			max = dist[i];
		}
	}

	return max;

	if (count > 0) {
		return sum / count;
	}
	return 0;
}

int getLocalTime()
{
  time_t currentTime;
  struct tm *localTime;

  time( &currentTime );
  localTime = localtime( &currentTime );

  int Hour   = localTime->tm_hour;
  int Min    = localTime->tm_min;
  int Sec    = localTime->tm_sec;

//  std::cout << "Execute at: " << Hour << ":" << Min << ":" << Sec << std::endl;
	return Hour * 10000 + Min * 100 + Sec;
}

int main( void )
{
	int loop = 0, count, i;
	float dist[7][5];
	float distance;
	int distIndex = 0;
	printf( "Piano stair!!!\n" );

	initSound();

	wiringPiSetup();

	pinMode( GPIO_TRIGGER, OUTPUT );

	for (i = 0; i < sizeof(gpio) / sizeof(int); i++)
	{
		pinMode(gpio[i], INPUT );
		printf( "pinMode output [%d] = %d \n" , i, gpio[i]);
	}

	// Set trigger to False (Low)
	digitalWrite( GPIO_TRIGGER, LOW );
	//	digitalWrite(GPIO_LED, LOW); 
	delay( 500 );

	while ( 1 )
	{
		int time = getLocalTime();
		if (time < 73000 || time > 220000)
		{
			printf( "Sleep time.\n");
			sleep(60);
			continue;
		}
		
		int min = 1000;
		int mi = -1;
		
		for (int index = 0; index < sizeof(gpio) / sizeof(int); index++)
		{
			
			int i = randIndex[index];	
			//printf( "%d : %d\n", index, i);
			dist[i][distIndex] = get_distance(gpio[i]);
			float distance = getAverageDist(dist[i]);

			if (distance > 1 && overLimit(i, distance)) {
				// play(i);
				printf( "Distance %d: %9.1f cm\n", i, distance );
				if (min > distance) {
					min = distance;
					mi = i;
				}
			}

			delay(10);
		}
		
		if (min > 0 && min < 100) {
			printf( "Play %d\n", mi);
			// play(mi);
			play(song[playIndex++]);
			
			if (playIndex >= sizeof(song) / sizeof(int)) {
				playIndex = 0;
			}
		}

		for (i = 0; i < sizeof(gpio) / sizeof(int); i++)
			printf( "%d\t%d: %5.1f cm,\t", time, i, getAverageDist(dist[i]));
		printf( "\n");
		printf( "\n");

		distIndex++;
		if (distIndex >= 1) {
			distIndex = 0;
		}
		delay(30);

	}
	return 0;
}
