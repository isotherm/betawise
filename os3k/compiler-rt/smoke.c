/* SPDX-License-Identifier: MIT */
#include <stdint.h>

/* Volatile operands force GCC to emit the runtime calls under test. */
static volatile uint32_t a = 0xfedcba98, b = 0x12345678;
static volatile int32_t c = -101, d = 9;
static volatile uint64_t wide = UINT64_C(0xfedcba9876543210), divisor = 0x123456789ULL;
static volatile unsigned shift = 37;
static volatile float f = 1.5f, g = 0.25f;
static volatile double x = -3.5, y = 0.125;
static volatile uint32_t nan_bits = 0x7fc00000;

int smoke(void)
{
    if (a * b != 0x35068740 || a / b != 14 || a % b != 0x8)
        return 1;
    if (c / d != -11 || c % d != -2)
        return 2;
    if (wide * divisor != UINT64_C(0xcccccccce1833a90) ||
        wide / divisor != UINT64_C(0xe0000000) || wide % divisor != 0x96543210)
        return 3;
    if (wide << shift != UINT64_C(0xca86420000000000) ||
        wide >> shift != 0x7f6e5d4 || (int64_t)wide >> shift != -0x91a2c)
        return 4;
    if (f + g != 1.75f || f - g != 1.25f || f * g != 0.375f || f / g != 6.0f)
        return 5;
    if (x + y != -3.375 || x - y != -3.625 || x * y != -0.4375 || x / y != -28)
        return 6;
    if ((int)x != -3 || (unsigned long long)f != 1 || (float)c != -101.0f ||
        (double)b != 305419896.0 || (double)g != y * 2 || (float)x != -3.5f)
        return 7;
    float nan = ((union { uint32_t bits; float value; }){.bits = nan_bits}).value;
    if (nan == f || nan < f || nan > f || nan <= f || nan >= f || !(nan != f))
        return 8;
    return 0;
}
