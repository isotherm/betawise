#include <setjmp.h>

_Static_assert(sizeof(jmp_buf) == 52, "68000 jump buffer layout");
extern int check_registers(void);

__attribute__((noinline, noreturn))
static void unwind(jmp_buf env, int value, unsigned depth) {
    volatile unsigned frame[8];
    frame[0] = depth;
    if (frame[0]) {
        unwind(env, value, depth - 1);
    }
    longjmp(env, value);
}

int test(void) {
    jmp_buf outer, inner;
    volatile unsigned visits = 0;
    switch (setjmp(outer)) {
        case 0:
            visits++;
            unwind(outer, 0, 4);
        case 1:
            if (visits != 1) return 1;
            break;
        default:
            return 2;
    }
    switch (setjmp(outer)) {
        case 0:
            switch (setjmp(inner)) {
                case 0: unwind(inner, 37, 3);
                case 37: break;
                default: return 3;
            }
            visits++;
            unwind(outer, -9, 5);
        case -9:
            if (visits != 2) return 4;
            break;
        default:
            return 5;
    }
    return check_registers() ? 6 : 0;
}
