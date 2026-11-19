#include "os3k.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "py/stream.h"

mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    mp_hal_stdout_tx_strn_cooked(str, len);
    return len;
}

void mp_hal_stdout_tx_str(const char *str) {
    mp_hal_stdout_tx_strn_cooked(str, strlen(str));
}

uintptr_t mp_hal_stdio_poll(uintptr_t flags) {
    // Output can be written synchronously; input is owned by applet messages.
    return flags & MP_STREAM_POLL_WR;
}

int mp_hal_stdin_rx_chr(void) {
    mp_raise_OSError(MP_EOPNOTSUPP);
}

int os3k_readline(vstr_t *line, const char *prompt) {
    (void)line;
    (void)prompt;
    mp_raise_OSError(MP_EOPNOTSUPP);
}

int mp_interrupt_char = 3;

void mp_hal_set_interrupt_char(int c) {
    mp_interrupt_char = c;
}

mp_uint_t mp_hal_ticks_ms(void) {
    // OS3K monotonic clock hook, retained for future time-module integration.
    return GetUptimeMilliseconds();
}
