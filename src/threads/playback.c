#include "playback.h"

#include "../globals.h"

#define printf(...) ksceDebugPrintf("[MAFU-I] " __VA_ARGS__)

int playbackThreadLogic(SceSize args, void *argp)
{
	printf("Started playback thread\n");
	//send_notification_request("Playback");

	CaptureInfo playbackHeader = {0};
	SceUInt64 playbackStart;
	SceUInt64 currentTime;
	SceUID captureFile;

	captureFile = ksceIoOpen("ur0:/data/MAFU-I/capture.dat", SCE_O_RDONLY, 0777);
	if(captureFile < 0)
	{
		printf("Error opening capture file for reading! %#.8x\n", captureFile);
		isPlaying = 0;
		return -1;
	}

	//printf("Playback offset: %lld\n", ssfileOffset);

	if(ssfileOffset != 0)
		ksceIoLseek(captureFile, ssfileOffset, SCE_SEEK_SET);

	playbackStart = ksceKernelGetSystemTimeWide() - ssfileElapsedTime;

	while(isPlaying)
	{
		currentTime = ksceKernelGetSystemTimeWide();
		
		if(playbackHeader.captureTime <= currentTime - playbackStart)
		{
			switch(playbackHeader.type)
			{
				case INPUT_NONE:
				break;
				case INPUT_DIGITAL:
				{
					ksceIoRead(captureFile, &inputState.controlData.buttons, playbackHeader.dataSize);
				} break;
				case INPUT_ANALOG:
				{
					ksceIoRead(captureFile, (((void *)(&inputState.controlData)) + 12), playbackHeader.dataSize);
				} break;
				case INPUT_TOUCHPAD_FRONT:
				{
					ksceIoRead(captureFile, &inputState.frontTouchData, playbackHeader.dataSize);
				} break;
				case INPUT_TOUCHPAD_BACK:
				{
					ksceIoRead(captureFile, &inputState.backTouchData, playbackHeader.dataSize);
				} break;
				default:
				{
					printf("Unknown packet!\n");
				}break;
			}

			if(ksceIoRead(captureFile, &playbackHeader, sizeof(CaptureInfo)) == 0)              //The file has reached the end, there are no more inputs.
			{                                                                                   //TODO: error checking?
				isPlaying = 0;
				/*playbackStart = ksceKernelGetSystemTimeWide();
				ksceIoLseek(captureFile, 0, SCE_SEEK_SET);
				memset(&playbackHeader, 0, sizeof(CaptureInfo));*/  //NONSTOP
				ssfileOffset = 0;
				ssfileElapsedTime = 0;
				printf("Reached EOF\n");
				memset(&inputState, 0, sizeof(InputState));
				printf("Playback stopped\n");
				return 0;
			}
			else
			{
				//printf("Next input is of type %d and it's scheduled for %llu, current time is %llu\t(%llu/%llu)\n", playbackHeader.type, playbackHeader.captureTime, currentTime - playbackStart, playbackHeader.captureTime, currentTime - playbackStart);
			}
			
		}
		ksceKernelDelayThread(1000);    //Give other plugins/applications some time (i.e.: Adrenaline)
	}
	//Clean up and return
	ssfileOffset = ksceIoLseek(captureFile, 0, SCE_SEEK_CUR) - sizeof(CaptureInfo);	//We've already read the header, we need to go back!
	ssfileElapsedTime = currentTime - playbackStart;

	ksceIoClose(captureFile);
	memset(&inputState, 0, sizeof(InputState));
	printf("Playback stopped %lld\n", ssfileOffset);
	return 0;
}