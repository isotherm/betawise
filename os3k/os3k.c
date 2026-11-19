#include "os3k_display_internal.h"

void BwProcessMessage(Message_e message, uint32_t param, uint32_t* status) {
    _DisplayProcessMessage(message, status);
    ProcessMessage(message, param, status);
}

bool CallSysInt(uint32_t unused_zero, SysInt_e info, void* data) {
    if(_DisplayQuery(info, data)) {
        return true;
    }
    return _OS3K_CallSysInt(unused_zero, info, data);
}
