#include "betawise.h"

APPLET_HEADER_BEGIN
    APPLET_ID(0xA1A0)
    APPLET_NAME("Hello World")
    APPLET_INFO("Copyright (c) 2021 Alpaxo Software")
    APPLET_VERSION(BETAWISE_VERSION_MAJOR, BETAWISE_VERSION_MINOR, BETAWISE_VERSION_REVISION)
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

GLOBAL_DATA_BEGIN
GLOBAL_DATA_END

void ProcessMessage(Message_e message, uint32_t param, uint32_t* status) {
    *status = 0;
    switch(message) {
        case MSG_SETFOCUS:
            ClearScreen();
            SetCursorMode(CURSOR_MODE_HIDE);
            PutStringCentered(1, "hello, world");
            break;
    }
}
