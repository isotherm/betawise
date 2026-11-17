#include <stdint.h>

// Use relocatable 68000 code for compiler-generated 32-bit division and remainder.
static uint32_t divide(uint32_t dividend, uint32_t divisor, uint32_t *remainder) {
    if (divisor == 0) {
        __builtin_trap();
    }

    uint32_t quotient = 0;
    uint32_t bit = 1;
    while (divisor < dividend && divisor <= UINT32_MAX / 2) {
        divisor <<= 1;
        bit <<= 1;
    }
    do {
        if (dividend >= divisor) {
            dividend -= divisor;
            quotient |= bit;
        }
        divisor >>= 1;
        bit >>= 1;
    } while (bit);
    *remainder = dividend;
    return quotient;
}

static uint32_t magnitude(int32_t value) {
    return value < 0 ? -(uint32_t)value : (uint32_t)value;
}

uint32_t __udivsi3(uint32_t dividend, uint32_t divisor) {
    uint32_t remainder;
    return divide(dividend, divisor, &remainder);
}

uint32_t __umodsi3(uint32_t dividend, uint32_t divisor) {
    uint32_t remainder;
    divide(dividend, divisor, &remainder);
    return remainder;
}

int32_t __divsi3(int32_t dividend, int32_t divisor) {
    uint32_t remainder;
    uint32_t quotient = divide(magnitude(dividend), magnitude(divisor), &remainder);
    return (dividend < 0) != (divisor < 0) ? -quotient : quotient;
}

int32_t __modsi3(int32_t dividend, int32_t divisor) {
    uint32_t remainder;
    divide(magnitude(dividend), magnitude(divisor), &remainder);
    return dividend < 0 ? -remainder : remainder;
}
