// Blocking input cannot consume the applet's event-driven REPL messages.
#include "py/misc.h"
#include "shared/runtime/interrupt_char.h"

#define mp_hal_readline os3k_readline
int os3k_readline(vstr_t *line, const char *prompt);

void mp_hal_move_cursor_back(unsigned int count);
void mp_hal_erase_line_from_cursor(unsigned int count);
