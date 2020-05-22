/* Compile with gcc -lasound -pthread threadaudio.c */

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <semaphore.h>

using namespace std;

#define PCM_DEVICE "default"

#define C1 "316898__jaz-the-man-2__do.wav"
#define D1 "316908__jaz-the-man-2__re.wav"
#define E1 "316906__jaz-the-man-2__mi.wav"
#define F1 "316904__jaz-the-man-2__fa.wav"
#define G1 "316912__jaz-the-man-2__sol.wav"
#define A1 "316902__jaz-the-man-2__la.wav"
#define B1 "316913__jaz-the-man-2__si.wav"

#define FILE_COUNT 7

//#define DEBUG

#ifdef DEBUG
#define PRINT(...) do { printf(__VA_ARGS__); } while (0)
#else
#define PRINT(...)  
#endif

const char *file[FILE_COUNT] = { C1, D1, E1, F1, G1, A1, B1};
char* waveBuffer[FILE_COUNT];
int waveOffset[FILE_COUNT] = {-1, -1, -1, -1, -1, -1, -1};
int fileSize[FILE_COUNT];

unsigned char audiobuffer[0x40000];
unsigned char mixbuffer[0x40000];
pthread_mutex_t audiomutex = PTHREAD_MUTEX_INITIALIZER;

int frameBufferSize = 0;
int playSize = 0;
int audiostop = 0;

sem_t empty, full;

void *audioPlayer (void *param)
{
	snd_pcm_t *pcm_handle;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;
	unsigned int rate, channels;
	unsigned int pcm;

	rate 	 = 44100;
	channels = 2;

	/* Open the PCM device in playback mode */
	if (pcm = snd_pcm_open(&pcm_handle, PCM_DEVICE,
				SND_PCM_STREAM_PLAYBACK, 0) < 0) 
	{
		printf("ERROR: Can't open \"%s\" PCM device. %s\n",
				PCM_DEVICE, snd_strerror(pcm));
		return 0;
	}

	/* Allocate parameters object and fill it with default values*/
	snd_pcm_hw_params_alloca(&params);

	snd_pcm_hw_params_any(pcm_handle, params);

	/* Set parameters */
	if (pcm = snd_pcm_hw_params_set_access(pcm_handle, params,
				SND_PCM_ACCESS_RW_INTERLEAVED) < 0) 
		printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm));

	if (pcm = snd_pcm_hw_params_set_format(pcm_handle, params,
				SND_PCM_FORMAT_S16_LE) < 0) 
		printf("ERROR: Can't set format. %s\n", snd_strerror(pcm));

	if (pcm = snd_pcm_hw_params_set_channels(pcm_handle, params, channels) < 0) 
		printf("ERROR: Can't set channels number. %s\n", snd_strerror(pcm));

	if (pcm = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0) < 0) 
		printf("ERROR: Can't set rate. %s\n", snd_strerror(pcm));

	/* Write parameters */
	if (pcm = snd_pcm_hw_params(pcm_handle, params) < 0)
		printf("ERROR: Can't set harware parameters. %s\n", snd_strerror(pcm));

	/* Allocate buffer to hold single period */
	snd_pcm_hw_params_get_period_size(params, &frames, 0);

	frameBufferSize = frames * channels * 2 /* 2 -> sample size */;

	int *audiostop = (int*)param;
	while (!*audiostop) {
		(void) sem_wait(&full);

		int size = playSize;
		int offset = 0;
		while (size - offset >= frameBufferSize) {
			pcm = snd_pcm_writei(pcm_handle, audiobuffer + offset, frames);

			if (pcm == -EPIPE) {
				printf("XRUN.\n");
				snd_pcm_prepare(pcm_handle);
			} else if (pcm < 0) {
				printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm));
			} else {
				offset += pcm;
			}
		}
		if (size > 0)
		PRINT("play %d / %d\n", offset, size);

		(void) sem_post(&empty);

	}

	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);
}

void *mixer (void *param)
{
	int *audiostop = (int*)param;
	while (!*audiostop) {
		int maxSize = frameBufferSize;
		//int maxSize = sizeof(mixbuffer);
		memset(mixbuffer, 0, maxSize);

		int i = 0; 
		for (i = 0; i < maxSize; i++) {
			int total = 0;
			int count = 0;
			for (int j = 0; j < FILE_COUNT; j++)
			{
				if (waveOffset[j] >= 0)
				{
					if (waveOffset[j] >= fileSize[j])
					{
						waveOffset[j] = -1;
					}
					else
					{
						total += (unsigned char) waveBuffer[j][waveOffset[j]];
						count++;
						waveOffset[j]++;
					}
				}
			}

			if (count > 0) {			
				mixbuffer[i] = (int)(total / (float)count);
			}
			else
			{
				break;
			}				
		}

		playSize = i;
		if (i > 0)
		{
			PRINT("Wave offset: ");
			for (int j = 0; j < FILE_COUNT; j++)
			{
				PRINT("%d:%d \t", j, waveOffset[j]);
			}
			PRINT("\n");
		}

		(void) sem_wait(&empty);

		memcpy(audiobuffer, mixbuffer, i);

		(void) sem_post(&full);
	}
}

int loadFiles() {
	for (int i = 0; i < FILE_COUNT; i++)
	{
		ifstream is(file[i]);
		is.seekg(0, ios::end);
		int length = is.tellg();
		printf("%d %s wave size: %d\n", i, file[i], length);
		waveBuffer[i] = (char*)malloc(length);
		fileSize[i] = length;
		is.seekg(0, ios::beg);
		is.read (waveBuffer[i],length);
		is.close();
	}
	return 0;
}

pthread_t audiothread;
pthread_t mixerThread;

int initSound(void)
{
	printf( "Odroid Ultrasonic sensor Piano program\n" );

	printf( "creating threads\n" );
	sem_init(&empty, 0, 1);
	sem_init(&full, 0, 0);

	pthread_attr_t tattr;
	int ret;
	int newprio = -20;
	sched_param param;
	
	/* initialized with default attributes */
	ret = pthread_attr_init (&tattr);
	
	/* safe to get existing scheduling param */
	ret = pthread_attr_getschedparam (&tattr, &param);
	
	/* set the priority; others are unchanged */
	param.sched_priority = newprio;
	
	/* setting the new scheduling param */
	ret = pthread_attr_setschedparam (&tattr, &param);

	// ALSA Player 생성.
	// Audio QUEUE에 값이 들어있다면 계속 Play한다. 
	pthread_create(&audiothread, &tattr, audioPlayer, &audiostop);

	// Mux thread
	// 개별 wave 파일의 offset 정보를 가지고 있고 offset에서 frame size만큼 하나의 출력으로 만들어 Audio Queue에 넣어 play 하도록 한다.
	pthread_create(&mixerThread, NULL, mixer, &audiostop);

	//memset(playing, 0, sizeof(gpio));
	printf( "Loading wave files\n" );
	loadFiles();
	printf( "Load completed\n" );

	return 0;
}

void stop()
{
	audiostop = 1;
	pthread_join(audiothread, NULL);
}

void play(int index)
{
	waveOffset[index] = 0;
}
