#include "os3k_keyboard.h"

char TranslateKeyToChar(KeyMod_e key)
{
    char c;
    c = _OS3K_TranslateKeyToChar(key);
    if(!c) {
        key &= ~KEY_MOD_CAPS_LOCK;
        if(key == (KeyMod_e)KEY_APPLETS) {
            // Invokes the applets menu and does not return.
            SystemHandleEvents(SYSTEM_EVENT_SPECIAL_KEYS, key);
        } else if(key == (KEY_MOD_CTRL | KEY_C)) {
            c = '\x03';
        } else if(key == (KeyMod_e)KEY_BACKSPACE) {
            c = '\b';
        }
    }
    return c;
}
