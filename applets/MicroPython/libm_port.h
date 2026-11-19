/* The Linux m68k math header declares evaluation types for its FPU ABI.
 * MicroPython's libm declares its own types for the soft-float build. */
#if __FLT_EVAL_METHOD__ != 0
#error MicroPython libm requires evaluation in the declared floating-point type
#endif

#define float_t os3k_linux_float_t
#define double_t os3k_linux_double_t
#include <math.h>
#undef double_t
#undef float_t
