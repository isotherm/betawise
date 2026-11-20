#include "os3k.h"

APPLET_HEADER_BEGIN
    APPLET_ID(0xA1F1)
    APPLET_NAME("File Manager")
    APPLET_INFO("Browse and manage applet files")
    APPLET_VERSION(0, 1, "a")
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

static struct {
    uint32_t folder, scanCursor;
    uint16_t ids[255], count, choice;
    uint8_t applets[31], appletCount, appletChoice;
    bool files, active;
} g_state;

static void DrawRow(uint8_t r, const char* text) {
    SetCursor(r, 1, CURSOR_MODE_HIDE);
    PutString(text);
}

static void DrawHex(uint16_t value) {
    char text[5];
    const char digits[] = "0123456789ABCDEF";
    unsigned i;
    for(i = 0; i < 4; ++i) text[i] = digits[(value >> (12 - 4*i)) & 15];
    text[4] = 0;
    PutString(text);
}

static void RedrawScreen(void) {
    /* Keep ROM output and compiler-generated aggregate stores word-aligned.
     * A 37-byte initialized array generated MOVE.L to an odd stack address. */
    char name[38] __attribute__((aligned(2)));
    name[0] = 0;
    ClearScreen();
    if(!g_state.files) {
        DrawRow(1, "File Manager: applets");
        if(g_state.appletCount) {
            AppletGetName(g_state.applets[g_state.appletChoice], name);
            name[32] = 0;
            DrawRow(2, name);
        } else DrawRow(2, "No installed applets");
        DrawRow(4, "Up/Down  Enter:files");
        return;
    }
    AppletGetName(g_state.folder, name);
    name[32] = 0;
    DrawRow(1, name);
    if(g_state.count) {
        uint16_t id = g_state.ids[g_state.choice], tag = 0;
        char number[12];
        name[0] = 0;
        FileAccessName(id, name, 0);
        name[15] = 0;
        DrawRow(2, name);
        DrawRow(3, "ID:"); DrawHex(id);
        PutString(" Tag:");
        if(FileAccessTag(id, 0, &tag) == 0) DrawHex(tag);
        PutString(" ");
        if(FormatInt32(FileQuerySize(id), 10, number)) PutString(number);
        PutString("B");
    } else DrawRow(2, g_state.scanCursor <= 65535 ? "Scanning..." : "No files");
    DrawRow(4, "R:rename M:move D:delete Esc:back");
}

static void RefreshFiles(void) {
    g_state.count = g_state.choice = 0;
    g_state.scanCursor = 1;
}

/* The public API has no enumeration call. Probe exact IDs incrementally,
 * without assuming a byte partition or touching the global clipboard alias. */
static void ScanFiles(void) {
    unsigned n;
    uint16_t oldCount = g_state.count;
    for(n = 0; n < 128 && g_state.scanCursor <= 65535; ++n, ++g_state.scanCursor) {
        uint16_t id = (uint16_t)g_state.scanCursor;
        if(id != FILE_ID_CLIPBOARD && !(id >= 0xFC && id <= 0xFF) &&
           FileResolveId(id & 255, id >> 8) == id) {
            if(g_state.count == 255) { g_state.scanCursor = 65536; break; }
            g_state.ids[g_state.count++] = id;
        }
    }
    if(oldCount != g_state.count || g_state.scanCursor == 65536) RedrawScreen();
}

static bool AuthorizeFile(uint16_t id) {
    uint16_t resolved = 0;
    if(!FileGetPasswordProtection()) return true;
    return FilePromptPassword(id, 0, 1, 0, 0, &resolved) && resolved == id;
}

static bool ConfirmAction(const char* title) {
    KeyMod_e key;
    ClearScreen();
    DrawRow(1, title);
    DrawRow(3, "Enter:confirm  Esc:cancel");
    key = WaitForKey();
    return (key & 255) == KEY_ENTER;
}

static void RenameFile(uint16_t id) {
    char name[16] = {0};
    uint8_t length;
    const Key_e exits[] = { KEY_ENTER, KEY_ESC, KEY_NONE };
    if(!AuthorizeFile(id) || !FileAccessName(id, name, 0)) return;
    name[15] = 0;
    length = strlen(name);
    ClearScreen();
    DrawRow(1, "Rename (15 characters)");
    SetCursor(2, 1, CURSOR_MODE_SHOW);
    if(TextBox(name, &length, 15, exits, false) == KEY_ENTER && length) {
        name[length] = 0;
        if(!FileAccessName(id, name, 1)) DisplayMessage("Rename failed");
    }
    SetCursorMode(CURSOR_MODE_HIDE);
}

static uint32_t SelectDestination(void) {
    /* Keep labels alive: DialogAddItem retains pointers. */
    char names[31][38] __attribute__((aligned(2)));
    unsigned i, count = 0;
    ClearScreen();
    DrawRow(1, "Move to applet:");
    DialogInit(true, 2, 4, 33);
    for(i = 0; i < g_state.appletCount; ++i) {
        uint32_t index = g_state.applets[i];
        if(index != g_state.folder && AppletGetName(index, names[count])) {
            names[count][32] = 0;
            DialogAddItem(names[count], strlen(names[count]), ' ', index,
                          KEY_NONE, (size_t)-1);
            ++count;
        }
    }
    if(!count) return 0;
    DialogAddExitKey(KEY_ENTER);
    DialogAddExitKey(KEY_ESC);
    DialogSetChoice(1);
    DialogDraw();
    if((DialogRun() & 255) != KEY_ENTER) return 0;
    return DialogGetChoiceId();
}

/* A move copies logical bytes, verifies them, then removes the source.
 * No RAM/ROM addresses or clipboard storage are borrowed across allocations. */
static void MoveFile(uint16_t id) {
    uint32_t source = g_state.folder, target, offset = 0;
    int32_t length, position, created = -1;
    uint16_t tag;
    char name[16], password[7];
    uint8_t buffer[128], verify[128];
    const char* error = "Move failed; source retained";
    if(!AuthorizeFile(id)) return;
    /* Flag meanings and linked/editor state are not sufficiently understood
     * to transplant such files into an unrelated applet. */
    if(FileTestFlags(id, 0x0D) != 0) {
        DisplayMessage("Special file: move unsupported");
        return;
    }
    length = FileQuerySize(id);
    position = FileTell(id);
    if(length < 0 || position < 0 || !FileAccessName(id, name, 0) ||
       FileAccessTag(id, 0, &tag) < 0 || FileAccessPassword(id, password, 1) < 0)
        return;
    name[15] = password[6] = 0;
    target = SelectDestination();
    if(!target || !ConfirmAction("Move logical contents?")) return;
    if(FileSetFolder(target, NULL) < 0) goto finish;
    /* A duplicate tag can change which record the receiving applet opens. */
    if(FileFindByTag(tag) >= 0) {
        error = "Tag exists in destination";
        goto finish;
    }
    created = FileCreate(0, name, 0, length);
    if(created < 0) goto finish;
    if(FileAccessTag(created, tag, NULL) < 0 ||
       FileAccessPassword(created, password, 0) < 0) goto rollback;
    while(offset < (uint32_t)length) {
        uint32_t chunk = (uint32_t)length - offset;
        if(chunk > sizeof buffer) chunk = sizeof buffer;
        if(FileSetFolder(source, NULL) < 0 ||
           FileSeek(id, offset, FILE_SEEK_START) != (int32_t)offset ||
           FileReadBuffer(id, buffer, chunk) != (int32_t)chunk) goto rollback;
        if(FileSetFolder(target, NULL) < 0 ||
           FileWriteBuffer(created, buffer, chunk, FILE_WRITE_APPEND) != (int32_t)chunk ||
           FileSeek(created, offset, FILE_SEEK_START) != (int32_t)offset ||
           FileReadBuffer(created, verify, chunk) != (int32_t)chunk ||
           memcmp(buffer, verify, chunk)) goto rollback;
        offset += chunk;
    }
    if(FileQuerySize(created) != length ||
       FileSeek(created, position, FILE_SEEK_START) != position) goto rollback;
    if(FileSetFolder(source, NULL) < 0) goto finish;
    if(FileDelete(id) < 0) {
        error = "Copied; source delete failed";
        goto finish;
    }
    error = NULL;
    goto finish;
rollback:
    if(FileSetFolder(target, NULL) < 0 || FileDelete(created) < 0)
        error = "Source kept; partial copy remains";
finish:
    FileSetFolder(source, NULL);
    if(error) {
        FileSeek(id, position, FILE_SEEK_START);
        DisplayMessage(error);
    }
    RefreshFiles();
}

void ProcessMessage(Message_e message, uint32_t param, uint32_t* status) {
    int32_t previous;
    unsigned i;
    *status = 0;
    /* Return is translated to MSG_CHAR on AS3000; navigation keys use MSG_KEY. */
    if(message == MSG_CHAR && (param == '\r' || param == '\n')) {
        message = MSG_KEY;
        param = KEY_ENTER;
    }
    if(message == MSG_SETFOCUS) {
        g_state.active = true;
        g_state.files = false;
        g_state.appletCount = g_state.appletChoice = 0;
        for(i = 1; i < 32; ++i) {
            char name[38] __attribute__((aligned(2)));
            if(AppletGetName(i, name)) g_state.applets[g_state.appletCount++] = i;
        }
        RedrawScreen();
        return;
    }
    if(message == MSG_KILLFOCUS) { g_state.active = false; return; }
    if(!g_state.active) return;
    if(!g_state.files) {
        if(message != MSG_KEY || (param & KEY_MOD_KEY_UP)) return;
        if((param & 255) == KEY_UP && g_state.appletChoice) --g_state.appletChoice;
        if((param & 255) == KEY_DOWN && g_state.appletChoice + 1 < g_state.appletCount) ++g_state.appletChoice;
        if((param & 255) == KEY_ENTER && g_state.appletCount) {
            g_state.folder = g_state.applets[g_state.appletChoice];
            g_state.files = true;
            RefreshFiles();
        }
        /* Draw files only inside the selected folder below. */
        if(!g_state.files) { RedrawScreen(); return; }
    }
    previous = FileSetFolder(g_state.folder, NULL);
    if(previous < 0) { g_state.files = false; DisplayMessage("Folder unavailable"); return; }
    if(message == MSG_IDLE && g_state.scanCursor <= 65535) ScanFiles();
    if(message == MSG_KEY && !(param & KEY_MOD_KEY_UP)) {
        if((param & 255) == KEY_ESC) g_state.files = false;
        if((param & 255) == KEY_UP && g_state.choice) --g_state.choice;
        if((param & 255) == KEY_DOWN && g_state.choice + 1 < g_state.count) ++g_state.choice;
    }
    if(message == MSG_CHAR && g_state.count) {
        uint16_t id = g_state.ids[g_state.choice];
        switch(param) {
        case 'r': case 'R': RenameFile(id); break;
        case 'm': case 'M': MoveFile(id); break;
        case 'd': case 'D':
            if(AuthorizeFile(id) && ConfirmAction("Delete selected file?")) {
                if(FileDelete(id) < 0) DisplayMessage("Delete failed");
                RefreshFiles();
            }
            break;
        }
    }
    if(message != MSG_IDLE) RedrawScreen();
    FileSetFolder(previous, NULL);
}
