#include "os3k.h"
#include "py/builtin.h"
#include "py/cstack.h"
#include "py/compile.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/repl.h"
#include "py/runtime.h"

APPLET_HEADER_BEGIN
    APPLET_ID(0xA1C0)
    APPLET_NAME("MicroPython")
    APPLET_VERSION(0, 1, 0)
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

enum { STACK_BUDGET = 4096 };
static uint32_t heap[4096];
static char source[512];
static size_t length;
static size_t line_start;
static bool started;
static bool at_line_start;

// The io module supplies memory streams, but no OS3K filesystem adapter yet.
static mp_obj_t os3k_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    (void)n_args;
    (void)args;
    (void)kwargs;
    mp_raise_OSError(MP_EOPNOTSUPP);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, os3k_open);

void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    while (len--) {
        unsigned char c = *str++;
        if (c == '\n' || c >= ' ') {
            putchar(c);
            at_line_start = c == '\n';
        }
    }
}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    (void)filename;
    mp_raise_OSError(MP_ENOENT);
}

static void show_input(void) {
    if (!at_line_start) {
        putchar('\n');
    }
    fputs(line_start ? "... " : ">>> ", stdout);
    at_line_start = false;
    mp_hal_stdout_tx_strn_cooked(source + line_start, length - line_start);
}

void gc_collect(void) {
    // Include pointers in registers saved by GCC in this frame.
    __builtin_unwind_init();
    uintptr_t stack;
    __asm__ volatile ("move.l %%sp,%0" : "=r" (stack));
    gc_collect_start();
    gc_collect_root((void **)stack, ((uintptr_t)MP_STATE_THREAD(stack_top) - stack) / sizeof(void *));
    gc_collect_end();
}

void nlr_jump_fail(void *value) {
    (void)value;
    abort();
}

static void execute(void) {
    mp_cstack_init_with_sp_here(STACK_BUDGET);
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lexer = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, source, length, 0);
        qstr name = lexer->source_name;
        mp_parse_tree_t tree = mp_parse(lexer, MP_PARSE_SINGLE_INPUT);
        mp_call_function_0(mp_compile(&tree, name, true));
        nlr_pop();
    } else {
        mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));
    }
}

static void prompt(void) {
    length = line_start = 0;
    show_input();
}

static void input(char c) {
    if (c == '\r' || c == '\n') {
        SetCursorMode(CURSOR_MODE_HIDE);
        putchar('\n');
        at_line_start = true;
        source[length] = '\0';
        if (length != line_start && length + 1 < sizeof(source) &&
            mp_repl_continue_with_input(source)) {
            source[length++] = '\n';
            line_start = length;
            show_input();
            return;
        }
        if (length) {
            execute();
        }
        prompt();
    } else if (c == '\b' || c == 127) {
        if (length > line_start) {
            length--;
            fputs("\b \b", stdout);
        }
    } else if (c == 3) {
        putchar('\n');
        at_line_start = true;
        prompt();
    } else if (c >= ' ' && length + 1 < sizeof(source)) {
        source[length++] = c;
        mp_hal_stdout_tx_strn_cooked(&c, 1);
    }
}

void ProcessMessage(Message_e message, uint32_t param, uint32_t *status) {
    *status = 0;
    switch (message) {
        case MSG_SETFOCUS:
            at_line_start = true;
            if (!started) {
                mp_cstack_init_with_sp_here(STACK_BUDGET);
                gc_init(heap, heap + sizeof(heap) / sizeof(heap[0]));
                mp_init();
                started = true;
                fputs("MicroPython\n", stdout);
            }
            show_input();
            break;
        case MSG_KILLFOCUS:
            SetCursorMode(CURSOR_MODE_HIDE);
            return;
        case MSG_CHAR:
            if (started) {
                input((char)param);
            }
            break;
        case MSG_KEY:
            if (started) {
                switch (param & ~KEY_MOD_CAPS_LOCK) {
                    case KEY_BACKSPACE: input('\b'); break;
                    case KEY_MOD_CTRL | KEY_C: input(3); break;
                }
            }
            break;
        default:
            return;
    }
    if (started) {
        SetCursorMode(CURSOR_MODE_SHOW);
    }
}
