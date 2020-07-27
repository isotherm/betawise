#include "betawise.h"

struct gd_t {
    char scratch[256];
    void (*pCommand)();
    int n;
    unsigned args[10];
    unsigned regs[4];
};

APPLET_HEADER_BEGIN
    APPLET_ID(0xA1DE)
    APPLET_NAME("Debugging Tool")
    APPLET_INFO("Copyright (c) 2020 Alpaxo Software")
    APPLET_VERSION(0, 0, 1)
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

void new_line() {
    for(int i = 0; i < 10; i++) {
        gd->args[i] = 0;
    }
    gd->pCommand = 0;
    gd->n = -1;
    printf("$ ");
}

void command_call() {
    if(gd->n < 1) {
        puts("");
        puts("Syntax: C address [arg1 [arg...]]");
        return;
    }
    puts(" OK");
    unsigned pFunc = gd->args[0];
    if(pFunc >= 0xA000 && pFunc <= 0xA400 && !(pFunc & 3)) {
        pFunc = (unsigned)&ClearScreen + ((pFunc - 0xA000) >> 1);
    }
    switch(gd->n) {
        case 0:
            ((void(*)())pFunc)();
            break;
        case 1:
            ((void(*)(unsigned))pFunc)(gd->args[1]);
            break;
        case 2:
            ((void(*)(unsigned,unsigned))pFunc)(gd->args[1], gd->args[2]);
            break;
        case 3:
            ((void(*)(unsigned,unsigned,unsigned))pFunc)(gd->args[1], gd->args[2], gd->args[3]);
            break;
        case 4:
            ((void(*)(unsigned,unsigned,unsigned,unsigned))pFunc)(gd->args[1], gd->args[2], gd->args[3], gd->args[4]);
            break;
        case 5:
            ((void(*)(unsigned,unsigned,unsigned,unsigned,unsigned))pFunc)(gd->args[1], gd->args[2], gd->args[3], gd->args[4], gd->args[5]);
            break;
        case 6:
            ((void(*)(unsigned,unsigned,unsigned,unsigned,unsigned,unsigned))pFunc)(gd->args[1], gd->args[2], gd->args[3], gd->args[4], gd->args[5], gd->args[6]);
            break;
        case 7:
            ((void(*)(unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned))pFunc)(gd->args[1], gd->args[2], gd->args[3], gd->args[4], gd->args[5], gd->args[6], gd->args[7]);
            break;
        case 8:
            ((void(*)(unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned))pFunc)(gd->args[1], gd->args[2], gd->args[3], gd->args[4], gd->args[5], gd->args[6], gd->args[7], gd->args[8]);
            break;
        case 9:
            ((void(*)(unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned))pFunc)(gd->args[1], gd->args[2], gd->args[3], gd->args[4], gd->args[5], gd->args[6], gd->args[7], gd->args[8], gd->args[9]);
            break;
    }
    register int data0 asm("d0");
    register int data1 asm("d1");
    register int *pAddr0 asm("a0");
    register int *pAddr1 asm("a1");
    gd->regs[0] = data0;
    gd->regs[1] = data1;
    gd->regs[2] = (int)pAddr0;
    gd->regs[3] = (int)pAddr1;
}

void command_regs() {
    if(gd->n != 0) {
        puts("");
        puts("Syntax: R");
        return;
    }
    printf(" D0=%08X A0=%08X A5=%08X\n", gd->regs[0], gd->regs[2], gd);
}

void command_byte() {
    if(gd->n < 1 || gd->n > 2) {
        puts("");
        puts("Syntax: B address [write]");
        return;
    }
    char* pMem = (char*)gd->args[0];
    if(gd->n == 2) {
        *pMem = gd->args[1];
        puts(" OK");
    } else {
        char val = *pMem;
        printf(" = %c %02X (%d)\n", val, val, val);
    }
}

void command_word() {
    if(gd->n < 1 || gd->n > 2) {
        puts("");
        puts("Syntax: W address [write]");
        return;
    }
    short* pMem = (short*)gd->args[0];
    if(gd->n == 2) {
        *pMem = gd->args[1];
        puts(" OK");
    } else {
        short val = *pMem;
        printf(" = %04X (%d)\n", val, val);
    }
}

void command_long() {
    if(gd->n < 1 || gd->n > 2) {
        puts("");
        puts("Syntax: L address [write]");
        return;
    }
    long* pMem = (long*)gd->args[0];
    if(gd->n == 2) {
        *pMem = gd->args[1];
        puts(" OK");
    } else {
        long val = *pMem;
        printf(" = %08X (%d)\n", val, val);
    }
}

void command_string() {
    if(gd->n < 1) {
        puts("");
        puts("Syntax: S address [write]");
        return;
    }
    char* pMem = (char*)gd->args[0];
    if(gd->n > 1) {
        ((char*)gd->args)[gd->n] = '\0';
        strcpy(pMem, (char*)&gd->args[1]);
        puts("\" OK");
    } else {
        printf(" = \"%s\"\n", pMem);
    }
}

void ProcessMessage(uint32_t message, uint32_t param, uint32_t* status) {
    char key, row, col;
    *status = 0;
    switch(message) {
        case MSG_INIT:
            break;

        case MSG_SETFOCUS:
            ClearScreen();
            SetCursor(1, 1, 0x0F);
            new_line();
            break;

        case MSG_CHAR:
            key = toupper(param);
            if(!gd->pCommand) {
                switch(key) {
                    case 'C':
                        gd->pCommand = &command_call;
                        break;
                    case 'R':
                        gd->pCommand = &command_regs;
                        break;
                    case 'B':
                        gd->pCommand = &command_byte;
                        break;
                    case 'W':
                        gd->pCommand = &command_word;
                        break;
                    case 'L':
                        gd->pCommand = &command_long;
                        break;
                    case 'S':
                        gd->pCommand = &command_string;
                        break;
                    case '\r':
                        puts("");
                        new_line();
                        break;
                    default:
                        GetCursorPos(&row, &col);
                        SetCursor(1, 34, 0x0F);
                        printf("  [%2X]", param);
                        SetCursor(row, col, 0x0F);
                        return;
                }
                putchar(key);
            } else if(key == '\r') {
                gd->n++;
                gd->pCommand();
                new_line();
            } else if(gd->pCommand == &command_string && gd->n >= 4 && gd->n < 39) {
                ((char*)gd->args)[gd->n++] = param;
                putchar(param);
                break;
            } else if(key == ' ') {
                putchar(key);
                gd->n++;
            } else if(gd->n >= 0 && gd->n < 10) {
                if(key >= '0' && key <= '9') {
                    putchar(key);
                    gd->args[gd->n] <<= 4;
                    gd->args[gd->n] |= key - '0';
                } else if(key >= 'A' && key <= 'F') {
                    putchar(key);
                    gd->args[gd->n] <<= 4;
                    gd->args[gd->n] |= 10 + (key - 'A');
                } else if(key == 'V') {
                    gd->args[gd->n] += (unsigned)gd;
                    printf(">%X", gd->args[gd->n]);
                }
            }
            if(gd->pCommand == &command_string && gd->n == 1) {
                putchar('"');
                gd->n = 4;
            }
            break;
        case MSG_KEY:
            switch(param) {
                case KEY_CLEAR_FILE:
                    ClearScreen();
                    SetCursor(1, 1, 0x0F);
                    new_line();
                    break;

                case KEY_ESC:
                case MOD_CTRL | KEY_C:
                    puts("^C");
                    new_line();
                    break;

                default:
                    GetCursorPos(&row, &col);
                    SetCursor(1, 34, 0x0F);
                    printf("{%4X}", param);
                    SetCursor(row, col, 0x0F);
                    break;
                }
            break;
    }
}
