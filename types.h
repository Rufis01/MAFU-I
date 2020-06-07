#ifndef _INPUTTYPES
#define _INPUTTYPES
typedef enum
{
    INPUT_NONE,
    INPUT_DIGITAL,
    INPUT_ANALOG,
    //How to implement these???
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
    CaptureInfo header;
    unsigned int reportNumber;
    /*Followed by data*/
} __attribute__((__packed__)) TouchInfo;

typedef struct
{
    SceCtrlData controlData;
} InputState;

typedef struct
{
    unsigned long long digitalDelay;
    unsigned long long digitalTick;     //last
    unsigned long long analogDelay;
    unsigned long long analogTick;      //last
    unsigned long long touchFrontDelay;
    unsigned long long touchFrontTick;  //last
    unsigned long long touchBackDelay;
    unsigned long long touchBackTick;   //last
    unsigned long long accelDelay;
    unsigned long long accelTick;       //last
    unsigned long long gyroDelay;
    unsigned long long gyroTick;        //last
} Timers;

typedef struct
{
    unsigned short touchFrontThresholdX;
    unsigned short touchFrontThresholdY;
    unsigned short touchFrontThresholdZ;
    unsigned short touchBackThresholdX;
    unsigned short touchBackThresholdY;
    unsigned short touchBackThresholdZ;

    unsigned short analogThreshold;
} Settings;


#endif