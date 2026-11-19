#include "os3k_applet.h"
#include "os3k_dialog.h"
#include "os3k_file.h"
#include "os3k_markup.h"
#include "os3k_stdlib.h"

/* ROM pointer results arrive in D0; this GCC target returns pointers in A0.
 * Integer raw declarations make GCC perform the conversion, including NULL.
 * Keep the raw calls out of public headers. An A-line entry returns through
 * the caller's stack return address, not the instruction after its opcode;
 * these must remain ordinary calls to separate raw syscall stubs.
 */

extern uintptr_t _OS3K_AppletGetResources(void);

const AppletResource_t* AppletGetResources(void) {
    return (const AppletResource_t*)_OS3K_AppletGetResources();
}

extern uintptr_t _OS3K_DialogGetItemText(uint8_t index, uint8_t* text_len);

const char* DialogGetItemText(uint8_t index, uint8_t* text_len) {
    return (const char*)_OS3K_DialogGetItemText(index, text_len);
}

extern uintptr_t _OS3K_FontDialogCreate(FontDialogLayout_e layout);

FontDialog_t* FontDialogCreate(FontDialogLayout_e layout) {
    return (FontDialog_t*)_OS3K_FontDialogCreate(layout);
}

extern uintptr_t _OS3K_FindByteInRange(const uint8_t* first, const uint8_t* last,
                                       uint8_t value);

const uint8_t* FindByteInRange(const uint8_t* first, const uint8_t* last,
                               uint8_t value) {
    return (const uint8_t*)_OS3K_FindByteInRange(first, last, value);
}

extern uintptr_t _OS3K_FindBufferPattern(const BufferSearch_t* search);

const uint8_t* FindBufferPattern(const BufferSearch_t* search) {
    return (const uint8_t*)_OS3K_FindBufferPattern(search);
}

extern uintptr_t _OS3K_FileBindState(uint16_t file_id, void** data, uint32_t* length,
                                     uint32_t* capacity, uint32_t* position);

void* FileBindState(uint16_t file_id, void** data, uint32_t* length,
                    uint32_t* capacity, uint32_t* position) {
    return (void*)_OS3K_FileBindState(file_id, data, length, capacity, position);
}

extern uintptr_t _OS3K_FileAccessName(uint16_t file_id, char* name, uint8_t rename);

char* FileAccessName(uint16_t file_id, char* name, uint8_t rename) {
    return (char*)_OS3K_FileAccessName(file_id, name, rename);
}

extern uintptr_t _OS3K_ClipboardGetData(void);

void* ClipboardGetData(void) {
    return (void*)_OS3K_ClipboardGetData();
}

extern uintptr_t _OS3K_memchr(const void* ptr, int value, size_t num);

void* memchr(const void* ptr, int value, size_t num) {
    return (void*)_OS3K_memchr(ptr, value, num);
}

extern uintptr_t _OS3K_memcpy(void* dst, const void* src, size_t num);

void* memcpy(void* dst, const void* src, size_t num) {
    return (void*)_OS3K_memcpy(dst, src, num);
}

extern uintptr_t _OS3K_memmove(void* dst, const void* src, size_t num);

void* memmove(void* dst, const void* src, size_t num) {
    return (void*)_OS3K_memmove(dst, src, num);
}

extern uintptr_t _OS3K_memset(void* ptr, int value, size_t num);

void* memset(void* ptr, int value, size_t num) {
    return (void*)_OS3K_memset(ptr, value, num);
}

extern uintptr_t _OS3K_strcat(char* dst, const char* src);

char* strcat(char* dst, const char* src) {
    return (char*)_OS3K_strcat(dst, src);
}

extern uintptr_t _OS3K_strchr(const char* str, int c);

char* strchr(const char* str, int c) {
    return (char*)_OS3K_strchr(str, c);
}

extern uintptr_t _OS3K_strcpy(char* dst, const char* src);

char* strcpy(char* dst, const char* src) {
    return (char*)_OS3K_strcpy(dst, src);
}

extern uintptr_t _OS3K_strncat(char* dst, const char* src, size_t num);

char* strncat(char* dst, const char* src, size_t num) {
    return (char*)_OS3K_strncat(dst, src, num);
}

extern uintptr_t _OS3K_strncpy(char* dst, const char* src, size_t num);

char* strncpy(char* dst, const char* src, size_t num) {
    return (char*)_OS3K_strncpy(dst, src, num);
}

extern uintptr_t _OS3K_strstr(const char* str, const char* pattern);

char* strstr(const char* str, const char* pattern) {
    return (char*)_OS3K_strstr(str, pattern);
}

/* Native markup results use hidden A1 storage but leave A0 = storage + 4.
 * Return a fresh C result to supply GCC's normal aggregate return convention,
 * zero the reserved byte, and suppress undefined offsets on success.
 */
extern MarkupResult_t _OS3K_MarkupParse(const char* begin, const char* end,
                                        MarkupElement_t* elements, int8_t capacity,
                                        MarkupFilter_t filter);
extern MarkupResult_t _OS3K_MarkupGetAttribute(const MarkupElement_t* element,
                                               const char* name, char* output,
                                               int16_t capacity, int16_t* length);
extern MarkupResult_t _OS3K_MarkupGetText(const MarkupElement_t* element,
                                          char* output, int16_t capacity, int16_t* length);
extern MarkupResult_t _OS3K_MarkupFindElements(const MarkupElement_t* parent,
                                               const char* path, MarkupElement_t* elements,
                                               int8_t capacity, int8_t* count);
extern MarkupResult_t _OS3K_MarkupGetChildAttribute(const MarkupElement_t* parent,
                                                    const char* path, const char* name,
                                                    char* output, uint32_t ignored,
                                                    int16_t* length);
extern MarkupResult_t _OS3K_MarkupGetChildText(const MarkupElement_t* parent,
                                               const char* path, char* output,
                                               int16_t capacity, int16_t* length);

static MarkupResult_t normalizeMarkupResult(MarkupResult_t native) {
    MarkupResult_t result = {native.status, 0, 0};

    if (native.status != MARKUP_OK) {
        result.offset = native.offset;
    }
    return result;
}

MarkupResult_t MarkupParse(const char* begin, const char* end,
                           MarkupElement_t* elements, int8_t capacity,
                           MarkupFilter_t filter) {
    return normalizeMarkupResult(_OS3K_MarkupParse(begin, end, elements, capacity, filter));
}

MarkupResult_t MarkupGetAttribute(const MarkupElement_t* element, const char* name,
                                  char* output, int16_t capacity, int16_t* length) {
    return normalizeMarkupResult(_OS3K_MarkupGetAttribute(element, name, output,
                                                          capacity, length));
}

MarkupResult_t MarkupGetText(const MarkupElement_t* element, char* output,
                             int16_t capacity, int16_t* length) {
    return normalizeMarkupResult(_OS3K_MarkupGetText(element, output, capacity, length));
}

MarkupResult_t MarkupFindElements(const MarkupElement_t* parent, const char* path,
                                  MarkupElement_t* elements, int8_t capacity,
                                  int8_t* count) {
    return normalizeMarkupResult(_OS3K_MarkupFindElements(parent, path, elements,
                                                          capacity, count));
}

MarkupResult_t MarkupGetChildAttribute(const MarkupElement_t* parent, const char* path,
                                       const char* name, char output[18], int16_t* length) {
    return normalizeMarkupResult(_OS3K_MarkupGetChildAttribute(parent, path, name,
                                                               output, 0, length));
}

MarkupResult_t MarkupGetChildText(const MarkupElement_t* parent, const char* path,
                                  char* output, int16_t capacity, int16_t* length) {
    return normalizeMarkupResult(_OS3K_MarkupGetChildText(parent, path, output,
                                                          capacity, length));
}
