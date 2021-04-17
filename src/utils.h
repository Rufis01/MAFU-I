#ifndef _UTILS
#define _UTILS

#include "types.h"

void mergePadData(SceCtrlData* userData, SceCtrlData* kernelData);
void mergeTouchData(SceTouchData* userData, SceTouchData* kernelData, SceUInt32 port);

#endif