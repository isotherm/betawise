#include <alloca.h>
#include <stdint.h>

// Makefile supplies the selected upstream ROM level to every translation unit.
#ifndef MICROPY_CONFIG_ROM_LEVEL
#define MICROPY_CONFIG_ROM_LEVEL MICROPY_CONFIG_ROM_LEVEL_CORE_FEATURES
#endif
// Port memory budgets in KiB (1024 bytes).
#define OS3K_MP_STACK_KB (4)
#if MICROPY_CONFIG_ROM_LEVEL >= MICROPY_CONFIG_ROM_LEVEL_EXTRA_FEATURES
// The default deflate window alone needs 32 KiB, plus VM/object overhead.
#define OS3K_MP_HEAP_KB (64)
#else
#define OS3K_MP_HEAP_KB (16)
#endif

#define MICROPY_OBJ_REPR MICROPY_OBJ_REPR_B
#define MICROPY_FLOAT_IMPL MICROPY_FLOAT_IMPL_FLOAT
#define MICROPY_PY_BUILTINS_COMPLEX (0)
#define MICROPY_ENABLE_COMPILER (1)
#define MICROPY_HELPER_REPL (1)
#define MICROPY_REPL_EVENT_DRIVEN (1)
#define MICROPY_REPL_EMACS_KEYS (1)
#define MICROPY_REPL_AUTO_INDENT (0)
#define MICROPY_HAL_HAS_VT100 (1)
#define MP_STATE_PORT MP_STATE_VM
#define MICROPY_ENABLE_GC (1)
#define MICROPY_STACK_CHECK (1)
#define MICROPY_KBD_EXCEPTION (1)
#define MICROPY_PY_MACHINE (1)
#define MICROPY_PY_MACHINE_MEMX (1)
#define MICROPY_PY_SYS_PLATFORM "os3k"
#define MP_NEED_LOG2 (1)
#define MICROPY_PY_TIME (0)

// Single-threaded applet: no async polling, threads or network backends.
#define MICROPY_PY_THREAD (0)
#define MICROPY_PY_THREAD_GIL (0)
#define MICROPY_PY_ASYNCIO (0)
#define MICROPY_PY_SELECT (0)
#define MICROPY_PY_SELECT_SELECT (0)
#define MICROPY_PY_SOCKET (0)
#define MICROPY_PY_LWIP (0)
#define MICROPY_PY_SSL (0)

// uctypes packs descriptors into >=31 small-int bits; REPR_B has only 30.
#define MICROPY_PY_UCTYPES (0)

typedef long mp_off_t;

// VM hook to allow breaking in the middle of running code.
void os3k_vm_poll(void);
#define MICROPY_VM_HOOK_INIT unsigned os3k_poll_budget = 256;
#define MICROPY_VM_HOOK_LOOP do { if (--os3k_poll_budget == 0) { os3k_poll_budget = 256; os3k_vm_poll(); } } while (0);
