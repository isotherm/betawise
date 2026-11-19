#include "os3k.h"
#define RESULT ((volatile uint32_t*)0x71c00)
static void callback(SpeechEvent_e event, void* userData)
{
    RESULT[1] = event;
    RESULT[2] = (uint32_t)userData;
}
void typed_probe(void)
{
    uint32_t errors = 0;
    if (SpeechControl(SPEECH_DISABLE) != 1) ++errors;
    if (SpeechEnqueue(SPEECH_NOTIFY_STARTED | SPEECH_NOTIFY_FINISHED,
                      "typed", callback, (void*)0x12345678) != SPEECH_QUEUE_DISABLED) ++errors;
    if (SpeechControl(SPEECH_ENABLE) != 0) ++errors;
    if (SpeechEnqueue(SPEECH_NOTIFY_STARTED | SPEECH_NOTIFY_FINISHED,
                      "typed", callback, (void*)0x12345678) != SPEECH_QUEUE_OK) ++errors;
    if (SpeechControl(SPEECH_QUEUE_IS_EMPTY) != 0) ++errors;
    if (WirelessGetLastReceiveStatus() != 0xff42) ++errors;
    RESULT[0] = errors ? errors : 0x70617373;
}
