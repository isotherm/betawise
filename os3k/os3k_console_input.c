#include "os3k_display_internal.h"

int fgetc(FILE* stream) {
    return stream == stdin ? _ConsoleGetChar() : _OS3K_fgetc(stream);
}

int getc(FILE* stream) { return fgetc(stream); }

char* fgets(char* str, int size, FILE* stream) {
    if(size <= 0) return NULL;
    int n = 0;
    while(n + 1 < size) {
        int c = fgetc(stream);
        if(c < 0) { if(!n) return NULL; break; }
        str[n++] = c;
        if(c == '\n' || c == '\r') break;
    }
    str[n] = 0;
    return str;
}
