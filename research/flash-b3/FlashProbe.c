#include "os3k.h"
#include "../../applets/FileManager/Flash.h"

APPLET_HEADER_BEGIN
    APPLET_ID(0xA1FF)
    APPLET_NAME("Flash validation probe")
    APPLET_VERSION(0, 1, "a")
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

/* Inert until the emulator explicitly arms it in RAM. Never install on hardware. */
volatile uint32_t g_probeArm;
volatile uint32_t g_probeDone;
volatile int32_t g_results[24];
Flash_t g_flash;
uint8_t g_data[258] __attribute__((aligned(2)));

static void Probe(void) {
    FlashBlock_t block;
    unsigned i;
    g_results[0] = FlashOpen(&g_flash);
    if(g_results[0] != FLASH_OK) { g_probeDone = 1; return; }
    for(i = 0; i < sizeof g_data; ++i) g_data[i] = (i * 13) & 0x7F;
    g_results[1] = FlashProgram(&g_flash, g_flash.begin + 1, g_data + 1, 257, 1000);
    g_results[2] = g_flash.completed;
    /* No new programming pulses when content is unchanged. */
    g_results[3] = FlashProgram(&g_flash, g_flash.begin + 1, g_data + 1, 257, 1000);
    g_data[1] = 0xFF;
    g_results[4] = FlashProgram(&g_flash, g_flash.begin + 1, g_data + 1, 1, 1000);
    g_results[5] = FlashEraseBlock(&g_flash, g_flash.begin + 1, 1000);
    g_results[6] = FlashEraseBlock(&g_flash, 0x400000, 1000);
    g_results[7] = FlashProgram(&g_flash, g_flash.end - 1, g_data, 2, 1000);
    g_results[8] = FlashEraseBlock(&g_flash, g_flash.begin, 1000);
    g_results[9] = g_flash.completed;
    g_results[10] = FlashRead(&g_flash, g_flash.begin, g_data, sizeof g_data);
    for(i = 0; i < sizeof g_data; ++i) if(g_data[i] != 0xFF) g_results[10] = -99;
    /* Error/status/delay behavior is injected by the model for these bytes. */
    g_data[0] = 0x55;
    g_results[11] = FlashProgram(&g_flash, g_flash.begin + 0x400, g_data, 1, 1000);
    g_results[12] = g_flash.status;
    g_results[13] = FlashProgram(&g_flash, g_flash.begin + 0x401, g_data, 1, 1);
    g_results[14] = g_flash.waitExceeded;
    g_results[15] = FlashProgram(&g_flash, g_flash.begin + 0x402, g_data, 1, 1000);
    g_results[16] = FlashGetBlock(0x89D3, 0xFFFF, &block);
    g_results[17] = block.offset;
    g_results[18] = block.length;
    g_results[19] = FlashGetBlock(0x89D2, 0xF1234, &block);
    g_results[20] = block.offset;
    g_results[21] = block.length;
    g_probeDone = 1;
}

void ProcessMessage(Message_e message, uint32_t param, uint32_t* status) {
    *status = 0;
    if(message == MSG_SETFOCUS) {
        ClearScreen();
        PutString("Flash probe: emulator only");
    }
    if(message == MSG_CHAR && param == 'p' && g_probeArm == 0xB3A50001 && !g_probeDone) Probe();
}
