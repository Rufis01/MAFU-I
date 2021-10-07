#ifndef _TYPES
#define _TYPES

#include <psp2/touch.h>
#include <psp2kern/ctrl.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/kernel/sysclib.h> 

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
	unsigned int savestateKeycombo;
	unsigned int loadstateKeycombo;
} Settings;

typedef enum
{
	SS_NONE = 0,
	SS_REC,
	SS_PLAY
}SavestateState;

typedef struct
{
	int state;
	SceOff fileOffset;
	SceUInt64 elapsedTime;
} __attribute__((__packed__)) SavestateInfo;

//DEJAVU STUFF
#define SCE_RTC_CTX_SIZE 0x1c0

typedef struct {
	uint32_t mode;
	uint32_t fattime;
	uint32_t process_num;
	uint32_t ttbs[32];
	char titleid[32][32];
	char rtc_ctx[SCE_RTC_CTX_SIZE];
	char path[256];
} PayloadArguments;

enum DejavuModes {
	DEJAVU_MODE_NONE,
	DEJAVU_MODE_LOAD,
	DEJAVU_MODE_SAVE,
};

#endif