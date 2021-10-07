#include "record.h"

#include "../globals.h"

#define printf(...) ksceDebugPrintf("[MAFU-I] " __VA_ARGS__)

void chkDigital(InputState *newInputState, InputState *oldInputState, SceUInt64 captureStart, SceUInt64 currentTime);
void chkAnalog(InputState *newInputState, InputState *oldInputState, SceUInt64 captureStart, SceUInt64 currentTime);
void chkTouch(InputState *newInputState, InputState *oldInputState, SceUInt64 captureStart, SceUInt64 currentTime, int port);

SceBool cmpAnalog(unsigned char *d1, unsigned char *d2);
SceBool cmpTouch(SceTouchData *d1, SceTouchData *d2);
SceBool writePacket(CaptureInfo *header, void* data);

SceUID captureFile = 0;

//TODO: nothing???

void resetAnalog(InputState *inputstate)
{
	inputstate->controlData.lx = inputstate->controlData.ly = inputstate->controlData.rx = inputstate->controlData.ry = 127;
}

int captureThreadLogic(SceSize args, void *argp)
{
	printf("Started capture thread\n");
	
	InputState oldInputState = {0}; resetAnalog(&oldInputState);
	InputState newInputState = {0}; resetAnalog(&newInputState);
	SceUInt64 captureStart;
	SceUInt64 currentTime;
	
	if(ssfileOffset == 0)
	{
		captureFile = ksceIoOpen("ur0:/data/MAFU-I/capture.dat", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
		printf("No offset!\n");
	}
	else
	{
		printf("Offset: %lld\n", ssfileOffset);
		char byte = 0;

		printf("Rename: %#.8x\n", ksceIoRename("ur0:/data/MAFU-I/capture.dat", "ur0:/data/MAFU-I/capture.old"));
		captureFile = ksceIoOpen("ur0:/data/MAFU-I/capture.dat", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
		SceUID oldFile = ksceIoOpen("ur0:/data/MAFU-I/capture.old", SCE_O_RDONLY, 0777);
		
		for(int i = 0; i < ssfileOffset; i++)
		{
			ksceIoRead(oldFile, &byte, 1);
			ksceIoWrite(captureFile, &byte, 1);
		}

		ksceIoClose(oldFile);
		ksceIoRemove("ur0:/data/MAFU-I/capture.old");
	}

	if(captureFile < 0)
	{
		printf("Error opening capture file for writing! %#.8x\n", captureFile);
		isRecording = 0;
		return -1;
	}

	//Reset timers
	captureStart = timers.digitalTick = timers.analogTick = timers.frontTouchTick = timers.backTouchTick = timers.accelTick = timers.gyroTick = ksceKernelGetSystemTimeWide() - ssfileElapsedTime;
	oldInputState.controlData.buttons = settings.recordKeycombo;
	
	//TODO: restore sampling states?
	ksceTouchEnableTouchForce(SCE_TOUCH_PORT_FRONT);
	ksceTouchEnableTouchForce(SCE_TOUCH_PORT_BACK);
	ksceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	ksceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);

	while(isRecording)
	{
		currentTime = ksceKernelGetSystemTimeWide();

		if(timers.digitalDelay <= currentTime - timers.digitalTick)
		{
			chkDigital(&newInputState, &oldInputState, captureStart, currentTime);
			timers.digitalTick = currentTime;
		}

		if(timers.analogDelay <= currentTime - timers.analogTick)
		{
			chkAnalog(&newInputState, &oldInputState, captureStart, currentTime);
			timers.analogTick = currentTime;
		}

		if(timers.frontTouchDelay <= currentTime - timers.frontTouchTick)
		{
			chkTouch(&newInputState, &oldInputState, captureStart, currentTime, SCE_TOUCH_PORT_FRONT);
			timers.frontTouchTick = currentTime;
		}
		
		if(timers.backTouchDelay <= currentTime - timers.backTouchTick)
		{
			chkTouch(&newInputState, &oldInputState, captureStart, currentTime, SCE_TOUCH_PORT_BACK);
			timers.backTouchTick = currentTime;
		}

		ksceKernelDelayThread(1000);
	}
	//Clean up and return
	ssfileOffset = ksceIoLseek(captureFile, 0, SCE_SEEK_CUR);
	ssfileElapsedTime = currentTime - captureStart;

	/*printf("Last offset: %lld\n", ssfileOffset);
	printf("Elapsed time: %lld\n", ssfileElapsedTime);*/

	ksceIoClose(captureFile);
	printf("Capture stopped\n");
	return 0;
}

void chkDigital(InputState *newInputState, InputState *oldInputState, SceUInt64 captureStart, SceUInt64 currentTime)
{
	ksceCtrlPeekBufferPositive(0, &newInputState->controlData, 1);             	//Peek digital buff

	if(newInputState->controlData.buttons != oldInputState->controlData.buttons)  //If buttons changed, record them with the timestamp.
	{                                                                           //TODO: look into the time in ScePadData struct maybe?
		CaptureInfo header = {0};
		header.type = INPUT_DIGITAL;
		header.captureTime = currentTime - captureStart;
		header.dataSize = sizeof(int);

		writePacket(&header, &newInputState->controlData.buttons);

		memcpy(&oldInputState->controlData.buttons, &newInputState->controlData.buttons, sizeof(int));
	}
}

void chkAnalog(InputState *newInputState, InputState *oldInputState, SceUInt64 captureStart, SceUInt64 currentTime)
{
	unsigned char *newAnalogPointer = (unsigned char *)(&newInputState->controlData) + 12;     //Discard digital data
	unsigned char *oldAnalogPointer = (unsigned char *)(&oldInputState->controlData) + 12;     //Discard digital data

	ksceCtrlPeekBufferPositive(0, &newInputState->controlData, 1);          //Peek analog buff

	if(cmpAnalog(newAnalogPointer, oldAnalogPointer))                //If buttons changed, record them with the timestamp.
	{
		CaptureInfo header = {0};
		header.type = INPUT_ANALOG;
		header.captureTime = currentTime - captureStart;
		header.dataSize = sizeof(SceCtrlData) - 12;

		writePacket(&header, newAnalogPointer);

		memcpy(oldAnalogPointer, newAnalogPointer, header.dataSize);
	}
}

void chkTouch(InputState *newInputState, InputState *oldInputState, SceUInt64 captureStart, SceUInt64 currentTime, int port)
{
	SceTouchData* newTouchData = (port == SCE_TOUCH_PORT_FRONT ? &newInputState->frontTouchData : &newInputState->backTouchData);
	SceTouchData* oldTouchData = (port == SCE_TOUCH_PORT_FRONT ? &oldInputState->frontTouchData : &oldInputState->backTouchData);
	
	ksceTouchPeek(port, newTouchData, 1);

	if(cmpTouch(newTouchData, oldTouchData))                
	{
		CaptureInfo header = {0};
		header.type = (port == SCE_TOUCH_PORT_FRONT ? INPUT_TOUCHPAD_FRONT : INPUT_TOUCHPAD_BACK);
		header.captureTime = currentTime - captureStart;
		header.dataSize = sizeof(SceTouchData);

		writePacket(&header, newTouchData);

		memcpy(oldTouchData, newTouchData, header.dataSize);
	}
}

SceBool cmpAnalog(unsigned char *d1, unsigned char *d2)
{
	for(int i = 0; i < 16; i++)
	{
		if(*(d1 + i) < *(d2 + i) - settings.analogThreshold
		|| *(d1 + i) > *(d2 + i) + settings.analogThreshold)
		{
			return 1;
		}
	}
	return 0;
}

SceBool cmpTouch(SceTouchData *d1, SceTouchData *d2)
{
	if(d1->reportNum != d2->reportNum)
	{
		return 1;
	}

	for(int i = 0; i < d1->reportNum; i++)
	{
		if(memcmp(&d1->report[i], &d2->report[i], sizeof(SceTouchReport)) != 0)
		{
			return 1;
		}
	}
	return 0;
}

SceBool writePacket(CaptureInfo *header, void* data)
{
	//printf("Recording input of type %d,scheduled for %llu.\n", header->type, header->captureTime);
			
	ksceIoWrite(captureFile, header, sizeof(CaptureInfo));
	ksceIoWrite(captureFile, data, header->dataSize);

	return 1;
}