#ifndef _OS3K_H_
#define _OS3K_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "version.h"

// The OS uses stream numbers as FILE pointers.
typedef void FILE;
#define stdin   ((FILE*)0)
#define stdout  ((FILE*)1)
#define stderr  ((FILE*)2)

// Cursor mode definitions.
typedef enum _CursorMode_e {
    CURSOR_MODE_HIDE = 0x0C,
    CURSOR_MODE_SHOW = 0x0F
} CursorMode_e;

// Key codes definitions.
typedef enum _Key_e {
    KEY_FILE_1      = 0x2D,
    KEY_FILE_2      = 0x2C,
    KEY_FILE_3      = 0x04,
    KEY_FILE_4      = 0x0F,
    KEY_FILE_5      = 0x0E,
    KEY_FILE_6      = 0x0A,
    KEY_FILE_7      = 0x01,
    KEY_FILE_8      = 0x27,
    KEY_PRINT       = 0x2B,
    KEY_SPELL_CHECK = 0x35,
    KEY_FIND        = 0x3F,
    KEY_CLEAR_FILE  = 0x34,
    KEY_HOME        = 0x1F,
    KEY_END         = 0x3E,
    KEY_APPLETS     = 0x29,
    KEY_SEND        = 0x2A,
    KEY_GRAVE       = 0x2E,
    KEY_1           = 0x38,
    KEY_2           = 0x37,
    KEY_3           = 0x36,
    KEY_4           = 0x39,
    KEY_5           = 0x2F,
    KEY_6           = 0x30,
    KEY_7           = 0x3A,
    KEY_8           = 0x31,
    KEY_9           = 0x32,
    KEY_0           = 0x33,
    KEY_MINUS       = 0x28,
    KEY_EQUAL       = 0x25,
    KEY_BACKSPACE   = 0x03,
    KEY_TAB         = 0x06,
    KEY_Q           = 0x22,
    KEY_W           = 0x21,
    KEY_E           = 0x20,
    KEY_R           = 0x23,
    KEY_T           = 0x07,
    KEY_Y           = 0x09,
    KEY_U           = 0x24,
    KEY_I           = 0x1C,
    KEY_O           = 0x1D,
    KEY_P           = 0x1E,
    KEY_LEFTBRACE   = 0x02,
    KEY_RIGHTBRACE  = 0x00,
    KEY_BACKSLASH   = 0x15,
    KEY_CAPSLOCK    = 0x05,
    KEY_A           = 0x18,
    KEY_S           = 0x17,
    KEY_D           = 0x16,
    KEY_F           = 0x19,
    KEY_G           = 0x10,
    KEY_H           = 0x11,
    KEY_J           = 0x1B,
    KEY_K           = 0x12,
    KEY_L           = 0x13,
    KEY_SEMICOLON   = 0x14,
    KEY_APOSTROPHE  = 0x0B,
    KEY_ENTER       = 0x40,
    KEY_LEFTSHIFT   = 0x08,
    KEY_Z           = 0x43,
    KEY_X           = 0x42,
    KEY_C           = 0x41,
    KEY_V           = 0x44,
    KEY_B           = 0x4E,
    KEY_N           = 0x4F,
    KEY_M           = 0x46,
    KEY_COMMA       = 0x3B,
    KEY_DOT         = 0x3D,
    KEY_SLASH       = 0x47,
    KEY_RIGHTSHIFT  = 0x45,
    KEY_CTRL        = 0x4D,
    KEY_ALT         = 0x26,
    KEY_CMD         = 0x0C,
    KEY_SPACE       = 0x4C,
    KEY_ESC         = 0x48,
    KEY_DELETE      = 0x50,
    KEY_UP          = 0x4B,
    KEY_LEFT        = 0x49,
    KEY_DOWN        = 0x0D,
    KEY_RIGHT       = 0x4A,
    KEY_NONE        = 0xFF,
} Key_e;

typedef enum _KeyMod_e {
    KEY_MOD_CTRL        = 0x8000,
    KEY_MOD_CMD         = 0x4000,
    KEY_MOD_ALT         = 0x1000,
    KEY_MOD_RIGHTSHIFT  = 0x0800,
    KEY_MOD_LEFTSHIFT   = 0x0400,
    KEY_MOD_CAPS_LOCK   = 0x0200,
    KEY_MOD_KEY_UP      = 0x0080,
    KEY_MOD_NONE        = 0x0000,
    KEY_MOD_SHIFT       = (KEY_MOD_LEFTSHIFT | KEY_MOD_RIGHTSHIFT)
} KeyMod_e;

// Applet messages.
typedef enum _Message_e {
    MSG_IDLE            = 0x00,
    MSG_INIT            = 0x18,
    MSG_SETFOCUS        = 0x19,
    MSG_KILLFOCUS       = 0x1A,
    MSG_CHAR            = 0x20,
    MSG_KEY             = 0x21,
    MSG_USB_PLUG        = 0x30001,
    MSG_USB_UNPLUG      = 0x3000C,
    MSG_MOD_SYNTHETIC   = 0x1000000
} Message_e;

void ProcessMessage(Message_e message, uint32_t param, uint32_t* status);

// Raster op codes.
typedef enum _RopCode_e {
    ROP_CAPTURE = 1,
    ROP_DSTINVERT = 2,
    ROP_SRCINVERT = 3,
    ROP_NOTSRCCOPY = 4,
    ROP_WHITENESS = 5,
    ROP_BLACKNESS = 6,
    ROP_SRCPAINT = 7
} RopCode_e;

// System internal calls.
typedef enum _SysInt_e {
    SYS_INT_GET_HW_LEVEL            = 0x0000,
    SYS_INT_GET_CONTRAST            = 0x0100,
    SYS_INT_SET_CONTRAST            = 0x0200,
    SYS_INT_GET_ROW_HEIGHT          = 0x0300,
    SYS_INT_GET_LCD_HEIGHT          = 0x0400,
    SYS_INT_GET_LCD_WIDTH           = 0x0500,
    SYS_INT_GET_ROW_COUNT           = 0x0600,
    SYS_INT_GET_SYSTEM_FONT_PTR     = 0x0700,
    SYS_INT_GET_ROM_SIZE            = 0x0800,
    SYS_INT_IS_NEO_2                = 0x0900,
    SYS_INT_WIRELESS_DISPLAY_STATUS = 0x0A00,
    SYS_INT_WIRELESS_TURN_ON        = 0x0B00,
    //SYS_INT_0C00                  = 0x0C00,
    //SYS_INT_0D00                  = 0x0D00,
    //SYS_INT_0E00                  = 0x0E00,
    //SYS_INT_0F00                  = 0x0F00,
    SYS_INT_GET_FIRMWARE_VERSION    = 0x1000,
    //SYS_INT_2000                  = 0x2000,
    //SYS_INT_3000                  = 0x3000,
    SYS_INT_GET_SMALL_FONT_PTR      = 0x4000
    //SYS_INT_5000                  = 0x5000,
    //SYS_INT_8000                  = 0x8000,
    //SYS_INT_9000                  = 0x9000,
    //SYS_INT_A000                  = 0xA000
} SysInt_e;

// System functions.
void ClearScreen();
void SetCursor(uint8_t row, uint8_t col, CursorMode_e cursor_mode);
void GetCursorPos(uint8_t* row, uint8_t* col);
void PutStringCentered(uint8_t row, const char *str);
void PutChar(char c);
void PutStringRaw(const char* str);
void SetCursorMode(CursorMode_e cursor_mode);
void GetCursorMode(CursorMode_e* cursor_mode);
void ClearRowCols(uint8_t row, uint8_t col_first, uint8_t col_last);
void ClearRows(uint8_t row_first, uint8_t row_last);

void PutString(const char* str);

void DrawBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* bitmap);
void RasterOp(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* bitmap, RopCode_e rop);

void ProgressBar(uint8_t row, uint16_t value, uint16_t total);
// exit_keys is a list of key codes that exit text input. Terminated by -1 (\xFF).
char TextBox(char* buffer, uint8_t* len, uint16_t max_len, const Key_e* exit_keys, bool password);
KeyMod_e WaitForKey();
// If num is 0, strings are compared until a null terminator is reached.
char StringCompare(const char* str1, const char* str2, int case_sensitive, size_t num);
void DisplayMessage(const char* str);
KeyMod_e GetKey(bool process_special_keys);
void DrainKeyBuffer();
bool IsKeyReady();
// Returns KeyMod_e values shifted right by 8.
uint8_t GetKeyModifiers();
void ScanKeyboard();
void QueueKey(KeyMod_e key);
void SetKeyModifiers(uint16_t mask);

void SleepCentiseconds(uint32_t centiseconds);
void SleepCentimilliseconds(uint16_t centimilliseconds);
uint32_t GetUptimeSeconds();
uint32_t GetUptimeCentiseconds();
uint32_t GetUptimeMilliseconds();

void DialogInit(bool single, uint8_t row_first, uint8_t row_last, uint8_t col);
// marker is usually ' '; id is for the user; shortcut_key and file_size are usually -1.
int DialogAddItem(char* text, uint8_t text_len, char marker, int id, Key_e shortcut_key, size_t file_size);
int DialogAddExitKey(Key_e key);
void DialogSetChoice(uint8_t index);
void DialogDraw();
short DialogRun();
char DialogGetChoice();
int DialogGetChoiceId();
int DialogGetItemId(uint8_t index);

char TranslateKeyToChar(KeyMod_e key);

// These functions return an index of 0 (system applet) if not found.
uint8_t AppletFindByName(char* name, uint8_t start_index);
uint8_t AppletFindById(uint16_t id);
int AppletGetName(uint8_t index, char* name);
int AppletSendMessage(uint8_t index, Message_e message, uint32_t param, uint32_t* status);

// mask=0x8 to process special key. mask=4,2,1 unknown.
void SYS_A25C(uint8_t mask, KeyMod_e key);

uint32_t CallSysInt(uint32_t unused_zero, SysInt_e info, void* output);

// Standard library functions.
void SYS_A32C();
int getchar(void);
void SYS_A334();
void abort(void);
int atoi(const char *str);
long atol(const char *str);
int sscanf(const char* str, const char* fmt, ...);
char* fgets(char* str, int num, FILE* stream);
int fprintf(FILE* stream, const char* fmt, ...);
int fputc(int c, FILE* stream);
int fscanf(FILE* stream, const char* fmt, ...);
void* memchr(const void* ptr, int value, size_t num);
int memcmp(const void* ptr1, const void* ptr2, size_t num);
void* memcpy(void* dst, const void *src, size_t num);
void* memmove(void* dst, const void* src, size_t num);
void* memset(void* ptr, int value, size_t num);
int printf(const char *fmt, ...);
int rand(void);
int scanf(const char *fmt, ...);
int sprintf(char* str, const char *fmt, ...);
void srand(unsigned int seed);
char* strcat(char* dst, const char* src);
char* strchr(const char* str, int c);
int strcmp(const char* str1, const char* str2);
char* strcpy(char* dst, const char* src);
size_t strlen(const char* str);
char* strncat(char* dst, const char* src, size_t num);
int strncmp(const char* str1, const char* str2, size_t num);
char* strncpy(char* dst, const char* src, size_t num);
char* strrchr(const char* str, int c);
char* strstr(const char* str1, const char* str2);
int tolower(int c);
int toupper(int c);
int ungetc(int c, FILE* stream);

// Betawise library functions.
void BwProcessMessage(Message_e message, uint32_t param, uint32_t* status);
int fputs(const char* str, FILE* stream);
int putchar(int c);
int puts(const char* str);

// Header definitions.
typedef struct _AppletHeader_t {
    uint32_t signature;
    uint32_t romUsage;
    uint32_t ramUsage;
    uint32_t settingsOffset;
    uint32_t flags;
    uint16_t id;
    uint8_t headerVersion;
    uint8_t fileCount;
    char name[36];
    struct _Version_t {
        uint8_t versionMajor;
        uint8_t versionMinor;
        char versionRevision[1];
    } version;
    uint8_t languageId;
    char info[60];
    uint32_t minAsmVersion;
    uint32_t fileUsage;
    void* entryPoint;
    uint32_t magic[3];
} AppletHeader_t;

// Font definitions.
#define APPLET_FONT_NAME_PTR (&__header.name[11])
typedef struct _FontHeader_t {
    uint8_t height;
    uint8_t max_width;
    uint8_t max_bytes;
    uint8_t padding;
    const uint8_t* width_table;
    const uint16_t* offset_table;
    const uint8_t* bitmap_data;
} FontHeader_t;

// Global data definitions.
typedef struct _BwGlobalData_t {
    FontHeader_t* font;
    uint16_t lcd_width;
    uint16_t lcd_height;
    uint16_t roll;
    uint8_t row;
    uint8_t col;
    uint8_t row_count;
    uint8_t col_count;
} BwGlobalData_t;

#define GLOBAL_DATA_BEGIN \
    struct _GlobalData_t { \
        BwGlobalData_t __bw_private;
#define GLOBAL_DATA_END \
    } __bw_global_data;

extern char __os3k_rom_size;
extern char __os3k_bss_size;

#define APPLET_HEADER_BEGIN \
    AppletHeader_t __header __attribute__ ((section("os3k_header"))) = { \
        .signature = 0xC0FFEEAD, \
        .romUsage = (uint32_t)&__os3k_rom_size, \
        .ramUsage = (uint32_t)&__os3k_bss_size, \
        .settingsOffset = 0, \
        .flags = 0xFF000000, \
        .headerVersion = 1, \
        .fileCount = 0, \
        .name = "Test", \
        .minAsmVersion = 0, \
        .fileUsage = 0, \
        .entryPoint = &BwProcessMessage, \
        .magic = {0, 1, 2},
#define APPLET_FLAGS(param) .flags = 0xFF000000 | (param),
#define APPLET_ID(param) .id = param,
#define APPLET_NAME(param) .name = param,
#define APPLET_FONT_NAME(param) .name = "Neo Font - " param,
#define APPLET_INFO(param) .info = param,
#define APPLET_VERSION(major, minor, revision) .version = {major, minor, revision},
#define APPLET_LANGUAGE_EN_US .languageId = 1,
#define APPLET_LANGUAGE_EN_UK .languageId = 2,
#define APPLET_LANGUAGE_FR .languageId = 3,
#define APPLET_LANGUAGE_FR_CR .languageId = 4,
#define APPLET_LANGUAGE_IT .languageId = 5,
#define APPLET_LANGUAGE_DE .languageId = 6,
#define APPLET_LANGUAGE_ES .languageId = 7,
#define APPLET_LANGUAGE_NL .languageId = 8,
#define APPLET_LANGUAGE_SV .languageId = 9,
#define APPLET_HEADER_END \
    }; \
    uint32_t __footer __attribute__ ((section("os3k_footer"))) = { 0xCAFEFEED }; \

#define APPLET_FLAG_HIDDEN 0x001
#define APPLET_FLAG_FONT (0x010 | 0x020)
#define APPLET_FLAG_ALLOW_FONT_DIALOG 0x080
#define APPLET_FLAG_WIRELESS_REQUIRED 0x100

// There are two ST7565R LCD controllers, for left and right half of screen.
// All interaction is via command registers, unless reading/writing buffer.
#define LCD_CMD_REG_LEFT (*(volatile uint8_t*)0x1008000)
#define LCD_CMD_REG_RIGHT (*(volatile uint8_t*)0x1000000)
#define LCD_CMD_ON(yes) (0xAE | ((yes) & 1))
#define LCD_CMD_REVERSE(yes) (0xA6 | ((yes) & 1))
#define LCD_CMD_ALL_PIX_ON(yes) (0xA4 | ((yes) & 1))
#define LCD_CMD_START_LINE(line) (0x40 | ((line) & 0x3F))
#define LCD_CMD_PAGE_ADDR(pageaddr) (0xB0 | ((pageaddr) & 0x0F))
#define LCD_CMD_COL_ADDR_HI(coladdr) (0x10 | (((coladdr) >> 4) & 0x0F)
#define LCD_CMD_COL_ADDR_LO(coladdr) (0x00 | ((coladdr) & 0x0F)
#define LCD_CMD_COL_ADDR_INC 0xE0
#define LCD_CMD_COL_ADDR_END 0xEE
#define LCD_DATA_REG_LEFT (*(volatile uint8_t*)0x1008001)
#define LCD_DATA_REG_RIGHT (*(volatile uint8_t*)0x1000001)

// Global data pointer (applets must store global static data in this struct)
#ifdef BETAWISE_LIB
typedef struct _BwGlobalData_t GlobalData_t;
#else
typedef struct _GlobalData_t GlobalData_t;
#endif
register GlobalData_t* gd asm("a5");

#endif
