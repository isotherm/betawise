#include "os3k.h"
#include "py/builtin.h"
#include "py/cstack.h"
#include "py/compile.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/repl.h"
#include "py/runtime.h"
#include "shared/runtime/interrupt_char.h"
#include "shared/readline/readline.h"
#include "shared/runtime/pyexec.h"

APPLET_HEADER_BEGIN
    APPLET_ID(0xA1C0)
    APPLET_NAME("MicroPython")
    APPLET_VERSION(0, 1, 0)
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

static uint32_t heap[OS3K_MP_HEAP_KB * 1024 / sizeof(uint32_t)];
static bool started;
static bool repl_paste_mode;
// Preserve ordinary typeahead while execution owns the keyboard queue.
static uint32_t typeahead[64];
static unsigned typeahead_head, typeahead_count;

// Return a character, including NUL, or -1 for an untranslated special key.
static int keyboard_char(KeyMod_e key) {
    // Translate once without Ctrl: ROM translation may suppress Ctrl+letters,
    // and also maintains accent-composition state. Apply ASCII Ctrl encoding
    // generically rather than giving evaluation-time meanings to C or D.
    int c = (unsigned char)_OS3K_TranslateKeyToChar(key & ~KEY_MOD_CTRL);
    if (key & KEY_MOD_CTRL) {
        if (c == ' ') return 0;
        if (c == '?') return 127;
        if (c >= 'a' && c <= 'z') return c - 'a' + 1;
        if (c >= '@' && c <= '_') return c & 0x1f;
        return -1;
    }
    if (c) return c;
    switch (key & ~KEY_MOD_CAPS_LOCK) {
        case KEY_BACKSPACE: return '\b';
        case KEY_TAB: return '\t';
        case KEY_ENTER: return '\r';
        case KEY_ESC: return 27;
        default: return -1;
    }
}

void os3k_vm_poll(void) {
    // ScanKeyboard may otherwise STOP an idle CPU, even during computation.
    bool sleep = IsIdleSleepEnabled();
    SetIdleSleepEnabled(false);
    ScanKeyboard();
    SetIdleSleepEnabled(sleep);
    KeyMod_e key;
    while ((key = GetKey(false)) != KEY_NONE) {
        if (key & KEY_MOD_KEY_UP) {
            continue;
        }
        int c = keyboard_char(key);
        if (c >= 0 && c == mp_interrupt_char) {
            mp_sched_keyboard_interrupt();
        } else if (typeahead_count < sizeof(typeahead) / sizeof(typeahead[0])) {
            // Store translated characters separately from untranslated special keys.
            typeahead[(typeahead_head + typeahead_count++) % 64] =
                c >= 0 ? (0x10000 | (unsigned char)c) : key;
        }
    }
}

static void reset_vm(void) {
    mp_cstack_init_with_sp_here(OS3K_MP_STACK_KB * 1024);
    if (started) mp_deinit();
    gc_init(heap, heap + sizeof(heap) / sizeof(heap[0]));
    mp_init();
    mp_hal_set_interrupt_char(3);
    readline_init0();
    repl_paste_mode = false;
    pyexec_mode_kind = PYEXEC_MODE_FRIENDLY_REPL;
    pyexec_event_repl_init();
    started = true;
}

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
        if (c == '\n' || c == '\r' || c == '\b' || c >= ' ') {
            putchar(c);
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

// The shared readline editor calls these instead of emitting VT100 escapes.
void mp_hal_move_cursor_back(unsigned int count) {
    // Betawise already handles cursor movement and wrapping for backspace.
    while (count--) putchar('\b');
}

void mp_hal_erase_line_from_cursor(unsigned int count) {
    for (unsigned int i = 0; i < count; ++i) putchar(' ');
    mp_hal_move_cursor_back(count);
}

static void show_input(void) {
    // Bring readline's logical cursor to the end before redrawing after focus.
    readline_process_char(27);
    readline_process_char('[');
    readline_process_char('F');
    ClearScreen();
    vstr_t *line = MP_STATE_VM(repl_line);
    size_t start = line->len;
    while (start && line->buf[start - 1] != '\n') --start;
    fputs(pyexec_mode_kind == PYEXEC_MODE_RAW_REPL ? ">" : (start ? "... " : ">>> "), stdout);
    mp_hal_stdout_tx_strn_cooked(line->buf + start, line->len - start);
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

// Only submission and REPL commands need pyexec's event state machine.
// In-progress editing stays in readline so an empty buffer does not cause
// upstream pyexec to restart the prompt or discard an escape prefix.
static int repl_process_char(int c) {
    if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
        return pyexec_event_repl_process_char(c);
    }
    if (repl_paste_mode) {
        if (c == CHAR_CTRL_C || c == CHAR_CTRL_D) repl_paste_mode = false;
        return pyexec_event_repl_process_char(c);
    }

    vstr_t *line = MP_STATE_VM(repl_line);
    bool empty_line = line->len == 0 || line->buf[line->len - 1] == '\n';
    bool command = c == '\r' || c == '\n' || c == CHAR_CTRL_C
        || (c == CHAR_CTRL_D && empty_line)
        || (line->len == 0 && (c == CHAR_CTRL_A || c == CHAR_CTRL_B || c == CHAR_CTRL_E));
    if (command) {
        if (c == CHAR_CTRL_E) repl_paste_mode = true;
        return pyexec_event_repl_process_char(c);
    }
    readline_process_char(c);
    return 0;
}

static void input(char c) {
    // Editing, completion and history also allocate, so anchor GC on each event.
    mp_cstack_init_with_sp_here(OS3K_MP_STACK_KB * 1024);
    SetCursorMode(CURSOR_MODE_HIDE);
    int result = 0;
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        result = repl_process_char((unsigned char)c);
        #if MICROPY_ENABLE_SCHEDULER
        // Preserve bounded dispatch of the batch queued by this command.
        if (!(result & PYEXEC_FORCED_EXIT)) {
            mp_hal_set_interrupt_char(CHAR_CTRL_C);
            for (size_t pending = mp_sched_num_pending(); pending; --pending) {
                mp_handle_pending(MP_HANDLE_PENDING_CALLBACKS_AND_EXCEPTIONS);
            }
            mp_hal_set_interrupt_char(-1);
        }
        #endif
        nlr_pop();
    } else {
        mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));
        repl_paste_mode = false;
        pyexec_event_repl_process_char(CHAR_CTRL_C);
    }
    if (result & PYEXEC_FORCED_EXIT) {
        fputs("MPY: soft reboot\n", stdout);
        reset_vm();
    }
}

static void input_key(uint32_t key) {
    if (key & KEY_MOD_KEY_UP) return;
    if (key & KEY_MOD_CTRL) {
        int c = keyboard_char(key);
        if (c >= 0) input((char)c);
        return;
    }
    const char *escape = NULL;
    key &= ~KEY_MOD_CAPS_LOCK;
    switch (key) {
        case KEY_BACKSPACE: input('\b'); break;
        case KEY_TAB: input('\t'); break;
        case KEY_UP: escape = "\033[A"; break;
        case KEY_DOWN: escape = "\033[B"; break;
        case KEY_RIGHT: escape = "\033[C"; break;
        case KEY_LEFT: escape = "\033[D"; break;
        case KEY_HOME: escape = "\033[H"; break;
        case KEY_END: escape = "\033[F"; break;
        case KEY_DELETE: escape = "\033[3~"; break;
        case KEY_APPLETS: SystemHandleEvents(SYSTEM_EVENT_SPECIAL_KEYS, key); break;
    }
    if (escape && pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL) {
        while (*escape) input(*escape++);
    }
}

void ProcessMessage(Message_e message, uint32_t param, uint32_t *status) {
    *status = 0;
    switch (message) {
        case MSG_SETFOCUS:
            if (!started) {
                reset_vm();
            } else {
                show_input();
            }
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
                input_key(param);
            }
            break;
        default:
            return;
    }
    while (started && typeahead_count) {
        unsigned key = typeahead[typeahead_head];
        typeahead_head = (typeahead_head + 1) % 64;
        --typeahead_count;
        if (key & 0x10000) input((char)key);
        else input_key(key);
    }
    if (started) {
        SetCursorMode(CURSOR_MODE_SHOW);
    }
}
