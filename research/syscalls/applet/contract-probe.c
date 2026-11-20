/* Compile against the public narrow-return declarations, then run in original ROM. */
#include "os3k_applet.h"
#include "os3k_file.h"
#ifndef RESULT_ADDRESS
#error RESULT_ADDRESS required
#endif
void contract_width_probe(void)
{
    volatile uint32_t *result = (volatile uint32_t *)RESULT_ADDRESS;
    char name[37];
    uint32_t status = 0;
    result[0] = AppletGetName(0, name);
    result[1] = AppletSendMessage(0, MSG_KEY, 0, &status);
    result[2] = AppletGetContextId();
    AppletClearContext();
    result[3] = AppletGetContextId();
    result[4] = ClipboardGetFormat();
    result[5] = 0x70617373;
}
