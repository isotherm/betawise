#include "os3k.h"

// We intermix stdio and dialog calls. Need to plan how to resovle this.
extern void _OS3K_SetCursor(uint8_t row, uint8_t col, CursorMode_e cursor_mode);

enum { SCRATCH_SIZE = 256 };
enum { BUFFER_COUNT = 7 };
enum { BUFFER_INPUT = 13 };
enum { BUFFER_SIZE = 1 + BUFFER_INPUT + 2 };
enum { BYTES_PER_ROW = 8 };

typedef enum _Mode_e {
    MODE_NIBBLE_HI = 0,
    MODE_NIBBLE_LO = 1,
    MODE_ASCII = 2
} Mode_e;

#define BUS_ERROR_HANDLER_PTR *((void(**)())8)

APPLET_HEADER_BEGIN
    APPLET_ID(0xA1DE)
    APPLET_NAME("Debugging Tool")
    APPLET_INFO("Copyright (c) 2020 Alpaxo Software")
    APPLET_VERSION(BETAWISE_VERSION_MAJOR, BETAWISE_VERSION_MINOR, BETAWISE_VERSION_REVISION)
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

uint8_t g_scratch[SCRATCH_SIZE];
char g_buffer[BUFFER_COUNT][BUFFER_SIZE];
volatile uint8_t* g_pAddress;
volatile uint8_t* g_pPrevAddress;
void (*g_prevBusErrorHandler)();
uint16_t g_rawSysCall;
Mode_e g_mode;
short g_cursor;
uint8_t g_rowsPerScreen;
uint8_t g_bytesPerScreen;
volatile uint8_t g_busError;
bool g_isAS3000;

void BusErrorHandler() {
    g_busError = 1;
    asm("add.w #8,%sp");
    asm("rte");
    __builtin_unreachable();
}

void InstallBusErrorHandler() {
    g_prevBusErrorHandler = BUS_ERROR_HANDLER_PTR;
    BUS_ERROR_HANDLER_PTR = &BusErrorHandler;
}

void UninstallBusErrorHandler() {
    BUS_ERROR_HANDLER_PTR = g_prevBusErrorHandler;
}

void DumpSetCursor(char offset, Mode_e mode, CursorMode_e cursor_mode) {
    char row = 1 + (offset / BYTES_PER_ROW);
    char col = (offset % BYTES_PER_ROW);
    if(mode == MODE_ASCII) {
        col += BYTES_PER_ROW * 3;
    } else {
        col *= 3;
        if(mode == MODE_NIBBLE_LO) {
            col++;
        }
    }
    col += g_isAS3000 ? 9 : 12;
    SetCursor(row, col, cursor_mode);
}

void DumpSetCursorCur() {
    DumpSetCursor(g_cursor, g_mode, CURSOR_MODE_SHOW);
}

void DumpRedrawByteHex(uint8_t c, char error) {
    char buffer[4];
    sprintf(buffer, error ? "\xd7\xd7 " : "%02X ", c);
    puts(buffer);
}

void DumpRedrawByteAscii(uint8_t c, char error) {
    if(error) {
        c = '\xd7'; // central X (multiply)
    } else if(c < 0x20) {
        c = g_isAS3000 ? '\x08' : '\xb7'; // small dot
    }
    if(g_isAS3000) {
        // If we use the stdio function, the screen scrolls.
        PutChar(c);
    } else {
        putchar(c);
    }
}

void DumpWriteAndRedrawCur(uint8_t value, uint8_t mask) {
    volatile uint8_t* p = &g_pAddress[g_cursor];
    InstallBusErrorHandler();
    g_busError = 0;
    value = (*p & mask) | value;
    if(!mask || !g_busError) {
        *p = value;
    }
    g_busError = 0;
    uint8_t c = *p;
    UninstallBusErrorHandler();
    DumpSetCursor(g_cursor, MODE_NIBBLE_HI, CURSOR_MODE_HIDE);
    DumpRedrawByteHex(c, g_busError);
    DumpSetCursor(g_cursor, MODE_ASCII, CURSOR_MODE_HIDE);
    DumpRedrawByteAscii(c, g_busError);
}

void DumpRedrawScreen() {
    uint8_t buffer[BYTES_PER_ROW > 10 ? BYTES_PER_ROW : 10];
    char busError[BYTES_PER_ROW];
    volatile uint8_t* pAddr = g_pAddress;
    InstallBusErrorHandler();
    for(char row = 1; row <= g_rowsPerScreen; row++) {
        SetCursor(row, g_isAS3000 ? 1 : 3, CURSOR_MODE_HIDE);
        sprintf(buffer, g_isAS3000 ? "%-08.7X" : "%08X ", pAddr);
        puts(buffer);
        for(char byte = 0; byte < BYTES_PER_ROW; byte++) {
            g_busError = 0;
            buffer[byte] = *pAddr++;
            busError[byte] = g_busError;
        }
        for(char byte = 0; byte < BYTES_PER_ROW; byte++) {
            DumpRedrawByteHex(buffer[byte], busError[byte]);
        }
        for(char byte = 0; byte < BYTES_PER_ROW; byte++) {
            DumpRedrawByteAscii(buffer[byte], busError[byte]);
        }
    }
    UninstallBusErrorHandler();
    DumpSetCursorCur();
}

void DumpMoveCursor(char cursor) {
    g_cursor = cursor;
    if(g_cursor < 0) {
        g_pAddress -= BYTES_PER_ROW;
        g_cursor += BYTES_PER_ROW;
        DumpRedrawScreen();
    } else if(g_cursor >= g_bytesPerScreen) {
        g_pAddress += BYTES_PER_ROW;
        g_cursor -= BYTES_PER_ROW;
        DumpRedrawScreen();
    } else {
        DumpSetCursorCur();
    }
}

void DumpSetAddress(uint32_t addr) {
    g_pPrevAddress = g_pAddress + g_cursor;
    g_pAddress = (volatile uint8_t*)(addr & ~(BYTES_PER_ROW - 1));
    g_cursor = addr & (BYTES_PER_ROW - 1);
    if(g_mode == MODE_NIBBLE_LO) {
        g_mode = MODE_NIBBLE_HI;
    }
}

char HexCharToNibble(char c) {
    if(c >= '0' && c <= '9') {
        return c - '0';
    } else if(c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else if(c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else {
        return -1;
    }
}

char NumberFromString(char* pBuffer, uint32_t* pNumber) {
    char c;
    char syscall = 0;
    uint32_t n = 0;

    if(pBuffer[0] == '$') {
        // Temporary string literal.
        *pNumber = (uint32_t)(pBuffer + 1);
        return 0;
    }

    if(pBuffer[0] == '!') {
        syscall = 1;
        pBuffer++;
    }
    if(pBuffer[0] == '0' && tolower(pBuffer[1]) == 'x') {
        pBuffer += 2;
        for(; c = *pBuffer; pBuffer++) {
            c = HexCharToNibble(c);
            if(c < 0) {
                break;
            }
            n = (n << 4) | c;
        }
    } else {
        for(; c = *pBuffer; pBuffer++) {
            if(c < '0' || c > '9') {
                break;
            }
            n = (n * 10) + (c - '0');
        }
    }

    if(syscall) {
        // Calculate system call opcode.
        g_rawSysCall = 0xA000 | ((n << 2) & 0x7fc);
        n = (uint32_t)&g_rawSysCall;
    } else if(pBuffer[0] == '+') {
        // Add scratch buffer pointer.
        n += (uint32_t)g_scratch;
        pBuffer++;
    }

    if(*pBuffer) {
        return -1;
    }
    *pNumber = n;
    return 0;
}

int NumberPrompt(char* pPrompt, char* pBuffer, short maxLen, char* pDefault) {
    char len;
    Key_e exitKey, exitKeys[] = { KEY_ESC, KEY_ENTER, KEY_NONE };
    uint32_t n;

    if(!pBuffer[0]) {
        strcpy(pBuffer, pDefault);
    }
    len = strlen(pBuffer);
    PutStringRaw(pPrompt);
    exitKey = TextBox(pBuffer, &len, maxLen - 1, exitKeys, 0) & ~KEY_MOD_CAPS_LOCK;
    if(exitKey == KEY_ESC) {
        return -2;
    }
    pBuffer[len] = 0;
    return NumberFromString(pBuffer, &n);
}

void ProcessMessage(Message_e message, uint32_t param, uint32_t* status) {
    uint32_t scratch;
    char buffer[16];

    *status = 0;
    switch(message) {
        case MSG_INIT:
            CallSysInt(0, SYS_INT_GET_HW_LEVEL, &scratch);
            g_isAS3000 = (scratch < 2);
            g_pAddress = g_pPrevAddress = 0x0;
            g_mode = MODE_NIBBLE_HI;
            g_cursor = 0;
            for(char i = 0; i < BUFFER_COUNT; i++) {
                g_buffer[i][0] = ' ';
                g_buffer[i][1] = '\0';
            }
            break;

        case MSG_SETFOCUS:
            CallSysInt(0, SYS_INT_GET_ROW_COUNT, &scratch);
            g_rowsPerScreen = scratch;
            g_bytesPerScreen = BYTES_PER_ROW * g_rowsPerScreen;
            ClearScreen();
            DumpRedrawScreen();
            break;

        case MSG_CHAR:
            if(param == '\t') {
                g_mode = (g_mode == MODE_ASCII ? MODE_NIBBLE_HI : MODE_ASCII);
                DumpSetCursorCur();
            } else if(param == '\r') {
                // Do nothing. Don't insert as text.
            } else if(g_mode == MODE_NIBBLE_HI) {
                char nibble = HexCharToNibble(param);
                if(nibble >= 0) {
                    DumpWriteAndRedrawCur(nibble << 4, 0x0F);
                    g_mode = MODE_NIBBLE_LO;
                    DumpSetCursorCur();
                }
            } else if(g_mode == MODE_NIBBLE_LO) {
                char nibble = HexCharToNibble(param);
                if(nibble >= 0) {
                    DumpWriteAndRedrawCur(nibble, 0xF0);
                    g_mode = MODE_NIBBLE_HI;
                    DumpMoveCursor(g_cursor + 1);
                }
            } else if(g_mode == MODE_ASCII) {
                DumpWriteAndRedrawCur(param, 0x00);
                DumpMoveCursor(g_cursor + 1);
            }
            break;

        case MSG_KEY:
            switch(param & ~KEY_MOD_CAPS_LOCK) {
                case KEY_LEFT:
                    if(g_mode == MODE_NIBBLE_LO) {
                        g_mode = MODE_NIBBLE_HI;
                        DumpSetCursorCur();
                    } else {
                        if(g_mode == MODE_NIBBLE_HI) {
                            g_mode = MODE_NIBBLE_LO;
                        }
                        DumpMoveCursor(g_cursor - 1);
                    }
                    break;

                case KEY_RIGHT:
                    if(g_mode == MODE_NIBBLE_HI) {
                        g_mode = MODE_NIBBLE_LO;
                        DumpSetCursorCur();
                    } else {
                        if(g_mode == MODE_NIBBLE_LO) {
                            g_mode = MODE_NIBBLE_HI;
                        }
                        DumpMoveCursor(g_cursor + 1);
                    }
                    break;

                case KEY_UP:
                    DumpMoveCursor(g_cursor - BYTES_PER_ROW);
                    break;

                case KEY_MOD_CTRL | KEY_UP:
                    g_pAddress -= g_bytesPerScreen;
                    DumpRedrawScreen();
                    break;

                case KEY_DOWN:
                    DumpMoveCursor(g_cursor + BYTES_PER_ROW);
                    break;
                    
                case KEY_MOD_CTRL | KEY_DOWN:
                    g_pAddress += g_bytesPerScreen;
                    DumpRedrawScreen();
                    break;

                case KEY_HOME:
                    DumpMoveCursor(g_cursor & ~(BYTES_PER_ROW - 1));
                    break;

                case KEY_MOD_CTRL | KEY_HOME:
                    DumpMoveCursor(g_cursor & ~(g_bytesPerScreen - 1));
                    break;

                case KEY_END:
                    DumpMoveCursor(g_cursor | (BYTES_PER_ROW - 1));
                    break;

                case KEY_MOD_CTRL | KEY_END:
                    DumpMoveCursor(g_cursor | (g_bytesPerScreen - 1));
                    break;

                case KEY_MOD_CTRL | KEY_I:
                    ClearScreen();
                    for(char choice = 1;;) {
                        uint32_t stack[BUFFER_COUNT];
                        uint8_t row, col;
                        DialogInit(0, 1, 4, 40);
                        DialogAddExitKey(KEY_ENTER);
                        DialogAddExitKey(KEY_ESC);
                        for(char i = 0; i < BUFFER_COUNT; i++) {
                            const int keys[] = {KEY_F, KEY_4, KEY_1, KEY_5, KEY_2, KEY_6, KEY_3};
                            char len = strlen((char*)g_buffer[i]);
                            int valid = !NumberFromString((char*)g_buffer[i] + 1, &stack[i]);
                            DialogAddItem((char*)g_buffer[i], len, valid ? ' ' : '\xd7', !valid, keys[i], -1);
                        }
                        DialogAddItem("all f(1,2,...)", 14, '\x10', 0, KEY_C, -1);
                        DialogSetChoice(choice);
                        DialogDraw();
                        KeyMod_e key = DialogRun() & ~KEY_MOD_CAPS_LOCK;
                        if(key == KEY_ESC) {
                            break;
                        }
                        if(DialogGetChoice() > BUFFER_COUNT) {
                            for(choice = 1; choice <= BUFFER_COUNT; choice++) {
                                if(DialogGetItemId(choice) != 0) {
                                    break;
                                }
                            }
                            if(choice <= BUFFER_COUNT) {
                                continue;
                            }
                            ClearScreen();
                            printf("Preparing to call *0x%08X(\n    0x%08X, 0x%08X, 0x%08X,\n    0x%08X, 0x%08X, 0x%08X)\n",
                                stack[0], stack[2], stack[4], stack[6], stack[1], stack[3], stack[5]);
                            uint32_t result = ((uint32_t(*)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t))stack[0])(
                                stack[2], stack[4], stack[6], stack[1], stack[3], stack[5]);
                            if(key & KEY_MOD_SHIFT) {
                                WaitForKey();
                            }
                            printf("Return value = 0x%08X (%d)", result, result);
                            WaitForKey();
                            break;
                        } else {
                            choice = DialogGetChoice();
                        }
                        GetCursorPos(&row, &col);
                        _OS3K_SetCursor(row, col + 6, CURSOR_MODE_HIDE);
                        NumberPrompt("", (char*)g_buffer[choice-1] + 1, BUFFER_INPUT, choice > 1 ? "0x" : "!");
                    }
                    DumpRedrawScreen();
                    break;

                case KEY_MOD_CTRL | KEY_G:
                    buffer[0] = 0;
                    for(;;) {
                        ClearRowCols(4, 1, g_isAS3000 ? 40 : 41);
                        _OS3K_SetCursor(4, 1, CURSOR_MODE_HIDE);
                        if(NumberPrompt("Go to address: ", buffer, sizeof(buffer), "0x") == -2) {
                            break;
                        }
                        if(NumberFromString(buffer, &scratch) == 0) {
                            DumpSetAddress(scratch);
                            break;
                        }
                    }
                    DumpRedrawScreen();
                    break;

                case KEY_MOD_CTRL | KEY_MOD_LEFTSHIFT | KEY_G:
                case KEY_MOD_CTRL | KEY_MOD_RIGHTSHIFT | KEY_G:
                    if(g_cursor & 1) {
                        break;
                    }
                    InstallBusErrorHandler();
                    g_busError = 0;
                    scratch = *(uint32_t*)(g_pAddress + g_cursor);
                    UninstallBusErrorHandler();
                    if(!g_busError) {
                        DumpSetAddress(scratch);
                    }
                    DumpRedrawScreen();
                    break;

                case KEY_BACKSPACE:
                    DumpSetAddress((uint32_t)g_pPrevAddress);
                    DumpRedrawScreen();
                    break;

                case KEY_MOD_CTRL | KEY_R:
                    DumpRedrawScreen();
                    break;

                case KEY_CLEAR_FILE:
                    memset((char*)g_scratch, 0, sizeof(g_scratch));
                    DumpRedrawScreen();
                    break;
            }
            break;
    }
}
