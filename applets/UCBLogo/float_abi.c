#include "softfloat.h"

typedef union {
    double native;
    float64_t encoded;
} DoubleBits;

static float64_t encode(double value)
{
    return (DoubleBits){.native = value}.encoded;
}

static double decode(float64_t value)
{
    return (DoubleBits){.encoded = value}.native;
}

double __adddf3(double a, double b) { return decode(f64_add(encode(a), encode(b))); }
double __subdf3(double a, double b) { return decode(f64_sub(encode(a), encode(b))); }
double __muldf3(double a, double b) { return decode(f64_mul(encode(a), encode(b))); }
double __divdf3(double a, double b) { return decode(f64_div(encode(a), encode(b))); }
double __floatsidf(int a) { return decode(i32_to_f64(a)); }
double __floatunsidf(unsigned a) { return decode(ui32_to_f64(a)); }
int __fixdfsi(double a) { return f64_to_i32_r_minMag(encode(a), false); }
int __eqdf2(double a, double b) { return !f64_eq(encode(a), encode(b)); }
int __nedf2(double a, double b) { return __eqdf2(a, b); }
int __unorddf2(double a, double b)
{
    return !f64_eq(encode(a), encode(a)) || !f64_eq(encode(b), encode(b));
}

static int compare(double a, double b, int unordered)
{
    if (__unorddf2(a, b)) return unordered;
    if (f64_eq(encode(a), encode(b))) return 0;
    return f64_lt(encode(a), encode(b)) ? -1 : 1;
}

int __ledf2(double a, double b) { return compare(a, b, 1); }
int __ltdf2(double a, double b) { return compare(a, b, 1); }
int __gedf2(double a, double b) { return compare(a, b, -1); }
int __gtdf2(double a, double b) { return compare(a, b, -1); }

/* The Linux libgcc multiply uses a 68020-only long multiply instruction. */
uint64_t __muldi3(uint64_t a, uint64_t b)
{
    uint32_t low_a = a, low_b = b;
    uint32_t a0 = (uint16_t)low_a, a1 = low_a >> 16;
    uint32_t b0 = (uint16_t)low_b, b1 = low_b >> 16;
    uint32_t p0 = a0 * b0, p1 = a1 * b0, p2 = a0 * b1;
    uint32_t middle = (p0 >> 16) + (uint16_t)p1 + (uint16_t)p2;
    uint32_t low = (middle << 16) | (uint16_t)p0;
    uint32_t high = a1 * b1 + (p1 >> 16) + (p2 >> 16) + (middle >> 16);
    high += (uint32_t)(a >> 32) * low_b + low_a * (uint32_t)(b >> 32);
    return ((uint64_t)high << 32) | low;
}
