/* SPDX-License-Identifier: MIT */
#include <stdint.h>

/* Force MULU.W so the compiler cannot lower the product to __mulsi3. */
static inline uint32_t multiply_words(uint16_t a, uint16_t b)
{
    uint32_t product = a;
    __asm__("mulu.w %1,%0" : "+d"(product) : "d"(b) : "cc");
    return product;
}

/* Only the low 32 bits of the product are returned. */
uint32_t __mulsi3(uint32_t a, uint32_t b)
{
    uint32_t low = multiply_words(a, b);
    uint32_t cross = multiply_words(a >> 16, b) + multiply_words(b >> 16, a);
    return low + (cross << 16);
}
