//HAHA, global variables go brrrr

#include "types.h"

extern int shellPid;
extern SceUInt64 ssfileElapsedTime;
extern SceOff ssfileOffset;
extern SceBool isRecording;
extern SceBool isPlaying;
extern SceUID playbackThreadId;
extern SceUID captureThreadId;
extern Timers timers;
extern Settings settings;
extern InputState inputState;
extern PayloadArguments* dejavu_pargs;

extern void send_notification_request(const char *msg);