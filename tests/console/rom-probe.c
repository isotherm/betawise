#include "os3k_display_internal.h"

/* Runs in disposable RAM after original-ROM boot; no patched syscalls. */
void console_probe(void) {
    volatile unsigned *result = (void *)0x35000;
    uint32_t status = 0;
    uint8_t row, col, rows, cols;
    *result = 1;
    _DisplayProcessMessage(MSG_INIT, &status);
    _DisplayProcessMessage(MSG_SETFOCUS, &status);
    _ConsoleReset();
    _DisplayGetSize(&rows, &cols);
    if(rows < 2 || cols < 10) { *result = 2; return; }
    fputs("\033[2J\033[2;", stdout);
    fputs("5HAB", stdout);
    GetCursorPos(&row, &col);
    if(row != 2 || col != 7) { *result = 3; return; }
    fputs("\033[6n", stdout);
    const char *expected = "\033[2;7R";
    while(*expected) if(getc(stdin) != *expected++) { *result = 4; return; }
    if(ConsoleReadPending() != -1) { *result = 5; return; }
    fputs("\033[1D\033[K", stdout);
    GetCursorPos(&row, &col);
    if(row != 2 || col != 6) { *result = 6; return; }
    fputs("\033[1J", stdout);
    GetCursorPos(&row, &col);
    if(row != 2 || col != 6) { *result = 7; return; }
    fputs("\033[2J\033[HTERMINAL PASS", stdout);
    GetCursorPos(&row, &col);
    if(row != 1 || col != 14) { *result = 8; return; }
    /* Scroll and erase in the rolled coordinate system too. */
    for(unsigned n=0; n<rows+1; ++n) fputs("\nscroll", stdout);
    fputs("\033[2J\033[HTERMINAL PASS", stdout);
    *result = 0x70617373;
}
