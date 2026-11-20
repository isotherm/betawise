#include "os3k_display_internal.h"

/* A small per-applet stream parser, deliberately separate from raw drawing.
 * No ROM hooks, global terminal modes, formatting, or alternate screen. */
static struct {
    uint16_t parameter[2];
    uint8_t state, index;
    bool invalid;
    char pending[32];
    uint8_t head, count;
    char key[8];
    uint8_t key_index, key_count;
} console;

void _ConsoleReset(void) {
    console.state = console.index = console.head = console.count = 0;
    console.key_index = console.key_count = 0;
    console.invalid = false;
    console.parameter[0] = console.parameter[1] = 0;
}

int ConsoleReadPending(void) {
    if(!console.count) return -1;
    int c = (uint8_t)console.pending[console.head];
    console.head = (console.head + 1) % sizeof(console.pending);
    --console.count;
    return c;
}

static void reply_byte(char c) {
    console.pending[(console.head + console.count++) % sizeof(console.pending)] = c;
}

static void reply_number(uint8_t n) {
    if(n >= 100) reply_byte('0' + n / 100);
    if(n >= 10) reply_byte('0' + (n / 10) % 10);
    reply_byte('0' + n % 10);
}

static uint8_t clamp(int n, uint8_t limit) {
    return n < 1 ? 1 : n > limit ? limit : n;
}

static void command(int c) {
    uint8_t row, col, rows, cols;
    CursorMode_e mode;
    _DisplayGetSize(&rows, &cols);
    GetCursorPos(&row, &col);
    row = clamp(row, rows);
    col = clamp(col, cols);
    GetCursorMode(&mode);
    int n = console.parameter[0] ? console.parameter[0] : 1;
    switch(c) {
        case 'A': row = clamp(row - n, rows); break;
        case 'B': row = clamp(row + n, rows); break;
        case 'C': col = clamp(col + n, cols); break;
        case 'D': col = clamp(col - n, cols); break;
        case 'H': case 'f':
            row = clamp(n, rows);
            col = clamp(console.parameter[1] ? console.parameter[1] : 1, cols);
            break;
        case 'J': case 'K':
            if(console.parameter[0] <= 2) {
                uint8_t first = c == 'K' || console.parameter[0] == 0 ? row : 1;
                uint8_t last = c == 'K' || console.parameter[0] == 1 ? row : rows;
                for(uint8_t y = first; y <= last; ++y) {
                    uint8_t left = y == row && console.parameter[0] == 0 ? col : 1;
                    uint8_t right = y == row && console.parameter[0] == 1 ? col : cols;
                    _DisplayEraseCells(y, left, right);
                }
            }
            return; /* Erasure leaves cursor (including pending wrap) alone. */
        case 'n':
            if(console.parameter[0] == 6 && sizeof(console.pending) - console.count >= 12) {
                reply_byte('\033'); reply_byte('[');
                reply_number(row); reply_byte(';'); reply_number(col); reply_byte('R');
            }
            return;
        default: return; /* Includes SGR: inverse/attributes intentionally deferred. */
    }
    SetCursor(row, col, mode);
}

int _ConsolePutChar(int value) {
    uint8_t c = value;
    if(c == 27) { console.state = 1; return c; }
    if(c == 0x18 || c == 0x1a) { console.state = 0; return c; }
    if(console.state == 1) {
        console.state = c == '[' ? 2 : 0;
        console.index = 0;
        console.invalid = false;
        console.parameter[0] = console.parameter[1] = 0;
        return c;
    }
    if(console.state == 2) {
        if(c >= '0' && c <= '9') {
            uint16_t *p = &console.parameter[console.index];
            if(*p > 99) console.invalid = true;
            else *p = *p * 10 + c - '0';
        } else if(c == ';') {
            if(console.index) console.invalid = true;
            else ++console.index;
        } else if(c >= 0x40 && c <= 0x7e) {
            console.state = 0;
            if(!console.invalid && (console.index == 0 || c == 'H' || c == 'f' || c == 'm')) command(c);
        } else console.invalid = true;
        return c;
    }
    if(c == '\t') {
        uint8_t row, col, rows, cols;
        _DisplayGetSize(&rows, &cols);
        GetCursorPos(&row, &col);
        CursorMode_e mode;
        GetCursorMode(&mode);
        SetCursor(row, clamp(((col - 1) / 8 + 1) * 8 + 1, cols), mode);
    } else if(c >= 32 || c == '\r' || c == '\n' || c == '\b' || c == '\a') {
        _DisplayPutChar(c);
    }
    return c;
}

size_t ConsoleKeySequence(KeyMod_e key, char output[8]) {
    const char *sequence = NULL;
    size_t count = 0;
    if(key == (KeyMod_e)KEY_NONE || (key & KEY_MOD_KEY_UP)) return 0;
    key &= ~KEY_MOD_CAPS_LOCK;
    switch((unsigned)key) {
        case KEY_UP: sequence = "\033[A"; break;
        case KEY_DOWN: sequence = "\033[B"; break;
        case KEY_RIGHT: sequence = "\033[C"; break;
        case KEY_LEFT: sequence = "\033[D"; break;
        /* Preserve the extended keys already supported by MicroPython. */
        case KEY_HOME: sequence = "\033[H"; break;
        case KEY_END: sequence = "\033[F"; break;
        case KEY_DELETE: sequence = "\033[3~"; break;
        case KEY_BACKSPACE: output[0] = '\b'; return 1;
        case KEY_TAB: output[0] = '\t'; return 1;
        case KEY_ENTER: output[0] = '\r'; return 1;
        case KEY_ESC: output[0] = 27; return 1;
        case KEY_APPLETS:
            SystemHandleEvents(SYSTEM_EVENT_SPECIAL_KEYS, key);
            return 0;
        default: break;
    }
    if(sequence) {
        while(*sequence) output[count++] = *sequence++;
        return count;
    }
    int c = (uint8_t)_OS3K_TranslateKeyToChar(key & ~KEY_MOD_CTRL);
    if(key & KEY_MOD_CTRL) {
        if(c == ' ') c = 0;
        else if(c == '?') c = 127;
        else if(c >= 'a' && c <= 'z') c -= 'a' - 1;
        else if(c >= '@' && c <= '_') c &= 0x1f;
        else return 0;
    } else if(!c) return 0;
    output[0] = c;
    return 1;
}

int _ConsoleGetChar(void) {
    if(console.key_index < console.key_count) return (uint8_t)console.key[console.key_index++];
    int c = ConsoleReadPending();
    if(c >= 0) return c;
    for(;;) {
        while(!IsKeyReady()) ScanKeyboard();
        console.key_count = ConsoleKeySequence(GetKey(false), console.key);
        if(console.key_count) { console.key_index = 1; return (uint8_t)console.key[0]; }
    }
}
