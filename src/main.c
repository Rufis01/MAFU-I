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

*/

//Dejavu's PayloadArguments pargs is at offset 0xb04c of segment 1

#include <psp2/touch.h>
#include <psp2kern/ctrl.h>
#include <psp2kern/display.h> 
#include <psp2kern/kernel/iofilemgr.h> 
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/cpu.h>

#include <taihen/taihen.h>
#include <taihen/patches.h>
#include <taihen/module.h>

#include <stdio.h>
#include <string.h>

#include "types.h"

#include "utils.h"
#include "hooks.h"
#include "threads/mainthread.h"
#include "threads/record.h"
#include "threads/playback.h"

#define printf(...) ksceDebugPrintf("[MAFU-I] " __VA_ARGS__)

int shellPid = 0;
SceUInt64 ssfileElapsedTime = 0;
SceOff ssfileOffset = 0;
SceBool isRecording = 0;
SceBool isPlaying = 0;
SceUID playbackThreadId = 0;
SceUID captureThreadId = 0;
Timers timers = {0};
Settings settings = {0};
InputState inputState = {0};

SceUID mainThreadId = 0;

SceUID touchPatchID = 0;
tai_hook_ref_t hookRefs[NUM_HOOKS];
SceUID patchRefs[NUM_HOOKS];

int h_vshKernelSendSysEvent(int a1);
int h_ksceDisplaySetFrameBufInternal(int head, int index, const SceDisplayFrameBuf *pParam, int sync);

CTRL_POSITIVE_HOOK(hookRefs, sceCtrlPeekBufferPositive, &inputState.controlData)
CTRL_POSITIVE_HOOK(hookRefs, sceCtrlPeekBufferPositiveExt, &inputState.controlData)
CTRL_POSITIVE_HOOK(hookRefs, sceCtrlPeekBufferPositive2, &inputState.controlData)
CTRL_POSITIVE_HOOK(hookRefs, sceCtrlPeekBufferPositiveExt2, &inputState.controlData)
CTRL_POSITIVE_HOOK(hookRefs, sceCtrlReadBufferPositive, &inputState.controlData)
CTRL_POSITIVE_HOOK(hookRefs, sceCtrlReadBufferPositiveExt, &inputState.controlData)
CTRL_POSITIVE_HOOK(hookRefs, sceCtrlReadBufferPositive2, &inputState.controlData)
CTRL_POSITIVE_HOOK(hookRefs, sceCtrlReadBufferPositiveExt2, &inputState.controlData)

TOUCH_HOOK(hookRefs, sceTouchPeek, inputState)
TOUCH_HOOK(hookRefs, sceTouchPeek2, inputState)
TOUCH_HOOK(hookRefs, sceTouchRead, inputState)
TOUCH_HOOK(hookRefs, sceTouchRead2, inputState)

TOUCH_REGION_HOOK(hookRefs, sceTouchPeekRegion, inputState)

void initHooks()
{
	REGISTER_HOOK(patchRefs, hookRefs, "SceCtrl", 0xD197E3C7, 0xA9C3CED6, sceCtrlPeekBufferPositive);
	REGISTER_HOOK(patchRefs, hookRefs, "SceCtrl", 0xD197E3C7, 0x15F81E8C, sceCtrlPeekBufferPositive2);
	REGISTER_HOOK(patchRefs, hookRefs, "SceCtrl", 0xD197E3C7, 0xA59454D3, sceCtrlPeekBufferPositiveExt);
	REGISTER_HOOK(patchRefs, hookRefs, "SceCtrl", 0xD197E3C7, 0x860BF292, sceCtrlPeekBufferPositiveExt2);

	REGISTER_HOOK(patchRefs, hookRefs, "SceCtrl", 0xD197E3C7, 0x67E7AB83, sceCtrlReadBufferPositive);
	REGISTER_HOOK(patchRefs, hookRefs, "SceCtrl", 0xD197E3C7, 0xC4226A3E, sceCtrlReadBufferPositive2);
	REGISTER_HOOK(patchRefs, hookRefs, "SceCtrl", 0xD197E3C7, 0xE2D99296, sceCtrlReadBufferPositiveExt);
	REGISTER_HOOK(patchRefs, hookRefs, "SceCtrl", 0xD197E3C7, 0xA7178860, sceCtrlReadBufferPositiveExt2);
	
	REGISTER_HOOK(patchRefs, hookRefs, "SceTouch", 0x3E4F4A81, 0xFF082DF0, sceTouchPeek);
	REGISTER_HOOK(patchRefs, hookRefs, "SceTouch", 0x3E4F4A81, 0x3AD3D0A1, sceTouchPeek2);
	REGISTER_HOOK(patchRefs, hookRefs, "SceTouch", 0x3E4F4A81, 0x169A1D58, sceTouchRead);
	REGISTER_HOOK(patchRefs, hookRefs, "SceTouch", 0x3E4F4A81, 0x39401BEA, sceTouchRead2);

	//REGISTER_HOOK(patchRefs, hookRefs, "SceTouch", 0x3E4F4A81, 0x04440622, sceTouchPeekRegion);
	//REGISTER_HOOK(patchRefs, hookRefs, "SceTouch", 0x3E4F4A81, 0x2CF6D7E2, sceTouchPeekRegionExt);

	REGISTER_HOOK(patchRefs, hookRefs, "SceDisplay", 0x9FED47AC, 0x16466675, ksceDisplaySetFrameBufInternal);

	REGISTER_HOOK(patchRefs, hookRefs, "SceVshBridge", 0x35C5ACD4, 0x71D9DB5C, vshKernelSendSysEvent);
}

void loadSettings()
{	
	settings.recordKeycombo = (SCE_CTRL_LEFT | SCE_CTRL_CIRCLE | SCE_CTRL_RTRIGGER);
	settings.playKeycombo = (SCE_CTRL_RIGHT | SCE_CTRL_CIRCLE | SCE_CTRL_RTRIGGER);
	settings.savestateKeycombo = (SCE_CTRL_LEFT | SCE_CTRL_TRIANGLE | SCE_CTRL_RTRIGGER);
	settings.loadstateKeycombo = (SCE_CTRL_RIGHT | SCE_CTRL_TRIANGLE | SCE_CTRL_RTRIGGER);
	settings.analogThreshold = 5;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
	SceCtrlData padData = {0};
	ksceCtrlPeekBufferPositive(0, &padData, 1);
	if(padData.buttons & SCE_CTRL_CROSS)
	{
			printf("Disabled!\n");
			return SCE_KERNEL_START_SUCCESS;
	}

	printf("Started!\n");

	ksceIoMkdir("ur0:/data/MAFU-I", 0777);
	ksceIoMkdir("ux0:/data/MAFU-I", 0777);

	initHooks();
	/*for(int i=0; i<NUM_HOOKS; i++)
	{
		printf("%d\n", patchRefs[i]);
	}*/
	loadSettings();

	//Patch for touch in kernel
	tai_module_info_t modinfo = {0};
	modinfo.size = sizeof(tai_module_info_t);
	module_get_by_name_nid(KERNEL_PID, "SceTouch", TAI_IGNORE_MODULE_NID, &modinfo);
	touchPatchID = taiInjectDataForKernel(KERNEL_PID, modinfo.modid, 0, 0x32CE, "\x00\xbf\x00\xbf", 4);

	mainThreadId = ksceKernelCreateThread("MAFU-I_mainThread", mainThreadLogic, 0x3C, 0x3000, 0, 0x10000, 0);

	ksceKernelStartThread(mainThreadId, 0, 0);

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	//TODO: free hooks, stop threads and stuff
	taiInjectReleaseForKernel(touchPatchID);
	return SCE_KERNEL_STOP_SUCCESS;
}

int h_vshKernelSendSysEvent(int a1)
{
	int ret = TAI_CONTINUE(int, hookRefs[vshKernelSendSysEvent_id], a1);
	shellPid = ksceKernelGetProcessId();
	return ret;
}

int h_ksceDisplaySetFrameBufInternal(int head, int index, const SceDisplayFrameBuf *pParam, int sync)
{
	/*if(pParam)
	{
		if(pParam->base)
		{
			for(int row = 0; row < MENU_HEIGHT; row++)
			{
				int offsety = (pParam->height - MENU_HEIGHT) / 2;
				int offsetx = (pParam->width - MENU_WIDTH) / 2;
				int offsettot = (offsety * (pParam->pitch) + offsetx);
				ksceKernelMemcpyKernelToUser(pParam->base + offset * 40 + (offsettot + ((pParam->pitch) * row)) * 4, test + (MENU_WIDTH * row), MENU_WIDTH*4);
			}
		}
	}*/
	return TAI_CONTINUE(int, hookRefs[ksceDisplaySetFrameBufInternal_id], head, index, pParam, sync);
}