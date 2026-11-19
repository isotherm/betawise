#ifndef OS3K_SETJMP_H
#define OS3K_SETJMP_H
// 68000 integer-only context: D2-D7, A2-A7, and the return PC.
// No floating-point state or signal mask is saved.
typedef unsigned long jmp_buf[13];
int setjmp(jmp_buf env) __attribute__((returns_twice));
void longjmp(jmp_buf env, int value) __attribute__((noreturn));
#endif
