#ifndef _TYPES
#define _TYPES

#include <psp2/touch.h>
#include <psp2kern/ctrl.h>
#include <psp2kern/kernel/iofilemgr.h> 

typedef enum
{
	INPUT_NONE,
	INPUT_DIGITAL,
	INPUT_ANALOG,
	INPUT_TOUCHPAD_FRONT,
	INPUT_TOUCHPAD_BACK,
	INPUT_SENSOR_ACCEL,
	INPUT_SENSOR_GYRO,
	INPUT_OTHER
} inputType;

typedef struct
{
	inputType type;
	unsigned long long captureTime;
	unsigned int dataSize;
} __attribute__((__packed__)) CaptureInfo;

typedef struct
{
	SceCtrlData controlData;
	SceTouchData frontTouchData;
	SceTouchData backTouchData;
} InputState;

typedef struct
{
	unsigned long long digitalDelay;
	unsigned long long digitalTick;
	unsigned long long analogDelay;
	unsigned long long analogTick;
	unsigned long long frontTouchDelay;
	unsigned long long frontTouchTick;
	unsigned long long backTouchDelay;
	unsigned long long backTouchTick;
	unsigned long long accelDelay;
	unsigned long long accelTick;
	unsigned long long gyroDelay;
	unsigned long long gyroTick;
} Timers;

typedef struct
{
	SceBool repeat;
	unsigned short frontTouchThreshold;
	unsigned short backTouchThreshold;
	unsigned short analogThreshold;
	unsigned int recordKeycombo;
	unsigned int playKeycombo;
} Settings;


#endif