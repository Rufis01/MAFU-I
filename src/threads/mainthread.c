#include "mainthread.h"
#include "playback.h"
#include "record.h"

#include "../globals.h"

#define printf(...) ksceDebugPrintf("[MAFU-I] " __VA_ARGS__)

int mainThreadLogic(SceSize args, void *argp)
{
	SceCtrlData padData = {0};
	SceCtrlData previousPadData = {0};

	printf("Main thread started!\n");
	while(!shellPid)
	{
		ksceKernelDelayThread(1000 * 1000 * 1 / 10);
	}

	SceUID playbackThreadId = ksceKernelCreateThread("MAFU-I_playbackThread", playbackThreadLogic, 0x10000100, 0x10000, 0, 0, NULL);
	SceUID captureThreadId = ksceKernelCreateThread("MAFU-I_captureThread", captureThreadLogic, 0x10000100, 0x10000, 0, 0, NULL);
	
	ksceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);

	while(1)
	{
		ksceCtrlPeekBufferPositive(0, &padData, 1);        
		
		if(!(isRecording) && (padData.buttons == settings.playKeycombo) && !(previousPadData.buttons == settings.playKeycombo))
		{
			isPlaying = !isPlaying;

			if(isPlaying)
			{
				ksceKernelStartThread(playbackThreadId, 0, 0);
			}
			
		}
		
		if(!(isPlaying) && (padData.buttons == settings.recordKeycombo) && !(previousPadData.buttons == settings.recordKeycombo))
		{
			isRecording = !isRecording;

			if(isRecording)
			{
				ksceKernelStartThread(captureThreadId, 0, 0);
			}
		}

		memcpy(&previousPadData, &padData, sizeof(SceCtrlData));
		ksceKernelDelayThread(1000);    //Give other plugins/applications some time (e.g.: Adrenaline)
	}
}