#ifndef OS3K_SETJMP_H
#define OS3K_SETJMP_H
typedef unsigned long jmp_buf[13];
int setjmp(jmp_buf env) __attribute__((returns_twice));
void longjmp(jmp_buf env, int value) __attribute__((noreturn));
#endif
