#include "os3k.h"

enum { SCRATCH_SIZE = 256 };
enum { BUFFER_COUNT = 7 };
enum { BUFFER_INPUT = 13 };
enum { BUFFER_SIZE = 1 + BUFFER_INPUT + 2 };

const uint8_t BYTES_PER_ROW = 8;
const uint8_t ADDRESS_COL = 3;
const uint8_t FIRST_BYTE_COL = 12;

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

GLOBAL_DATA_BEGIN
    uint8_t scratch[SCRATCH_SIZE];
    char buffer[BUFFER_COUNT][BUFFER_SIZE];
    volatile uint8_t* pAddress;
    volatile uint8_t* pPrevAddress;
    void (*prevBusErrorHandler)();
    volatile uint8_t busError;
    Mode_e mode;
    short cursor;
    uint8_t rowsPerScreen;
    uint8_t bytesPerScreen;
GLOBAL_DATA_END

void BusErrorHandler() {
    gd->busError = 1;
    asm("add.w #8,%sp");
    asm("rte");
    __builtin_unreachable();
}

void InstallBusErrorHandler() {
    gd->prevBusErrorHandler = BUS_ERROR_HANDLER_PTR;
    BUS_ERROR_HANDLER_PTR = &BusErrorHandler;
}

void UninstallBusErrorHandler() {
    BUS_ERROR_HANDLER_PTR = gd->prevBusErrorHandler;
}

void DumpSetCursor(char offset, Mode_e mode, CursorMode_e cursor_mode) {
    char row = 1 + (offset / BYTES_PER_ROW);
    char col = offset % BYTES_PER_ROW;
    if(mode == MODE_NIBBLE_HI) {
        col = FIRST_BYTE_COL + (col * 3);
    } else if(mode == MODE_NIBBLE_LO) {
        col = FIRST_BYTE_COL + 1 + (col * 3);
    } else if(mode == MODE_ASCII) {
        col = FIRST_BYTE_COL + (BYTES_PER_ROW * 3) + col;
    }
    BwSetCursor(row, col, cursor_mode);
}

void DumpSetCursorCur() {
    DumpSetCursor(gd->cursor, gd->mode, CURSOR_MODE_SHOW);
}

void DumpRedrawByteHex(uint8_t c, char error) {
    char buffer[4];
    sprintf(buffer, error ? "\xd7\xd7 " : "%02X ", c);
    BwPutString(buffer);
}

void DumpRedrawByteAscii(uint8_t c, char error) {
    BwPutCharRaw(error ? '\xd7' : c, CURSOR_MODE_HIDE);
}

void DumpWriteAndRedrawCur(uint8_t value, uint8_t mask) {
    volatile uint8_t* p = &gd->pAddress[gd->cursor];
    InstallBusErrorHandler();
    gd->busError = 0;
    value = (*p & mask) | value;
    if(!mask || !gd->busError) {
        *p = value;
    }
    gd->busError = 0;
    uint8_t c = *p;
    UninstallBusErrorHandler();
    DumpSetCursor(gd->cursor, MODE_NIBBLE_HI, CURSOR_MODE_HIDE);
    DumpRedrawByteHex(c, gd->busError);
    DumpSetCursor(gd->cursor, MODE_ASCII, CURSOR_MODE_HIDE);
    DumpRedrawByteAscii(c, gd->busError);
}

void DumpRedrawScreen() {
    uint8_t buffer[BYTES_PER_ROW > 10 ? BYTES_PER_ROW : 10];
    char busError[BYTES_PER_ROW];
    volatile uint8_t* pAddr = gd->pAddress;
    InstallBusErrorHandler();
    for(char row = 1; row <= gd->rowsPerScreen; row++) {
        BwSetCursor(row, ADDRESS_COL, CURSOR_MODE_HIDE);
        sprintf(buffer, "%08X ", pAddr);
        BwPutString(buffer);
        for(char byte = 0; byte < BYTES_PER_ROW; byte++) {
            gd->busError = 0;
            buffer[byte] = *pAddr++;
            busError[byte] = gd->busError;
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
    gd->cursor = cursor;
    if(gd->cursor < 0) {
        gd->pAddress -= BYTES_PER_ROW;
        gd->cursor += BYTES_PER_ROW;
        DumpRedrawScreen();
    } else if(gd->cursor >= gd->bytesPerScreen) {
        gd->pAddress += BYTES_PER_ROW;
        gd->cursor -= BYTES_PER_ROW;
        DumpRedrawScreen();
    } else {
        DumpSetCursorCur();
    }
}

void DumpSetAddress(uint32_t addr) {
    gd->pPrevAddress = gd->pAddress + gd->cursor;
    gd->pAddress = (volatile uint8_t*)(addr & ~(BYTES_PER_ROW - 1));
    gd->cursor = addr & (BYTES_PER_ROW - 1);
    if(gd->mode == MODE_NIBBLE_LO) {
        gd->mode = MODE_NIBBLE_HI;
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
        // Calculate system call index.
        n = (uint32_t)&ClearScreen + (n << 1);
    } else if(pBuffer[0] == '+') {
        // Add scratch buffer pointer.
        n += (uint32_t)gd->scratch;
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
    uint32_t addr;
    char buffer[16];

    *status = 0;
    switch(message) {
        case MSG_INIT:
            gd->pAddress = gd->pPrevAddress = 0;
            gd->mode = MODE_NIBBLE_HI;
            gd->cursor = 0;
            for(char i = 0; i < BUFFER_COUNT; i++) {
                gd->buffer[i][0] = ' ';
                gd->buffer[i][1] = '\0';
            }
            break;

        case MSG_SETFOCUS:
            BwGetScreenSize(&gd->rowsPerScreen, NULL);
            gd->bytesPerScreen = BYTES_PER_ROW * gd->rowsPerScreen;
            BwClearScreen();
            DumpRedrawScreen();
            break;

        case MSG_CHAR:
            if(param == '\t') {
                gd->mode = (gd->mode == MODE_ASCII ? MODE_NIBBLE_HI : MODE_ASCII);
                DumpSetCursorCur();
            } else if(param == '\r') {
                // Do nothing. Don't insert as text.
            } else if(gd->mode == MODE_NIBBLE_HI) {
                char nibble = HexCharToNibble(param);
                if(nibble >= 0) {
                    DumpWriteAndRedrawCur(nibble << 4, 0x0F);
                    gd->mode = MODE_NIBBLE_LO;
                    DumpSetCursorCur();
                }
            } else if(gd->mode == MODE_NIBBLE_LO) {
                char nibble = HexCharToNibble(param);
                if(nibble >= 0) {
                    DumpWriteAndRedrawCur(nibble, 0xF0);
                    gd->mode = MODE_NIBBLE_HI;
                    DumpMoveCursor(gd->cursor + 1);
                }
            } else if(gd->mode == MODE_ASCII) {
                DumpWriteAndRedrawCur(param, 0x00);
                DumpMoveCursor(gd->cursor + 1);
            }
            break;

        case MSG_KEY:
            switch(param & ~KEY_MOD_CAPS_LOCK) {
                case KEY_LEFT:
                    if(gd->mode == MODE_NIBBLE_LO) {
                        gd->mode = MODE_NIBBLE_HI;
                        DumpSetCursorCur();
                    } else {
                        if(gd->mode == MODE_NIBBLE_HI) {
                            gd->mode = MODE_NIBBLE_LO;
                        }
                        DumpMoveCursor(gd->cursor - 1);
                    }
                    break;

                case KEY_RIGHT:
                    if(gd->mode == MODE_NIBBLE_HI) {
                        gd->mode = MODE_NIBBLE_LO;
                        DumpSetCursorCur();
                    } else {
                        if(gd->mode == MODE_NIBBLE_LO) {
                            gd->mode = MODE_NIBBLE_HI;
                        }
                        DumpMoveCursor(gd->cursor + 1);
                    }
                    break;

                case KEY_UP:
                    DumpMoveCursor(gd->cursor - BYTES_PER_ROW);
                    break;

                case KEY_MOD_CTRL | KEY_UP:
                    gd->pAddress -= gd->bytesPerScreen;
                    DumpRedrawScreen();
                    break;

                case KEY_DOWN:
                    DumpMoveCursor(gd->cursor + BYTES_PER_ROW);
                    break;

                case KEY_MOD_CTRL | KEY_DOWN:
                    gd->pAddress += gd->bytesPerScreen;
                    DumpRedrawScreen();
                    break;

                case KEY_HOME:
                    DumpMoveCursor(gd->cursor & ~(BYTES_PER_ROW - 1));
                    break;

                case KEY_MOD_CTRL | KEY_HOME:
                    DumpMoveCursor(gd->cursor & ~(gd->bytesPerScreen - 1));
                    break;

                case KEY_END:
                    DumpMoveCursor(gd->cursor | (BYTES_PER_ROW - 1));
                    break;

                case KEY_MOD_CTRL | KEY_END:
                    DumpMoveCursor(gd->cursor | (gd->bytesPerScreen - 1));
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
                            char len = strlen((char*)gd->buffer[i]);
                            int valid = !NumberFromString((char*)gd->buffer[i] + 1, &stack[i]);
                            DialogAddItem((char*)gd->buffer[i], len, valid ? ' ' : '\xd7', !valid, keys[i], -1);
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
                        SetCursor(row, col + 6, CURSOR_MODE_HIDE);
                        NumberPrompt("", (char*)gd->buffer[choice-1] + 1, BUFFER_INPUT, choice > 1 ? "0x" : "!");
                    }
                    DumpRedrawScreen();
                    break;

                case KEY_MOD_CTRL | KEY_G:
                    buffer[0] = 0;
                    for(;;) {
                        ClearRowCols(4, 1, 41);
                        SetCursor(4, 1, CURSOR_MODE_HIDE);
                        if(NumberPrompt("Go to address: ", buffer, sizeof(buffer), "0x") == -2) {
                            break;
                        }
                        if(NumberFromString(buffer, &addr) == 0) {
                            DumpSetAddress(addr);
                            break;
                        }
                    }
                    DumpRedrawScreen();
                    break;

                case KEY_MOD_CTRL | KEY_MOD_LEFTSHIFT | KEY_G:
                case KEY_MOD_CTRL | KEY_MOD_RIGHTSHIFT | KEY_G:
                    if(gd->cursor & 1) {
                        break;
                    }
                    InstallBusErrorHandler();
                    gd->busError = 0;
                    addr = *(uint32_t*)(gd->pAddress + gd->cursor);
                    UninstallBusErrorHandler();
                    if(!gd->busError) {
                        DumpSetAddress(addr);
                    }
                    DumpRedrawScreen();
                    break;

                case KEY_BACKSPACE:
                    DumpSetAddress((uint32_t)gd->pPrevAddress);
                    DumpRedrawScreen();
                    break;

                case KEY_MOD_CTRL | KEY_R:
                    DumpRedrawScreen();
                    break;

                case KEY_CLEAR_FILE:
                    memset((char*)gd->scratch, 0, sizeof(gd->scratch));
                    DumpRedrawScreen();
                    break;
            }
            break;
    }
}
