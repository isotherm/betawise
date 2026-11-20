#include "os3k_display_internal.h"

bool FormatInt32(int32_t value, uint32_t radix, char* buffer) {
    if(radix < 10 || radix > 36) {
        return false;
    }
    _OS3K_FormatInt32(value, radix, buffer);
    return true;
}

int getchar(void) { return _ConsoleGetChar(); }

int fputc(int c, FILE* stream) {
    if(stream != stdout) {
        return _OS3K_fputc(c, stream);
    }
    return _ConsolePutChar(c);
}

int fputs(const char* str, FILE* stream) {
    char c;
    int res;
    while((c = *str++)) {
        res = fputc((uint8_t)c, stream);
        if(res < 0) {
            return res;
        }
    }
    return 0;
}

int putchar(int c) {
    return fputc(c, stdout);
}

int puts(const char* str) {
    return fputs(str, stdout);
}
