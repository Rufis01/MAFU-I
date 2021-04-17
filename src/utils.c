#include "utils.h"
#include "globals.h"

void mergePadData(SceCtrlData* userData, SceCtrlData* kernelData)
{
	if(userData)
	{
		SceCtrlData mergedData = {0};
		ksceKernelMemcpyUserToKernel((void *)&mergedData, (uintptr_t)userData, sizeof(SceCtrlData));             //Copy pad data from user(app) to kernel(plugin)

		if(isPlaying)
		{
			mergedData.buttons |= kernelData->buttons;                                                    //XOR kernel data with capture data (we don't wanna override the buttons)
			
			for(int i = 0; i < 16; i++)
			{
				*( ((unsigned char *)(&mergedData)) + 12 + i) = *( ((unsigned char *)(kernelData)) + 12 + i);   //Copy analog button data
			}
		}

		ksceKernelMemcpyKernelToUser((uintptr_t)userData, (void *)&mergedData, sizeof(SceCtrlData));             //Copy edited data back to user(app)
	}
}

void mergeTouchData(SceTouchData* userData, SceTouchData* kernelData, SceUInt32 port)
{
	if(userData)
	{
		SceTouchData mergedData = {0};
		ksceKernelMemcpyUserToKernel((void *)&mergedData, (uintptr_t)userData, sizeof(SceTouchData));

		if(isPlaying)
		{
			for(int i = 0; i < kernelData->reportNum; i++)
			{
				if(mergedData.reportNum >= (port == SCE_TOUCH_PORT_FRONT ? 6 : 4))
				{
					break;
				}
				memcpy(&mergedData.report[mergedData.reportNum++], &kernelData->report[i], sizeof(SceTouchReport));
			}
		}
		
		ksceKernelMemcpyKernelToUser((uintptr_t)userData, (void *)&mergedData, sizeof(SceTouchData));
	}
}