/* Directed original-ROM probe; no display or global syscall hooks. */
#include <stdarg.h>
typedef int (*emit_fn)(int, void *);
typedef int (*engine_fn)(emit_fn, void *, const char *, va_list);
struct result { unsigned count, calls, a5, bad; int status; char text[256]; };
volatile unsigned callback_count;
static int emit(int c, void *opaque) {
    struct result *r = opaque;
    unsigned a5;
    __asm__ volatile("move.l %%a5,%0" : "=d"(a5));
    r->a5 = a5;
    r->calls++;
    callback_count++;
    if (r->count >= 255) { r->bad = 1; return -1; }
    r->text[r->count++] = (char)c;
    r->text[r->count] = 0;
    return c;
}
static int format(engine_fn engine, struct result *r, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = engine(emit, r, fmt, args);
    va_end(args);
    return n;
}
void formatter_probe(engine_fn engine, const char *fmt, const char *s, unsigned mode) {
    struct result *r = (void *)0x39000;
    callback_count = 0;
    r->count = r->calls = r->a5 = r->bad = 0;
    r->text[0] = 0;
    if (mode == 0)
        r->status = format(engine, r, fmt, -123, 0x12345678L, 0xabcdefU, s, '!', 1.5);
    else
        r->status = format(engine, r, fmt, 8, 3, s, -2.25, 0x55);
}
