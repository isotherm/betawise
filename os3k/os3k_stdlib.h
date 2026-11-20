#ifndef _OS3K_STDLIB_H_
#define _OS3K_STDLIB_H_

#include "os3k_keyboard.h"

/** @file os3k_stdlib.h
 * @brief Firmware string helpers, standard-library functions, and stream I/O.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Length-delimited search request consumed by FindBufferPattern().
 *
 * The 68000 ABI places the four 32-bit fields at offsets 0, 4, 8, and 12,
 * followed by the one-byte case flag at offset 16. Neither buffer needs a
 * NUL terminator. Embedded zero bytes are compared like all other bytes.
 * The ROM reads this request without modifying it.
 */
typedef struct _BufferSearch_t {
    uint32_t pattern_length; /**< Number of pattern bytes to match, excluding any terminator. */
    const uint8_t* pattern; /**< Readable pattern_length-byte pattern. */
    uint32_t buffer_length; /**< Number of bytes in the buffer to search. */
    const uint8_t* buffer; /**< Readable buffer_length-byte search buffer. */
    bool case_sensitive; /**< True for exact bytes; false to apply the ROM's byte-wise lowercase conversion to both inputs. */
} BufferSearch_t;

/** @brief Count bytes before a NUL terminator using the early ROM helper.
 *
 * This is a separate copy of the byte-scanning implementation used by strlen(),
 * with the same semantics. The prefixed name keeps both entry points available
 * without a symbol collision.
 * @param str Readable NUL-terminated string.
 * @return Full 32-bit byte count, excluding the terminator.
 */
size_t _OS3K_strlen(const char* str);

/** @brief Find the first non-terminator byte matching a value in a string.
 *
 * Stops before comparing the terminating zero. Unlike strchr(), a search for
 * zero always returns NULL.
 * @param str Readable NUL-terminated string.
 * @param value Byte to match.
 * @return Pointer to the first match before the terminator, or NULL.
 */
const char* FindStringChar(const char* str, uint8_t value);

/** @brief Find a byte between two inclusive addresses.
 *
 * Adapts the ROM's D0 pointer result
 * to the C calling convention. This is a binary range search, not a
 * NUL-terminated string search. A zero byte does not end the search and can
 * itself be matched. A singleton range examines its one byte; a reversed
 * range returns NULL without reading data.
 * @param first First readable byte, inclusive.
 * @param last Last readable byte, inclusive, in the same buffer as first.
 * @param value Byte to match.
 * @return Pointer to the first matching byte, or NULL.
 */
const uint8_t* FindByteInRange(const uint8_t* first, const uint8_t* last, uint8_t value);

/** @brief Find the first matching pattern in a length-delimited byte buffer.
 *
 * Adapts the ROM's D0 pointer result
 * to the C calling convention. Examines every possible start position,
 * including the last one where the full pattern fits. It returns NULL for
 * an empty pattern, an empty buffer, or a pattern longer than the buffer.
 * This differs from strstr(), which accepts an empty pattern and searches
 * NUL-terminated strings.
 *
 * Case-insensitive mode folds ASCII A through Z and the ROM's additional
 * encoded character pairs, such as 0x80 to 0x84. It is a
 * byte encoding conversion, not Unicode or a host-locale comparison.
 * @param search Readable request with valid buffer and pattern pointers for
 * their stated lengths. The request pointer itself must not be NULL.
 * @return Pointer to the first matching start in search->buffer, or NULL.
 */
const uint8_t* FindBufferPattern(const BufferSearch_t* search);

/** @brief Call the unchecked ROM 32-bit numeral formatter.
 *
 * Prefer the checked FormatInt32() wrapper.
 * Decimal formatting treats value as
 * signed, including INT32_MIN, and adds a minus sign when needed. Other
 * radices format its unsigned 32-bit bit pattern. Digits above nine are
 * uppercase A through Z; no radix prefix or leading zero padding is added.
 *
 * The ROM has only ten bytes of internal digit scratch space and performs
 * no radix or capacity checks. More than ten magnitude digits corrupts its
 * stack, even when the caller supplies a larger output buffer. Decimal and
 * hexadecimal support every 32-bit input. For other radices, the unsigned
 * magnitude must fit in ten digits; for example, binary is limited to 1023.
 * @param value Signed decimal value or the bit pattern for another radix.
 * @param radix Base from 2 through 36, subject to the ten-digit limit. Zero
 * and one are invalid and can cause division errors or a nonterminating loop.
 * @param buffer Writable output storage. Twelve bytes suffice for every
 * supported input: ten magnitude digits, an optional minus sign, and NUL.
 */
void _OS3K_FormatInt32(int32_t value, uint32_t radix, char* buffer);

/** @brief Format a 32-bit numeral after checking the radix is safe for all inputs.
 *
 * Betawise wrapper for _OS3K_FormatInt32(). Accepts only bases 10 through 36,
 * ensuring that every 32-bit input fits the ROM's ten-digit scratch buffer.
 * Rejected bases return before calling the ROM or accessing buffer. Decimal
 * output is signed; other bases format the unsigned bit pattern using
 * uppercase digits, without a radix prefix or leading zero padding.
 * @param value Signed decimal value or the bit pattern for another radix.
 * @param radix Base from 10 through 36. Lower and higher bases are rejected.
 * @param buffer Writable storage of at least twelve bytes for accepted bases.
 * Left untouched when the radix is rejected.
 * @return True when formatted, false when the radix was rejected.
 */
bool FormatInt32(int32_t value, uint32_t radix, char* buffer);

/** @brief Opaque stream token. The ROM encodes stream numbers as pointers. */
typedef void FILE;
/** @brief Keyboard input stream token. */
#define stdin   ((FILE*)0)
/** @brief LCD output stream token. */
#define stdout  ((FILE*)1)
/** @brief Secondary ROM output stream token. */
#define stderr  ((FILE*)2)

/** @brief Read the ROM's error number without clearing it.
 * @return The current ROM error number.
 */
int _OS3K_errno(void);

/** @brief Read the ROM's error number. This expression is read-only. */
#define errno (_OS3K_errno())

/** @brief Read one translated keyboard character through the ROM.
 * @return Character code.
 */
int _OS3K_getchar(void);

/** @brief Read one translated keyboard character and echo it.
 * Uses the ROM output stream for echo.
 * @return The character as a sign-extended byte.
 */
int getchar_echo(void);

/** @brief Abandon the current applet invocation and transfer control to the firmware.
 * The ROM resets the stack and re-enters its applet control path. This call does not return to its caller.
 */
void abort(void);

/** @brief Parse an initial signed decimal integer.
 * Skips leading whitespace and accepts an optional sign. The ROM does not report overflow for this call.
 * @param str NUL-terminated input.
 * @return Parsed integer, or zero if no digits are consumed.
 */
int atoi(const char* str);

/** @brief Parse an initial signed decimal long.
 * Skips leading whitespace and accepts an optional sign. The ROM does not report overflow for this call.
 * @param str NUL-terminated input.
 * @return Parsed long, or zero if no digits are consumed.
 */
long atol(const char* str);

/** @brief Scan formatted input from a string.
 * A decimal conversion that overflows a 32-bit signed destination saturates and sets the ROM error number.
 * @param str NUL-terminated input.
 * @param fmt ROM scan format.
 * @param ... Pointers to the conversion destinations.
 * @return Number of assignments, or a negative result on input failure.
 */
int sscanf(const char* str, const char* fmt, ...);

/** @brief Read one byte from the ROM input stream.
 * When its buffer is empty, the ROM collects an echoed input line of up to 80 bytes. The returned line terminator can be a carriage return.
 * @param stream Use stdin.
 * @return The next byte as an unsigned value, or -1 for an unsupported stream.
 */
int _OS3K_fgetc(FILE* stream);

/** @brief Write formatted output to a ROM stream.
 * @param stream Output stream token.
 * @param fmt ROM print format.
 * @param ... Values required by the format.
 * @return Number of characters written, or a negative result on output failure.
 */
int fprintf(FILE* stream, const char* fmt, ...);

/** @brief Write one character to a ROM stream.
 * @param c Character value. The ROM writes the low byte.
 * @param stream Output stream token.
 * @return Character value on success, or -1 on failure.
 */
int _OS3K_fputc(int c, FILE* stream);

/** @brief Scan formatted input from a ROM stream.
 * @param stream Use stdin.
 * @param fmt ROM scan format.
 * @param ... Pointers to the conversion destinations.
 * @return Number of assignments, or a negative result on input failure.
 */
int fscanf(FILE* stream, const char* fmt, ...);

/** @brief Search a byte range for the first matching byte.
 * @param ptr Start of the readable range.
 * @param value Value to match, reduced to a byte.
 * @param num Number of bytes to examine.
 * @return Pointer to the first match, or NULL.
 */
void* memchr(const void* ptr, int value, size_t num);

/** @brief Compare two byte ranges.
 * @param ptr1 First range.
 * @param ptr2 Second range.
 * @param num Number of bytes to compare.
 * @return -1, 0, or 1 according to unsigned byte order.
 */
int memcmp(const void* ptr1, const void* ptr2, size_t num);

/** @brief Copy bytes between nonoverlapping ranges.
 * Use memmove() for overlapping ranges.
 * @param dst Writable destination.
 * @param src Readable source.
 * @param num Number of bytes to copy.
 * @return The destination pointer.
 */
void* memcpy(void* dst, const void* src, size_t num);

/** @brief Copy bytes between ranges that may overlap.
 * @param dst Writable destination.
 * @param src Readable source.
 * @param num Number of bytes to copy.
 * @return The destination pointer.
 */
void* memmove(void* dst, const void* src, size_t num);

/** @brief Fill a byte range.
 * @param ptr Start of the writable range.
 * @param value Fill value, reduced to a byte.
 * @param num Number of bytes to fill.
 * @return The original pointer.
 */
void* memset(void* ptr, int value, size_t num);

/** @brief Write formatted output to the ROM output stream.
 * @param fmt ROM print format.
 * @param ... Values required by the format.
 * @return Number of characters written, or a negative result on output failure.
 */
int printf(const char* fmt, ...);

/** @brief Advance the ROM pseudorandom generator.
 * @return A value from 0 through 32767.
 */
int rand(void);

/** @brief Scan formatted input from the ROM input stream.
 * @param fmt ROM scan format.
 * @param ... Pointers to the conversion destinations.
 * @return Number of assignments, or a negative result on input failure.
 */
int scanf(const char* fmt, ...);

/** @brief Format a NUL-terminated string into a caller buffer.
 * @param str Writable destination with room for the complete result and terminator.
 * @param fmt ROM print format.
 * @param ... Values required by the format.
 * @return Number of characters produced, excluding the terminator.
 */
int sprintf(char* str, const char* fmt, ...);

/** @brief Set the seed of the ROM pseudorandom generator.
 * @param seed 32-bit seed.
 */
void srand(unsigned int seed);

/** @brief Append a NUL-terminated string.
 * @param dst Writable string with room for both strings and the terminator.
 * @param src NUL-terminated suffix.
 * @return The destination pointer.
 */
char* strcat(char* dst, const char* src);

/** @brief Find the first occurrence of a byte in a string.
 * Betawise bridges the ROM D0 pointer result to the compiler's A0 return ABI.
 * @param str NUL-terminated string.
 * @param c Value to match, reduced to a byte. Zero can match the terminator.
 * @return Pointer to the first match, or NULL.
 */
char* strchr(const char* str, int c);

/** @brief Compare two NUL-terminated strings.
 * @param str1 First string.
 * @param str2 Second string.
 * @return Zero if equal, or a signed result ordered by the first differing unsigned byte.
 */
int strcmp(const char* str1, const char* str2);

/** @brief Copy a string including its NUL terminator.
 * @param dst Writable destination with room for the complete string.
 * @param src NUL-terminated source.
 * @return The destination pointer.
 */
char* strcpy(char* dst, const char* src);

/** @brief Count the bytes before a string terminator.
 * @param str NUL-terminated string.
 * @return Byte count excluding the terminator.
 */
size_t strlen(const char* str);

/** @brief Append up to a specified number of bytes from a string.
 * @param dst Writable NUL-terminated string with room for the suffix and terminator.
 * @param src NUL-terminated suffix.
 * @param num Maximum source bytes to append.
 * @return The destination pointer.
 */
char* strncat(char* dst, const char* src, size_t num);

/** @brief Compare at most a specified number of string bytes.
 * @param str1 First string.
 * @param str2 Second string.
 * @param num Maximum number of bytes to compare.
 * @return Zero if equal over the compared range, or a signed result ordered by the first differing unsigned byte.
 */
int strncmp(const char* str1, const char* str2, size_t num);

/** @brief Copy up to a specified number of string bytes.
 * Pads with NUL bytes when the source ends early. A source of num bytes or longer leaves the result unterminated.
 * @param dst Writable destination with room for num bytes.
 * @param src Source string or a readable range of num bytes.
 * @param num Number of destination bytes to write.
 * @return The destination pointer.
 */
char* strncpy(char* dst, const char* src, size_t num);

/** @brief Find the last occurrence of a byte in a string.
 * @param str NUL-terminated string.
 * @param c Value to match, reduced to a byte. Zero can match the terminator.
 * @return Pointer to the last match, or NULL.
 */
char* strrchr(const char* str, int c);

/** @brief Find the first occurrence of a substring.
 * @param str1 NUL-terminated string to search.
 * @param str2 NUL-terminated substring.
 * @return Pointer to the match, the original string for an empty substring, or NULL if absent.
 */
char* strstr(const char* str1, const char* str2);

/** @brief Convert an ASCII uppercase letter to lowercase.
 * @param c Character value.
 * @return Lowercase equivalent for ASCII A through Z. All other values are returned unchanged.
 */
int tolower(int c);

/** @brief Convert an ASCII lowercase letter to uppercase.
 * @param c Character value.
 * @return Uppercase equivalent for ASCII a through z. All other values are returned unchanged.
 */
int toupper(int c);

/** @brief Push a byte back into the ROM input buffer.
 * @param c Value whose low byte is stored. A value of -1 is rejected.
 * @param stream Use stdin.
 * @return The original value, or -1 if the stream or value is rejected or no consumed buffer slot is available.
 */
int ungetc(int c, FILE* stream);

// Betawise wrappers.

/** @brief Encode a physical key as terminal bytes (no terminating NUL).
 * Returns 0..8 bytes, including ASCII Ctrl codes and CSI arrow sequences.
 * Ignores releases; Applets invokes the normal system menu.
 */
size_t ConsoleKeySequence(KeyMod_e key, char output[8]);

/** @brief Read a queued terminal reply, or -1 if none is pending. Nonblocking.
 * ESC [ 6 n queues ESC [ row ; col R for stdin; no keyboard events are consumed.
 */
int ConsoleReadPending(void);

/** @brief Read an un-echoed terminal byte; stdin includes arrow sequences and replies.
 * Other stream tokens retain the original ROM fgetc behavior.
 */
int fgetc(FILE* stream);
int getc(FILE* stream);

/** @brief Read up to size-1 bytes, retaining the first CR or LF, then NUL terminate.
 * No editing or echo. Returns NULL on failure before any byte or size <= 0.
 * size == 1 writes just NUL. Input control sequences remain bytes.
 */
char* fgets(char* str, int size, FILE* stream);

/** @brief Read one character through Betawise's keyboard helpers.
 * @return An unsigned terminal byte, including encoded key sequences.
 */
int getchar(void);

/** @brief Write one character using Betawise's console output path.
 *
 * The stdout parser consumes basic VT100 cursor/erase sequences and delegates other
 * streams to _OS3K_fputc().
 * @param c Character value.
 * @param stream Output stream token.
 * @return Character value on success, or -1 on failure.
 */
int fputc(int c, FILE* stream);

/** @brief Betawise wrapper that writes a string through fputc().
 * @param str NUL-terminated string.
 * @param stream Output stream token.
 * @return Zero on success or a negative output error.
 */
int fputs(const char* str, FILE* stream);

/** @brief Betawise wrapper that writes a character through fputc() to stdout.
 * @param c Character value.
 * @return Character value on success or a negative output error.
 */
int putchar(int c);

/** @brief Betawise wrapper that writes a string through fputs() to stdout.
 *
 * The caller supplies any trailing newline.
 * @param str NUL-terminated string.
 * @return Zero on success or a negative output error.
 */
int puts(const char* str);

#endif
