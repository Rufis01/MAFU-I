#ifndef _HOOKS
#define _HOOKS

#include "types.h"

enum hookId
{
	vshKernelSendSysEvent_id = 0,
	ksceDisplaySetFrameBufInternal_id,
	sceCtrlPeekBufferPositive_id,
	sceCtrlPeekBufferPositiveExt_id,
	sceCtrlPeekBufferPositive2_id,
	sceCtrlPeekBufferPositiveExt2_id,
	sceCtrlReadBufferPositive_id,
	sceCtrlReadBufferPositiveExt_id,
	sceCtrlReadBufferPositive2_id,
	sceCtrlReadBufferPositiveExt2_id,
	sceTouchPeek_id,
	sceTouchPeek2_id,
	sceTouchRead_id,
	sceTouchRead2_id,
	sceTouchPeekRegion_id,
	NUM_HOOKS
};

#define REGISTER_HOOK(patchRefs, hookRefs, module, libNid, funNid, funName)\
patchRefs[funName##_id] = taiHookFunctionExportForKernel(KERNEL_PID, (tai_hook_ref_t *) &hookRefs[funName##_id], module, libNid, funNid, h_##funName)

#define CTRL_POSITIVE_HOOK(hookRefs, name, kpadData)\
int h_##name(int port, SceCtrlData* pad_data, int count)\
{\
	int ret = TAI_CONTINUE(int,hookRefs[name##_id], port, pad_data, count);\
	mergePadData(pad_data, kpadData);\
	return ret;\
}

#define TOUCH_HOOK(hookRefs, name, inputState)\
int h_##name(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs)\
{\
	int ret = TAI_CONTINUE(int,hookRefs[name##_id], port, pData, nBufs);\
	SceTouchData* buffer = (port == SCE_TOUCH_PORT_FRONT ? &inputState.frontTouchData : &inputState.backTouchData);\
	mergeTouchData(pData, buffer, port);\
	return ret;\
}

#define TOUCH_REGION_HOOK(hookRefs, name, inputState)\
int h_##name(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, int region)\
{\
	int ret = TAI_CONTINUE(int,hookRefs[name##_id], port, pData, nBufs, region);\
	SceTouchData* buffer = (port == SCE_TOUCH_PORT_FRONT ? &inputState.frontTouchData : &inputState.backTouchData);\
	mergeTouchData(pData, buffer, port);\
	return ret;\
}

#endif
