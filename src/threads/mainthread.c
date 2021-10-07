#include "mainthread.h"
#include "playback.h"
#include "record.h"

#include <psp2kern/power.h>
#include <taihen/taihen.h>

#include "../globals.h"

#define printf(...) ksceDebugPrintf("[MAFU-I] " __VA_ARGS__)

PayloadArguments* dejavu_pargs = 0;
SceBool requiredDejavu = 0;

SceBool getDejavu(PayloadArguments **pargs)
{
	tai_module_info_t modinfo = {0};
	modinfo.size = sizeof(tai_module_info_t);
	if(module_get_by_name_nid(KERNEL_PID, "dejavu", TAI_IGNORE_MODULE_NID, &modinfo) < 0)
	{
		printf("Could not find dejavu!\n");
		return 0;
	}
	if(module_get_offset(KERNEL_PID, modinfo.modid, 1, 0xb04c, pargs) < 0)
	{
		printf("Could not retrive dejavu's pargs!\n");
		return 0;
	}
	return 1;
}

/*uintptr_t getWaitRequest()
{

	if(module_get_export_func(KERNEL_PID, "SceTouch", 0xABC6F88F, 0x3951AF53, ) < 0)
	{
		return 0;
	}
	return 1;

}*/

void resumeSS()
{
	
	SavestateInfo info = {0};
	SceUID offsetFile = ksceIoOpen("ur0:/data/MAFU-I/SSInfo.dat", SCE_O_RDONLY, 0777);
	if(offsetFile)
	{
		//printf("Reading offset file...\n");
		ksceIoRead(offsetFile, &info, sizeof(info));
		ksceIoClose(offsetFile);
	}

	/*printf("Previous state: %d\n", info.state);
	printf("Previous offset: %lld\n", info.fileOffset);
	printf("Previous elapsed time: %lld\n", info.elapsedTime);*/

	switch(info.state)
	{
		case SS_REC:
		{
			isRecording = 1;
			isPlaying = 0;
			ssfileOffset = info.fileOffset;
			ssfileElapsedTime = info.elapsedTime;
			ksceKernelStartThread(captureThreadId, 0, 0);
		}break;
		case SS_PLAY:
		{
			isPlaying = 1;
			isRecording = 0;
			ssfileOffset = info.fileOffset;
			ssfileElapsedTime = info.elapsedTime;
			ksceKernelStartThread(playbackThreadId, 0, 0);
		}break;
	}
	
}

void powerCallback(int resume, int eventid, void *args, void *opt)
{
	if(resume)
	{
		if(eventid == 0x10000 && dejavu_pargs->mode != DEJAVU_MODE_NONE)	//This will (should) happen after dejavu has reloaded its variables
		{
			requiredDejavu = 1;
		}
		if(eventid == 0x100000 && requiredDejavu)
		{
			printf("Resuming after savestate...\n");
			requiredDejavu = 0;
			resumeSS();
		}
	}
}

void registerPowerCallback()
{
	//int cbid;
	//cbid = ksceKernelCreateCallback("MAFU-I_mainThread_powerCallback", 0, powerCallback, NULL);
	//kscePowerRegisterCallback(cbid);
	ksceKernelRegisterSysEventHandler("MAFU-I_mainThread_SysEventHandler", powerCallback, NULL);
}

void resetStateInfo()
{
	ssfileOffset = 0;
	ssfileElapsedTime = 0;
	SavestateInfo info = {0};
	SceUID offsetFile = ksceIoOpen("ur0:/data/MAFU-I/SSInfo.dat", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if(offsetFile < 0)
	{
		printf("Error opening Savestate Info file for writing! %#.8x\n", offsetFile);
		return -1;
	}
	ksceIoWrite(offsetFile, &info, sizeof(info));
	ksceIoClose(offsetFile);
}

int mainThreadLogic(SceSize args, void *argp)
{
	SceCtrlData padData = {0};
	SceCtrlData previousPadData = {0};
	char savestatepath[] = "ux0:/data/MAFU-I";

	printf("Main thread started!\n");
	while(!shellPid)
	{
		ksceKernelDelayThread(1000 * 1000 * 1 / 10);
	}

	playbackThreadId = ksceKernelCreateThread("MAFU-I_playbackThread", playbackThreadLogic, 0x10000100, 0x10000, 0, 0, NULL);
	captureThreadId = ksceKernelCreateThread("MAFU-I_captureThread", captureThreadLogic, 0x10000100, 0x10000, 0, 0, NULL);

	getDejavu(&dejavu_pargs);
	registerPowerCallback();
	
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
			else
			{
				int timeout = -1;
				ksceKernelWaitThreadEnd(playbackThreadId, 0, &timeout);
				resetStateInfo();
			}
			
		}
		
		if(!(isPlaying) && (padData.buttons == settings.recordKeycombo) && !(previousPadData.buttons == settings.recordKeycombo))
		{
			isRecording = !isRecording;

			if(isRecording)
			{
				ksceKernelStartThread(captureThreadId, 0, 0);
			}
			else
			{
				int timeout = -1;
				ksceKernelWaitThreadEnd(captureThreadId, 0, &timeout);
				resetStateInfo();
			}
		}

		if((dejavu_pargs) && (padData.buttons == settings.savestateKeycombo) && !(previousPadData.buttons == settings.savestateKeycombo))
		{
			SavestateInfo info = {0};
			int timeout = -1;

			if(isRecording)
			{
				isRecording = 0;
				ksceKernelWaitThreadEnd(captureThreadId, 0, &timeout);
				info.state = SS_REC;
			}
			else if(isPlaying)
			{
				isPlaying = 0;
				ksceKernelWaitThreadEnd(playbackThreadId, 0, &timeout);
				info.state = SS_PLAY;
			}

			info.elapsedTime = ssfileElapsedTime;
			info.fileOffset = ssfileOffset;

			/*printf("State: %d\n", info.state);
			printf("Offset: %lld\n", info.fileOffset);
			printf("Elapsed time: %lld\n", info.elapsedTime);*/

			SceUID offsetFile = ksceIoOpen("ur0:/data/MAFU-I/SSInfo.dat", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
			if(offsetFile < 0)
			{
				printf("Error opening Savestate Info file for writing! %#.8x\n", offsetFile);
				return -1;
			}
			ksceIoWrite(offsetFile, &info, sizeof(info));
			ksceIoClose(offsetFile);

			offsetFile = ksceIoOpen("ur0:/data/MAFU-I/SSInfo.dat", SCE_O_RDONLY, 0777);
			if(offsetFile)
			{
				//printf("Reading offset file...\n");
				ksceIoRead(offsetFile, &info, sizeof(info));
				ksceIoClose(offsetFile);
			}

			/*printf("State: %d\n", info.state);
			printf("Offset: %lld\n", info.fileOffset);
			printf("Elapsed time: %lld\n", info.elapsedTime);*/

			dejavu_pargs->mode = DEJAVU_MODE_SAVE;
			memcpy(dejavu_pargs->path, savestatepath, sizeof(savestatepath));
			kscePowerRequestSoftReset();
		}

		if((dejavu_pargs) && (padData.buttons == settings.loadstateKeycombo) && !(previousPadData.buttons == settings.loadstateKeycombo))
		{
			int timeout = -1;
			if(isRecording)
			{
				isRecording = 0;
				ksceKernelWaitThreadEnd(captureThreadId, 0, &timeout);
			}
			else if(isPlaying)
			{
				isPlaying = 0;
				ksceKernelWaitThreadEnd(playbackThreadId, 0, &timeout);
			}

			dejavu_pargs->mode = DEJAVU_MODE_LOAD;
			memcpy(dejavu_pargs->path, savestatepath, sizeof(savestatepath));
			kscePowerRequestSoftReset();
		}

		memcpy(&previousPadData, &padData, sizeof(SceCtrlData));
		ksceKernelDelayThreadCB(1000);		//Give other plugins/applications some time (e.g.: Adrenaline) AND check callbacks
	}
}