#include <alloca.h>
#include <stdint.h>

// Makefile supplies the selected upstream ROM level to every translation unit.
#ifndef MICROPY_CONFIG_ROM_LEVEL
#define MICROPY_CONFIG_ROM_LEVEL MICROPY_CONFIG_ROM_LEVEL_CORE_FEATURES
#endif
#define MICROPY_OBJ_REPR MICROPY_OBJ_REPR_B
#define MICROPY_FLOAT_IMPL MICROPY_FLOAT_IMPL_FLOAT
#define MICROPY_PY_BUILTINS_COMPLEX (0)
#define MICROPY_ENABLE_COMPILER (1)
#define MICROPY_HELPER_REPL (1)
#define MICROPY_ENABLE_GC (1)
#define MICROPY_STACK_CHECK (1)
#define MICROPY_PY_SYS_PLATFORM "os3k"

// The basic profile enables time upstream; the OS3K HAL is not implemented yet.
#define MICROPY_PY_TIME (0)

typedef long mp_off_t;
