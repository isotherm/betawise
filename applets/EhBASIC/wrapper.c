#include "os3k.h"

APPLET_HEADER_BEGIN
    APPLET_ID(0xA1BA)
    APPLET_NAME("EhBASIC")
    APPLET_INFO("Copyright(C) 2002-12 Lee Davison")
    APPLET_VERSION(BETAWISE_VERSION_MAJOR, BETAWISE_VERSION_MINOR, BETAWISE_VERSION_REVISION)
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

extern void EhBASIC_Init();
extern void EhBASIC_Resume();

void ProcessMessage(Message_e message, uint32_t param, uint32_t* status) {
    *status = 0;
    switch(message) {
        case MSG_INIT:
            EhBASIC_Init();
            break;

        case MSG_SETFOCUS:
            ClearScreen();
            SetCursorMode(CURSOR_MODE_SHOW);
            EhBASIC_Resume();
            break;
    }
}
