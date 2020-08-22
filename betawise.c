#define BETAWISE_LIB
#include "betawise.h"

#define DEFINE_SYSCALL(index, name) \
    asm(".global " #name); \
    asm(#name ": .word 0xA000 + 4*" #index)

DEFINE_SYSCALL(0, ClearScreen);
DEFINE_SYSCALL(1, SetCursor);
DEFINE_SYSCALL(2, GetCursorPos);
DEFINE_SYSCALL(3, PutStringCentered);
DEFINE_SYSCALL(4, PutChar);
DEFINE_SYSCALL(5, PutStringRaw);
DEFINE_SYSCALL(6, SetCursorMode);
DEFINE_SYSCALL(7, GetCursorMode);
DEFINE_SYSCALL(8, ClearRowCols);
DEFINE_SYSCALL(9, ClearRows);
DEFINE_SYSCALL(10, SYS_A028); // save screen
DEFINE_SYSCALL(11, SYS_A02C); // restore screen
DEFINE_SYSCALL(12, PutString);
DEFINE_SYSCALL(13, SYS_A034); // clear screen FF
DEFINE_SYSCALL(14, SYS_A038); // move block
DEFINE_SYSCALL(15, SYS_A03C); // putch to shadow
DEFINE_SYSCALL(16, SYS_A040);
DEFINE_SYSCALL(17, SYS_A044);
DEFINE_SYSCALL(18, DrawBitmap);
DEFINE_SYSCALL(19, RasterOp);
DEFINE_SYSCALL(20, SYS_A050);
DEFINE_SYSCALL(21, SYS_A054);
DEFINE_SYSCALL(22, SYS_A058);
DEFINE_SYSCALL(23, SYS_A05C);
DEFINE_SYSCALL(24, SYS_A060);
DEFINE_SYSCALL(25, SYS_A064);
DEFINE_SYSCALL(26, SYS_A068);
DEFINE_SYSCALL(27, SYS_A06C);
DEFINE_SYSCALL(28, SYS_A070);
DEFINE_SYSCALL(29, SYS_A074);
DEFINE_SYSCALL(30, SYS_A078);
DEFINE_SYSCALL(31, SYS_A07C);
DEFINE_SYSCALL(32, ProgressBar);
DEFINE_SYSCALL(33, TextBox);
DEFINE_SYSCALL(34, WaitForKey);
DEFINE_SYSCALL(35, StringCompare);
DEFINE_SYSCALL(36, DisplayMessage);
DEFINE_SYSCALL(37, GetKey);
DEFINE_SYSCALL(38, DrainKeyBuffer);
DEFINE_SYSCALL(39, IsKeyReady);
DEFINE_SYSCALL(40, GetModifierKeys);
DEFINE_SYSCALL(41, ScanKeyboard);
DEFINE_SYSCALL(42, QueueKey);
DEFINE_SYSCALL(43, SetModifierKeys);
DEFINE_SYSCALL(44, SYS_A0B0);
DEFINE_SYSCALL(45, SYS_A0B4);
DEFINE_SYSCALL(46, SYS_A0B8);
DEFINE_SYSCALL(47, SYS_A0BC);
DEFINE_SYSCALL(48, SYS_A0C0);
DEFINE_SYSCALL(49, SYS_A0C4);
DEFINE_SYSCALL(50, SYS_A0C8);
DEFINE_SYSCALL(51, SYS_A0CC);
DEFINE_SYSCALL(52, SYS_A0D0);
DEFINE_SYSCALL(53, SYS_A0D4);
DEFINE_SYSCALL(54, SYS_A0D8);
DEFINE_SYSCALL(55, SYS_A0DC);
DEFINE_SYSCALL(56, SYS_A0E0); // get uptime in centiseconds
DEFINE_SYSCALL(57, SYS_A0E4); // get uptime raw
DEFINE_SYSCALL(58, SYS_A0E8); // set timer
DEFINE_SYSCALL(59, SYS_A0EC); // clear timer
DEFINE_SYSCALL(60, DialogInit);
DEFINE_SYSCALL(61, DialogAddItem);
DEFINE_SYSCALL(62, DialogAddExitKey);
DEFINE_SYSCALL(63, DialogSetChoice);
DEFINE_SYSCALL(64, DialogDraw);
DEFINE_SYSCALL(65, DialogRun);
DEFINE_SYSCALL(66, DialogGetChoice);
DEFINE_SYSCALL(67, DialogGetChoiceId);
DEFINE_SYSCALL(68, DialogGetItemId);
DEFINE_SYSCALL(69, SYS_A114);
DEFINE_SYSCALL(70, SYS_A118);
DEFINE_SYSCALL(71, SYS_A11C);
DEFINE_SYSCALL(72, SYS_A120);
DEFINE_SYSCALL(73, SYS_A124);
DEFINE_SYSCALL(74, SYS_A128);
DEFINE_SYSCALL(75, SYS_A12C);
DEFINE_SYSCALL(76, SYS_A130);
DEFINE_SYSCALL(77, SYS_A134);
DEFINE_SYSCALL(78, SYS_A138);
DEFINE_SYSCALL(79, SYS_A13C);
DEFINE_SYSCALL(80, SYS_A140); // configure serial
DEFINE_SYSCALL(81, SYS_A144);
DEFINE_SYSCALL(82, SYS_A148); // open serial
DEFINE_SYSCALL(83, SYS_A14C);
DEFINE_SYSCALL(84, SYS_A150);
DEFINE_SYSCALL(85, SYS_A154);
DEFINE_SYSCALL(86, SYS_A158);
DEFINE_SYSCALL(87, SYS_A15C); // serial send byte
DEFINE_SYSCALL(88, SYS_A160);
DEFINE_SYSCALL(89, TranslateKeyToChar);
DEFINE_SYSCALL(90, SYS_A168);
DEFINE_SYSCALL(91, SYS_A16C);
DEFINE_SYSCALL(92, SYS_A170); // keyboard layout
DEFINE_SYSCALL(93, SYS_A174);
DEFINE_SYSCALL(94, SYS_A178);
DEFINE_SYSCALL(95, SYS_A17C);
DEFINE_SYSCALL(96, SYS_A180);
DEFINE_SYSCALL(97, SYS_A184);
DEFINE_SYSCALL(98, SYS_A188);
DEFINE_SYSCALL(99, SYS_A18C);
DEFINE_SYSCALL(100, SYS_A190);
DEFINE_SYSCALL(101, SYS_A194);
DEFINE_SYSCALL(102, SYS_A198);
DEFINE_SYSCALL(103, SYS_A19C);
DEFINE_SYSCALL(104, SYS_A1A0);
DEFINE_SYSCALL(105, SYS_A1A4);
DEFINE_SYSCALL(106, SYS_A1A8);
DEFINE_SYSCALL(107, SYS_A1AC);
DEFINE_SYSCALL(108, SYS_A1B0);
DEFINE_SYSCALL(109, SYS_A1B4);
DEFINE_SYSCALL(110, SYS_A1B8);
DEFINE_SYSCALL(111, SYS_A1BC);
DEFINE_SYSCALL(112, SYS_A1C0); // new applet
DEFINE_SYSCALL(113, SYS_A1C4);
DEFINE_SYSCALL(114, SYS_A1C8);
DEFINE_SYSCALL(115, SYS_A1CC);
DEFINE_SYSCALL(116, SYS_A1D0);
DEFINE_SYSCALL(117, SYS_A1D4);
DEFINE_SYSCALL(118, SYS_A1D8);
DEFINE_SYSCALL(119, SYS_A1DC);
DEFINE_SYSCALL(120, SYS_A1E0);
DEFINE_SYSCALL(121, SYS_A1E4);
DEFINE_SYSCALL(122, SYS_A1E8);
DEFINE_SYSCALL(123, SYS_A1EC);
DEFINE_SYSCALL(124, SYS_A1F0);
DEFINE_SYSCALL(125, SYS_A1F4);
DEFINE_SYSCALL(126, SYS_A1F8);
DEFINE_SYSCALL(127, SYS_A1FC);
DEFINE_SYSCALL(128, SYS_A200);
DEFINE_SYSCALL(129, SYS_A204);
DEFINE_SYSCALL(130, SYS_A208);
DEFINE_SYSCALL(131, SYS_A20C);
DEFINE_SYSCALL(132, SYS_A210);
DEFINE_SYSCALL(133, SYS_A214);
DEFINE_SYSCALL(134, SYS_A218);
DEFINE_SYSCALL(135, SYS_A21C);
DEFINE_SYSCALL(136, SYS_A220);
DEFINE_SYSCALL(137, SYS_A224);
DEFINE_SYSCALL(138, SYS_A228);
DEFINE_SYSCALL(139, SYS_A22C);
DEFINE_SYSCALL(140, SYS_A230);
DEFINE_SYSCALL(141, SYS_A234);
DEFINE_SYSCALL(142, AppletFindByName);
DEFINE_SYSCALL(143, AppletFindById);
DEFINE_SYSCALL(144, AppletGetName);
DEFINE_SYSCALL(145, AppletSendMessage);
DEFINE_SYSCALL(146, SYS_A248);
DEFINE_SYSCALL(147, SYS_A24C);
DEFINE_SYSCALL(148, SYS_A250);
DEFINE_SYSCALL(149, SYS_A254);
DEFINE_SYSCALL(150, SYS_A258);
DEFINE_SYSCALL(151, SYS_A25C);
DEFINE_SYSCALL(152, SYS_A260);
DEFINE_SYSCALL(153, SYS_A264);
DEFINE_SYSCALL(154, SYS_A268);
DEFINE_SYSCALL(155, SYS_A26C);
DEFINE_SYSCALL(156, SYS_A270);
DEFINE_SYSCALL(157, SYS_A274);
DEFINE_SYSCALL(158, SYS_A278);
DEFINE_SYSCALL(159, SYS_A27C);
DEFINE_SYSCALL(160, SYS_A280);
DEFINE_SYSCALL(161, SYS_A284);
DEFINE_SYSCALL(162, SYS_A288);
DEFINE_SYSCALL(163, SYS_A28C);
DEFINE_SYSCALL(164, SYS_A290);
DEFINE_SYSCALL(165, SYS_A294);
DEFINE_SYSCALL(166, SYS_A298);
DEFINE_SYSCALL(167, SYS_A29C);
DEFINE_SYSCALL(168, SYS_A2A0);
DEFINE_SYSCALL(169, SYS_A2A4);
DEFINE_SYSCALL(170, SYS_A2A8);
DEFINE_SYSCALL(171, SYS_A2AC);
DEFINE_SYSCALL(172, SYS_A2B0);
DEFINE_SYSCALL(173, SYS_A2B4);
DEFINE_SYSCALL(174, GetSystemInfo);
DEFINE_SYSCALL(175, SYS_A2BC);
DEFINE_SYSCALL(176, SYS_A2C0);
DEFINE_SYSCALL(177, SYS_A2C4);
DEFINE_SYSCALL(178, SYS_A2C8);
DEFINE_SYSCALL(179, SYS_A2CC);
DEFINE_SYSCALL(180, SYS_A2D0);
DEFINE_SYSCALL(181, SYS_A2D4);
DEFINE_SYSCALL(182, SYS_A2D8);
DEFINE_SYSCALL(183, SYS_A2DC);
DEFINE_SYSCALL(184, SYS_A2E0);
DEFINE_SYSCALL(185, SYS_A2E4);
DEFINE_SYSCALL(186, SYS_A2E8);
DEFINE_SYSCALL(187, SYS_A2EC);
DEFINE_SYSCALL(188, SYS_A2F0);
DEFINE_SYSCALL(189, SYS_A2F4);
DEFINE_SYSCALL(190, SYS_A2F8);
DEFINE_SYSCALL(191, SYS_A2FC);
DEFINE_SYSCALL(192, SYS_A300);
DEFINE_SYSCALL(193, SYS_A304);
DEFINE_SYSCALL(194, SYS_A308);
DEFINE_SYSCALL(195, SYS_A30C);
DEFINE_SYSCALL(196, SYS_A310);
DEFINE_SYSCALL(197, SYS_A314);
DEFINE_SYSCALL(198, SYS_A318);
DEFINE_SYSCALL(199, SYS_A31C);
DEFINE_SYSCALL(200, SYS_A320);
DEFINE_SYSCALL(201, SYS_A324);
DEFINE_SYSCALL(202, SYS_A328);
DEFINE_SYSCALL(203, SYS_A32C);
DEFINE_SYSCALL(204, getchar);
DEFINE_SYSCALL(205, SYS_A334); // getchar with echo
DEFINE_SYSCALL(206, abort);
DEFINE_SYSCALL(207, atoi);
DEFINE_SYSCALL(208, atol);
DEFINE_SYSCALL(209, sscanf);
DEFINE_SYSCALL(210, fgets);
DEFINE_SYSCALL(211, fprintf);
DEFINE_SYSCALL(212, fputc);
DEFINE_SYSCALL(213, fscanf);
DEFINE_SYSCALL(214, memchr);
DEFINE_SYSCALL(215, memcmp);
DEFINE_SYSCALL(216, memcpy);
DEFINE_SYSCALL(217, memmove);
DEFINE_SYSCALL(218, memset);
DEFINE_SYSCALL(219, printf);
DEFINE_SYSCALL(220, rand);
DEFINE_SYSCALL(221, scanf);
DEFINE_SYSCALL(222, sprintf);
DEFINE_SYSCALL(223, srand);
DEFINE_SYSCALL(224, strcat);
DEFINE_SYSCALL(225, strchr);
DEFINE_SYSCALL(226, strcmp);
DEFINE_SYSCALL(227, strcpy);
DEFINE_SYSCALL(228, strlen);
DEFINE_SYSCALL(229, strncat);
DEFINE_SYSCALL(230, strncmp);
DEFINE_SYSCALL(231, strncpy);
DEFINE_SYSCALL(232, strrchr);
DEFINE_SYSCALL(233, strstr);
DEFINE_SYSCALL(234, tolower);
DEFINE_SYSCALL(235, toupper);
DEFINE_SYSCALL(236, ungetc);
DEFINE_SYSCALL(237, SYS_A3B4);
DEFINE_SYSCALL(238, SYS_A3B8);
DEFINE_SYSCALL(239, SYS_A3BC);
DEFINE_SYSCALL(240, SYS_A3C0);
DEFINE_SYSCALL(241, SYS_A3C4);
DEFINE_SYSCALL(242, SYS_A3C8);
DEFINE_SYSCALL(243, SYS_A3CC);
DEFINE_SYSCALL(244, SYS_A3D0);
DEFINE_SYSCALL(245, SYS_A3D4);
DEFINE_SYSCALL(246, SYS_A3D8);
DEFINE_SYSCALL(247, SYS_A3DC);
DEFINE_SYSCALL(248, SYS_A3E0);
DEFINE_SYSCALL(249, SYS_A3E4);
DEFINE_SYSCALL(250, SYS_A3E8);
DEFINE_SYSCALL(251, SYS_A3EC);
DEFINE_SYSCALL(252, SYS_A3F0);
DEFINE_SYSCALL(253, SYS_A3F4);
DEFINE_SYSCALL(254, SYS_A3F8);
DEFINE_SYSCALL(255, SYS_A3FC);
DEFINE_SYSCALL(256, SYS_A400);
DEFINE_SYSCALL(257, SYS_A404);
DEFINE_SYSCALL(258, SYS_A408);
DEFINE_SYSCALL(259, SYS_A40C);
DEFINE_SYSCALL(260, SYS_A410);
DEFINE_SYSCALL(261, SYS_A414);
DEFINE_SYSCALL(262, SYS_A418);
DEFINE_SYSCALL(263, SYS_A41C);
DEFINE_SYSCALL(264, SYS_A420);
DEFINE_SYSCALL(265, SYS_A424);
DEFINE_SYSCALL(266, SYS_A428);
DEFINE_SYSCALL(267, SYS_A42C);
DEFINE_SYSCALL(268, SYS_A430);
DEFINE_SYSCALL(269, SYS_A434);
DEFINE_SYSCALL(270, SYS_A438);
DEFINE_SYSCALL(271, SYS_A43C);
DEFINE_SYSCALL(272, SYS_A440);
DEFINE_SYSCALL(273, SYS_A444);
DEFINE_SYSCALL(274, SYS_A448);
DEFINE_SYSCALL(275, SYS_A44C);
DEFINE_SYSCALL(276, SYS_A450);
DEFINE_SYSCALL(277, SYS_A454);
DEFINE_SYSCALL(278, SYS_A458);
DEFINE_SYSCALL(279, SYS_A45C);
DEFINE_SYSCALL(280, SYS_A460);
DEFINE_SYSCALL(281, SYS_A464);
DEFINE_SYSCALL(282, SYS_A468);
DEFINE_SYSCALL(283, SYS_A46C);
DEFINE_SYSCALL(284, SYS_A470);

int fputs(const char* str, FILE* stream) {
    while(*str) {
        if(fputc(*str++, stream) == EOF) {
            return EOF;
        }
    }
    return fputc('\n', stream);
}

int putchar(int character) {
    return fputc(character, stdout);
}

int puts(const char* str) {
    return fputs(str, stdout);
}

void BwProcessMessage(Message_e message, uint32_t param, uint32_t* status) {
    uint8_t appletIndex;
    uint32_t statusTemp;

    switch(message) {
        case MSG_SETFOCUS:
            // Initialize screen and font.
            appletIndex = AppletFindById(0xA1F0);
            if(appletIndex > 0) {
                AppletSendMessage(appletIndex, 0x1000002, (uint32_t)&gd->font, &statusTemp);
                gd->row_count = LCD_HEIGHT / 8;
            } else {
                GetSystemInfo(0, SYS_INFO_SYSTEM_FONT_PTR, &gd->font);
                gd->row_count = LCD_HEIGHT / 16;
            }
            gd->col_count = LCD_WIDTH / 6;
            break;

        case MSG_KILLFOCUS:
            // Restore vertical viewport offset to 0.
            LCD_CMD_REG_LEFT = LCD_CMD_REG_RIGHT = 0x40;
            break;
    };

    // Let the applet process the message.
    ProcessMessage(message, param, status);
}

void BwClearScreen() {
    ClearScreen();
    gd->roll = 0;
    gd->row = 0;
    BwSetCursorPos(1, 1);
}

void BwInvertCursor() {
    if(gd->row == 0) {
        return;
    }
    uint16_t x = gd->x;
    uint16_t w = gd->font->max_width;
    if(x > 0) {
        x--;
        w++;
    }
    RasterOp(x, gd->y % LCD_HEIGHT, w, gd->font->height + 1, NULL, ROP_DSTINVERT);
}

void BwSetCursorPos(uint8_t row, uint8_t col) {
    bool clear_row = false;
    
    BwInvertCursor();
    if(row < 1 || row > gd->row_count) {
        if(row < 1) {
            row = 1;
            gd->roll -= gd->font->height;
        } else {
            row = gd->row_count;
            gd->roll += gd->font->height;
        }
        LCD_CMD_REG_LEFT = LCD_CMD_REG_RIGHT = 0x40 | (gd->roll & 0x3F);
        clear_row = true;
    }    
    gd->row = row;
    gd->col = col;
    gd->x = (col - 1) * gd->font->max_width;
    gd->y = ((row - 1) * gd->font->height) + gd->roll;
    if(clear_row) {
        RasterOp(gd->x, gd->y % LCD_HEIGHT, LCD_WIDTH, gd->font->height, NULL, ROP_WHITENESS);
    }
    BwInvertCursor();
}

void BwGetScreenSize(uint8_t* rows, uint8_t* cols) {
    if(rows != NULL) {
        *rows = gd->row_count;
    }
    if(cols != NULL) {
        *cols = gd->col_count;
    }
}

void BwPutCharRaw(char c) {
    const uint8_t* bitmap = gd->font->bitmap_data + (gd->font->max_bytes * (uint8_t)c);
    BwInvertCursor();
    DrawBitmap(gd->x, gd->y % LCD_HEIGHT, gd->font->max_width, gd->font->height, bitmap);
    gd->x += gd->font->max_width;
    gd->col++;
    BwInvertCursor();
}

void BwPutChar(char c) {
    switch(c) {
        case '\a':
            // TODO: Handle bell.
            break;
        case '\b':
            if(gd->col > 1) {
                BwSetCursorPos(gd->row, gd->col - 1);
            } else {
                // TODO: Handle move to previous row?
            }
            break;
        case '\n':
            // Assume carriage return with line feeds.
            BwSetCursorPos(gd->row + 1, 1);
            break;
        case '\r':
            BwSetCursorPos(gd->row, 1);
            break;
        default:
            BwPutCharRaw(c);
            break;
    }
}

void BwPutString(const char* str) {
    char c;
    while(c = *str++) {
        BwPutChar(c);
    }
}

char BwGetChar() {
    KeyMod_e key;
    char c = 0;

    if(!IsKeyReady()) {
        ScanKeyboard();
    }
    key = GetKey(false);
    c = TranslateKeyToChar(key);
    if(!c) {
        key &= ~KEY_MOD_CAPS_LOCK;
        if(key == KEY_APPLETS) {
            // Invokes the applets menu and does not return.
            SYS_A25C(0x8, key);
        } else if(key == (KEY_MOD_CTRL | KEY_C)) {
            c = '\x03';
        } else if(key == KEY_BACKSPACE) {
            c = '\b';
        }
    }
    return c;
}
