/*
           @@@@@@@@@ @@@@@@@@@@@@@@@ @@@@@@@@@@@@@@@@@   @@@@@@@@@@@@@@@@@@@   
         @@@(((  ((@@(((((((((    (@@@((((((((((((  (@@@@@((((((((((((    (@@@ 
       @@(((((((  ((@@(((((((((((  ((@@@(((@@@@@(((   (((@@((((((((((((((  ((@@
     @@(((((((((((((@@(((@@@@@(((((((((@@@((@  @@((((((((((@@((((((((((((((((@@
    @(((((((((((@@@@@(((((@@  @((((((((@@@((@  @@(((((((((@@((((((((@@@@@@@@@  
   @((((((((@@@      @((((@@   @((((((((@@(((@@@((((((((((@@((((((@@@          
  @(((((((@@         @@((((@@@@(((((((((@@((((((((((((((((@@((((@@@@           
 @@(((((@@@@          @((((((((((((((((@@@(((((((((((((((@@(((((@@@@           
@@((((((((@@@@@@@@@@@@@@((((((((((((((@@@@(((((((((((((@@(((((((@@@@@          
@((((((((((((        @@@@(((((((((((((((((@@@@(((((@@((((((((((((@@@@@@        
@((((((((((((         ((@@@(((((((((((((    ((@@@(((@@@@((((((((((((  ((@@@@@  
@(((((((((((((((((      (@@@(((((@@@((((((    ((@@@((@   @(((((((((((    ((@@@@
@((((((((((((((((((    (@@@@((((@@    @((((((  ((@@((@@   @(((((((((((((  ((@@@
 @@(((((((((((((((((((((@@@@((((@@   @@((((((((((@@@((@@@@(((((((((((((((((((@@
  @@((((((((((((((((((((@@@@(((@@@@@@((((((((((((@@@((@@((((((((((((((((((((@@@
   @@((((((((((((((((((@@@@(((((((((((((((((((((@@@(((@@@((((((((((((((((@@@@@ 
     @@@@((((((((((((@@@@(((((((((((((((((((((@@@@(((((@@(((((((((@@@@@@@@     
        @@@@@@@@@@@@@@  @@@@@@@@@@@@@@@@@@@@@@  @@@@@@@@ @@@@@@@@@@            
                                  Presents

Name: MagicAutomaticFakeUserInput (MAFU-Input)
Version: 1.0
Author: Rufis_

Description:
A plugin to record your inputs and play them back.

TODO: add header to capture file to store info (size?) and flags like MAFU_CAPTURE_DIGITAL, MAFU_CAPTURE_ANALOG, etc...

*/
#include <psp2kern/ctrl.h>
#include <psp2kern/io/stat.h> 
#include <psp2kern/io/fcntl.h> 
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/threadmgr.h> 

#include <taihen.h>

#include <stdio.h>
#include <string.h>

#include "types.h"

#define printf ksceDebugPrintf

int (*ksceTouchUpdateInit)(void);

int recordKeycombo = (SCE_CTRL_LEFT | SCE_CTRL_CIRCLE | SCE_CTRL_RTRIGGER);
int playKeycombo = (SCE_CTRL_RIGHT | SCE_CTRL_CIRCLE | SCE_CTRL_RTRIGGER);

tai_hook_ref_t _ksceCtrlPeekBufferPositive = 0;
tai_hook_ref_t _sceCtrlPeekBufferPositive = 0;
tai_hook_ref_t _sceCtrlPeekBufferPositive2 = 0;

tai_hook_ref_t _sceCtrlSetSamplingMode = 0;

Timers timers = {0};
Settings settings = {(unsigned int)5};

SceBool isRecording = 0;
SceBool isPlaying = 0;

InputState inputState = {0};

int h_sceCtrlPeekBufferPositive(int port, SceCtrlData* pad_data, int count)
{
    //printf("[MAFU-I] Called hooked function sceCtrlPeekBufferPositive()\n");
    int ret = TAI_CONTINUE(int,_sceCtrlPeekBufferPositive, port, pad_data, count);

    if(pad_data)
    {
        SceCtrlData kernelPadData = {0};
        ksceKernelMemcpyUserToKernel((void *)&kernelPadData, (uintptr_t)pad_data, sizeof(SceCtrlData));
        kernelPadData.buttons |= inputState.controlData.buttons;

        if(isPlaying)
        {
            for(int i = 0; i < 16; i++)
            {
                *( ((unsigned char *)(&kernelPadData)) + 12 + i) = *( ((unsigned char *)(&inputState.controlData)) + 12 + i);
            }
        }

        ksceKernelMemcpyKernelToUser((uintptr_t)pad_data, (void *)&kernelPadData, sizeof(SceCtrlData));
    }
    return ret;
}

int h_sceCtrlPeekBufferPositive2(int port, SceCtrlData* pad_data, int count)
{
    //printf("[MAFU-I] Called hooked function sceCtrlPeekBufferPositive2()\n");
    int ret = TAI_CONTINUE(int,_sceCtrlPeekBufferPositive2, port, pad_data, count);

    if(pad_data)
    {
        SceCtrlData kernelPadData = {0};
        ksceKernelMemcpyUserToKernel((void *)&kernelPadData, (uintptr_t)pad_data, sizeof(SceCtrlData));
        kernelPadData.buttons |= inputState.controlData.buttons;

        if(isPlaying)
        {
            for(int i = 0; i < 16; i++)
            {
                *( ((unsigned char *)(&kernelPadData)) + 12 + i) = *( ((unsigned char *)(&inputState.controlData)) + 12 + i);
            }
        }

        ksceKernelMemcpyKernelToUser((uintptr_t)pad_data, (void *)&kernelPadData, sizeof(SceCtrlData));
    }
    return ret;
}

int captureThreadLogic(SceSize args, void *argp)
{
    printf("[MAFU-I] Started capture thread\n");
    
    InputState oldInputState = {0};
    InputState newInputState = {0};
    SceUInt64 captureStart;
    SceUInt64 currentTime;
    SceUID captureFile;

    oldInputState.controlData.buttons = recordKeycombo;

    //Reset timers
    captureStart = timers.digitalTick = timers.analogTick = timers.touchFrontTick = timers.touchBackTick = timers.accelTick = timers.gyroTick = ksceKernelGetSystemTimeWide();
    
    captureFile = ksceIoOpen("ur0:/data/MAFU-I/capture.dat", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if(captureFile < 0)
    {
        printf("[MAFU-I] Mafumafu is lacking inspiration. Error opening capture file for writing!\n");
        return -1;
    }

    while(isRecording)
    {
        currentTime = ksceKernelGetSystemTimeWide();

        if(timers.digitalDelay <= currentTime - timers.digitalTick)                     //Digital capture
        {
            ksceCtrlPeekBufferPositive(0, &newInputState.controlData, 1);             	//Peek digital buff

            if(newInputState.controlData.buttons != oldInputState.controlData.buttons)  //If buttons changed, record them with the timestamp.
            {                                                                           //TODO: look into the time in ScePadData struct
                CaptureInfo header = {0};

                header.type = INPUT_DIGITAL;
                header.captureTime = currentTime - captureStart;
                header.dataSize = sizeof(int);

                timers.digitalTick = currentTime;

                ksceIoWrite(captureFile, &header, sizeof(CaptureInfo));            //TODO: Check for errors!
                ksceIoWrite(captureFile, &newInputState.controlData.buttons, sizeof(int));            //TODO: Check for errors!

                memcpy(&oldInputState.controlData.buttons, &newInputState.controlData.buttons, sizeof(int));

                printf("[MAFU-I] Digital data captured at time offset %llu.\n", header.captureTime);
            }
        }

        if(timers.analogDelay <= currentTime - timers.analogTick)                  //Analog capture
        {
            unsigned char shouldUpdate = 0;
            unsigned char * newAnalogPointer = (unsigned char *)(&newInputState.controlData) + 12;     //Discard digital data
            unsigned char * oldAnalogPointer = (unsigned char *)(&oldInputState.controlData) + 12;     //Discard digital data

            ksceCtrlPeekBufferPositive(0, &newInputState.controlData, 1);          //Peek analog buff

            for(int i = 0; i < 16; i++)
            {
                if(*(newAnalogPointer + i) < *(oldAnalogPointer + i) - settings.analogThreshold
                || *(newAnalogPointer + i) > *(oldAnalogPointer + i) + settings.analogThreshold)
                 {
                     shouldUpdate = 1;
                     break;
                 }
            }

            if(shouldUpdate)                //If buttons changed, record them with the timestamp.
            {
                CaptureInfo header;
                header.type = INPUT_ANALOG;
                header.captureTime = currentTime - captureStart;
                header.dataSize = sizeof(SceCtrlData) - 12;

                ksceIoWrite(captureFile, &header, sizeof(CaptureInfo));                         //TODO: Check for errors!
                ksceIoWrite(captureFile, newAnalogPointer, header.dataSize);            //TODO: Check for errors!

                memcpy(oldAnalogPointer, newAnalogPointer, header.dataSize);

                printf("[MAFU-I] Analog data captured at time offset %llu. LX = %hhu LY = %hhu RX = %hhu RY = %hhu \n", header.captureTime, newInputState.controlData.lx, newInputState.controlData.ly, newInputState.controlData.rx, newInputState.controlData.ry);

            }
            timers.digitalTick = currentTime;
        }
        ksceKernelDelayThread(1000);    //Give other plugins/applications some time (i.e.: Adrenaline)
    }
    //Clean up and return
    ksceIoClose(captureFile);
    printf("[MAFU-I] Capture stopped\n");
    return 0;
}

int playbackThreadLogic(SceSize args, void *argp)
{
    printf("[MAFU-I] Started playback thread\n");

    CaptureInfo playbackHeader = {0};
    SceUInt64 playbackStart;
    SceUInt64 currentTime;
    SceUID captureFile;
    
    captureFile = ksceIoOpen("ur0:/data/MAFU-I/capture.dat", SCE_O_RDONLY, 0777);
    if(captureFile < 0)
    {
        printf("[MAFU-I] Mafumafu forgot how to read. Error opening capture file for reading!\n");
        return -1;
    }

    playbackStart = ksceKernelGetSystemTimeWide();

    while(isPlaying)
    {
        currentTime = ksceKernelGetSystemTimeWide();
        
        if(playbackHeader.captureTime <= (unsigned long long) (currentTime - playbackStart))  //It's time to read the next input
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
                    ksceIoRead(captureFile, (((unsigned char*)(&inputState.controlData)) + 12), playbackHeader.dataSize);
                } break;
                default:
                break;
            }

            if(ksceIoRead(captureFile, &playbackHeader, sizeof(CaptureInfo)) == 0)              //The file has reached the end, there are no more inputs.
            {                                                                                   //TODO: error checking?
                isPlaying = 0;
                printf("[MAFU-I] Mafumafu is tired of singing. Reached EOF\n");
            }
            else
            {
                printf("[MAFU-I] Next input frame is of type %#x, has a payload of size %#x and it's scheduled for %llu. Current time: %llu\n", playbackHeader.type, playbackHeader.dataSize, playbackHeader.captureTime + playbackStart, currentTime);
            }
            
        }
        ksceKernelDelayThread(1000);    //Give other plugins/applications some time (i.e.: Adrenaline)
    }
    //Clean up and return
    ksceIoClose(captureFile);
    memset(&inputState, 0, sizeof(InputState));
    printf("[MAFU-I] Playback stopped\n");
    return 0;
}

int mainThreadLogic(SceSize args, void *argp)
{
    printf("[MAFU-I] Main thread started!\n");
    SceUID playbackThreadId, captureThreadId;
    playbackThreadId = ksceKernelCreateThread("MAFU-I_playbackThread", playbackThreadLogic, 0x10000100, 0x10000, 0, 0, NULL);
    captureThreadId = ksceKernelCreateThread("MAFU-I_captureThread", captureThreadLogic, 0x10000100, 0x10000, 0, 0, NULL);
    SceCtrlData padData = {0};
    SceCtrlData previousPadData = {0};

    ksceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);

    while(1)
    {
        ksceCtrlPeekBufferPositive(0, &padData, 1);
        
        if(!(isRecording) && (padData.buttons == playKeycombo) && !(previousPadData.buttons == playKeycombo))
        {
            printf("[MAFU-I] Playback keycombo intercepted!\n");

            isPlaying = !isPlaying;

            if(isPlaying)
            {
                ksceKernelStartThread(playbackThreadId, 0, 0);
            }
            
        }
        
        if(!(isPlaying) && (padData.buttons == recordKeycombo) && !(previousPadData.buttons == recordKeycombo))
        {
            printf("[MAFU-I] Record keycombo intercepted!\n");

            isRecording = !isRecording;

            if(isRecording)
            {
                ksceKernelStartThread(captureThreadId, 0, 0);
            }
        }

        memcpy(&previousPadData, &padData, sizeof(SceCtrlData));
        ksceKernelDelayThread(1000);    //Give other plugins/applications some time (i.e.: Adrenaline)
    }
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
    printf("[MAFU-I] Started!\n");

    ksceIoMkdir("ur0:/data/MAFU-I", 0777);

    settings.analogThreshold = 5;

    taiHookFunctionExportForKernel(KERNEL_PID, &_sceCtrlPeekBufferPositive, "SceCtrl", 0xD197E3C7, 0xA9C3CED6, h_sceCtrlPeekBufferPositive);
    taiHookFunctionExportForKernel(KERNEL_PID, &_sceCtrlPeekBufferPositive2, "SceCtrl", 0xD197E3C7, 0x15F81E8C, h_sceCtrlPeekBufferPositive2);

    SceUID mainThreadId;
    mainThreadId = ksceKernelCreateThread("MAFU-I_mainThread", mainThreadLogic, 0x10000100, 0x10000, 0, 0, NULL);
    ksceKernelStartThread(mainThreadId, 0, 0);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
    //TODO: free hooks, stop threads and stuff
	return SCE_KERNEL_STOP_SUCCESS;
}