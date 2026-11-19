#ifndef _OS3K_FILE_H_
#define _OS3K_FILE_H_

/** @file os3k_file.h
 * @brief AlphaSmart firmware file and clipboard operations.
 *
 * File IDs are resolved in the selected applet table except for the clipboard ID.
 * File positions, lengths, and capacities are measured in bytes.
 */

#include <stdbool.h>
#include <stdint.h>

/** @brief Reference position for FileSeek(). */
typedef enum _FileSeekOrigin_e {
    FILE_SEEK_START = 0, /**< Measure from the start of the file. */
    FILE_SEEK_CURRENT = 1, /**< Measure from the current position. */
    FILE_SEEK_END = 2 /**< Measure from the logical end of the file. */
} FileSeekOrigin_e;

/** @brief Placement policy for FileWriteBuffer() and ClipboardPaste(). */
typedef enum _FileWriteMode_e {
    FILE_WRITE_APPEND = 1, /**< Append at EOF and preserve the current position. */
    FILE_WRITE_INSERT = 2, /**< Insert at the current position, shifting later bytes and advancing the position. */
    FILE_WRITE_OVERWRITE = 3 /**< Overwrite from the current position, extending the file if needed and advancing the position. */
} FileWriteMode_e;

/** @brief Position policy for FileOpen(). */
typedef enum _FileOpenMode_e {
    FILE_OPEN_KEEP_POSITION = 0, /**< Keep the stored position. */
    FILE_OPEN_REWIND = 1 /**< Set the position to zero. */
} FileOpenMode_e;

/** @brief Conversion policy for clipboard reads, length queries, and paste.
 *
 * Plain mode runs the firmware text converter. It strips internal formatting
 * markers and translates some encoded characters to text equivalents. For
 * example, it removes the `B1 02 B1` and `B2 02 B2` read-only markers while
 * preserving the text between them. The converted byte count can differ from
 * the stored length. Raw mode preserves the encoded bytes exactly.
 */
typedef enum _ClipboardTextMode_e {
    CLIPBOARD_TEXT_RAW = 1, /**< Use the stored bytes as-is. */
    CLIPBOARD_TEXT_PLAIN = 3 /**< Strip internal formatting and translate encoded text. */
} ClipboardTextMode_e;

/** @brief Resolve the current file in the active applet table. */
#define FILE_ID_CURRENT   0x0000
/** @brief Resolve the clipboard record directly. */
#define FILE_ID_CLIPBOARD 0x00CB
/** @brief Query the current file's saved length with FileQuerySize(). */
#define FILE_SIZE_SAVED_CURRENT 0x00FC
/** @brief Sum logical lengths in the active applet table with FileQuerySize(). */
#define FILE_SIZE_FOLDER_LENGTH 0x00FD
/** @brief Sum the greater of logical and reserved length per entry in the active table. */
#define FILE_SIZE_FOLDER_WITH_RESERVATIONS 0x00FE
/** @brief Sum the greater of logical and reserved length per entry across the file table. */
#define FILE_SIZE_ALL_WITH_RESERVATIONS 0x00FF
/** @brief Apply the saved length to the current file, exposing retained bytes. */
#define FILE_LENGTH_RESTORE_SAVED (-2)
/** @brief Record the current logical length as the saved length. */
#define FILE_LENGTH_SAVE_CURRENT  (-3)
/** @brief Set the reservation to the greater of the current logical length and 512 bytes. */
#define FILE_LENGTH_RESERVE_CURRENT (-5)

/** @brief One entry in the firmware RAM file table.
 *
 * The ROM uses a 0x48-byte stride. Address fields contain 32-bit target
 * addresses. A zero mirror address disables the corresponding output.
 */
typedef struct _FileInfo_t {
    uint32_t data_address; /**< 0x00, address of the file backing buffer. */
    uint32_t length; /**< 0x04, bytes in use. */
    uint32_t saved_length; /**< 0x08, byte length used by restore. */
    uint32_t capacity; /**< 0x0C, bytes available in the allocation. */
    uint32_t reserved_length; /**< 0x10, minimum bytes retained by the allocator. */
    uint32_t position; /**< 0x14, byte offset for sequential I/O. */
    char password[7]; /**< 0x18, up to six bytes plus NUL. */
    uint8_t unknown_1f; /**< 0x1F, opaque reserved byte; preserve it. */
    uint32_t flags; /**< 0x20, bit 0x02 marks the fallback current file. */
    uint32_t data_address_mirror; /**< 0x24, optional RAM address of a data pointer. */
    uint32_t length_mirror; /**< 0x28, optional RAM address of a length. */
    uint32_t capacity_mirror; /**< 0x2C, optional RAM address of a capacity. */
    uint32_t position_mirror; /**< 0x30, optional RAM address of a position. */
    char name[16]; /**< 0x34, NUL-terminated. */
    uint16_t file_id; /**< 0x44, resolved within the active applet's file table. */
    uint16_t tag; /**< 0x46, set/read by A1F4 and matched by A1F8. */
} FileInfo_t;

/** @brief Seek within a file.
 * @param file_id File ID.
 * @param offset Signed byte displacement from origin.
 * @param origin Reference point for offset.
 * @return New position clamped to the file bounds, or a negative error.
 */
int32_t FileSeek(uint16_t file_id, int32_t offset, FileSeekOrigin_e origin);

/** @brief Get the current byte position of a file.
 * @param file_id File ID.
 * @return Byte position or a negative error.
 */
int32_t FileTell(uint16_t file_id);

/** @brief Write bytes using the selected placement policy.
 * @param file_id File ID.
 * @param source Buffer containing byte_count readable bytes.
 * @param byte_count Requested number of bytes.
 * @param mode Append, insert, or overwrite policy.
 * @return Bytes written or a negative error.
 */
int32_t FileWriteBuffer(uint16_t file_id, const void* source, uint32_t byte_count,
                        FileWriteMode_e mode);

/** @brief Read bytes from the file position and advance it.
 * @param file_id File ID.
 * @param destination Writable destination with room for byte_count bytes.
 * @param byte_count Maximum bytes to read.
 * @return Bytes read, possibly fewer at EOF, or a negative error.
 */
int32_t FileReadBuffer(uint16_t file_id, void* destination, uint32_t byte_count);

/** @brief Zero the current file allocation and reset its length, saved length, and position.
 * @return Resolved file ID or a negative error.
 */
int32_t FileEraseCurrent(void);

/** @brief Remove bytes starting at the current file position.
 * @param byte_count Maximum number of bytes to remove.
 * @return Bytes removed or a negative error.
 */
int32_t FileDeleteAtPosition(uint32_t byte_count);

/** @brief Resolve the current file in the active applet table.
 * @return Resolved file ID or a negative error.
 */
int32_t FileGetCurrentId(void);

/** @brief Get unallocated file RAM plus reclaimable allocation space.
 * @return Available bytes.
 */
uint32_t FileGetFreeSpace(void);

/** @brief Get the unused space in a file's allocation.
 * @param file_id File ID.
 * @return Capacity minus logical length, or zero on lookup failure.
 */
uint32_t FileGetUnusedCapacity(uint16_t file_id);

/** @brief Get a file length or a size total.
 * @param file_id_or_selector File ID or one of the FILE_SIZE_* selectors.
 * @return Length or selected total in bytes, or a negative error.
 */
int32_t FileQuerySize(uint16_t file_id_or_selector);

/** @brief Set the current file length or apply a length selector.
 * Shrinking clamps the current position. Restoring a length exposes retained bytes.
 * @param length_or_selector Nonnegative length capped at capacity, or a FILE_LENGTH_* selector.
 * @return Resulting length for the documented selectors, or a negative error.
 */
int32_t FileSetCurrentLength(int32_t length_or_selector);

/** @brief Get the total capacity of a file's allocation.
 * @param file_id File ID.
 * @return Capacity in bytes or a negative error.
 */
int32_t FileGetCapacity(uint16_t file_id);

/** @brief Select an applet's file table and clear the cached open file.
 * @param applet_index Index of the destination applet.
 * @param applet_flags Optional output for the destination applet's flags.
 * @return Previous applet index or a negative error.
 */
int32_t FileSetFolder(uint32_t applet_index, uint32_t* applet_flags);

/** @brief Bind optional applet variables to the file's live state.
 * The ROM retains nonnull output addresses and refreshes those variables after changes. Keep the storage valid while bound.
 * @param file_id File ID.
 * @param data Optional address of a variable receiving the data pointer.
 * @param length Optional address of a variable receiving logical length.
 * @param capacity Optional address of a variable receiving capacity.
 * @param position Optional address of a variable receiving the byte position.
 * @return Current data pointer, or NULL if lookup fails.
 */
void* FileBindState(uint16_t file_id, void** data, uint32_t* length,
                    uint32_t* capacity, uint32_t* position);

/** @brief Select a file as current.
 * @param file_id File ID.
 * @param mode Whether to retain the stored position or rewind.
 * @return Resolved file ID or a negative error.
 */
int32_t FileOpen(uint16_t file_id, FileOpenMode_e mode);

/** @brief Clear the cached current file pointer.
 * Current-file lookup can still use the applet's last selection or the fallback-current flag.
 */
void FileClose(void);

/** @brief Set or clear file flag bits.
 * Once validated, the entire 32-bit mask is applied.
 * @param file_id File ID.
 * @param mask Mask containing at least one bit from 0x0D.
 * @param set Value 1 sets the bits. Other values clear them.
 * @return Resolved file ID or a negative error.
 */
int32_t FileUpdateFlags(uint16_t file_id, uint32_t mask, uint8_t set);

/** @brief Test whether any masked file flag is set.
 * Once validated, the entire 32-bit mask is tested.
 * @param file_id File ID.
 * @param mask Mask containing at least one bit from 0x0D.
 * @return 1 if any masked bit is set, 0 otherwise, or a negative error.
 */
int32_t FileTestFlags(uint16_t file_id, uint32_t mask);

/** @brief Prompt for the master password and reset the active table's file passwords.
 * @return Number of entries reset, zero if nothing was reset, or a negative lookup error.
 */
int32_t FileResetPasswords(void);

/** @brief Read or set a file password.
 * @param file_id File ID.
 * @param password For reads, writable storage of at least seven bytes. For writes, a NUL-terminated password of up to six bytes.
 * @param read Zero writes the password. A nonzero value reads it.
 * @return Resolved file ID or a negative error.
 */
int32_t FileAccessPassword(uint16_t file_id, char* password, uint8_t read);

/** @brief Prompt for a file or master password.
 * @param file_id Initial file ID.
 * @param single_attempt Value 1 ends the prompt after an incorrect password.
 * @param allow_escape Value 1 enables Escape as a cancel key.
 * @param allow_applet_keys Value 1 enables the additional applet key path.
 * @param allow_file_shortcuts Nonzero enables file shortcuts based on entry tags.
 * @param resolved_file_id Optional output for the resolved file ID on success.
 * @return True if authorized, false otherwise.
 */
bool FilePromptPassword(uint16_t file_id, uint8_t single_attempt, uint8_t allow_escape,
                        uint8_t allow_applet_keys, uint8_t allow_file_shortcuts,
                        uint16_t* resolved_file_id);

/** @brief Show the change-password dialog for a file.
 * @param file_id File ID.
 * @return Resolved file ID after the dialog, including on cancellation, or a negative lookup error.
 */
int32_t FilePromptChangePassword(uint16_t file_id);

/** @brief Create an empty file in the active applet table.
 * The allocation request is at least 512 bytes and at least reserved_length.
 * @param group File group. Zero selects the default group.
 * @param name NUL-terminated name. The stored name is limited to 15 bytes.
 * @param reserved_length Minimum bytes retained by the allocator.
 * @param requested_capacity Requested initial capacity.
 * @return New file ID or a negative error.
 */
int32_t FileCreate(uint8_t group, const char* name, uint32_t reserved_length,
                   uint32_t requested_capacity);

/** @brief Delete a file entry and compact the table.
 * @param file_id File ID.
 * @return Zero on success or a negative error.
 */
int32_t FileDelete(uint16_t file_id);

/** @brief Read or change the name of a file.
 * @param file_id File ID.
 * @param name Optional 16-byte output buffer when reading, or NUL-terminated input name when renaming.
 * @param rename Zero reads the name. Nonzero attempts a rename.
 * @return Pointer to the inline name, or NULL on failure.
 */
char* FileAccessName(uint16_t file_id, char* name, uint8_t rename);

/** @brief Read or set the file's 16-bit tag.
 * @param file_id File ID.
 * @param tag Tag to store when output is NULL.
 * @param output Optional tag destination. Pass NULL to write tag instead.
 * @return Zero on success or a negative error.
 */
int32_t FileAccessTag(uint16_t file_id, uint16_t tag, uint16_t* output);

/** @brief Look up a file by its tag.
 * @param tag Tag to match.
 * @return Matching file ID or a negative error.
 */
int32_t FileFindByTag(uint16_t tag);

/** @brief Resolve an ID built from two bytes in the active file table.
 * @param id_low Low byte of the file ID.
 * @param id_high High byte of the file ID.
 * @return Resolved file ID or a negative error.
 */
int32_t FileResolveId(uint8_t id_low, uint8_t id_high);

/** @brief Copy bytes from the current file position to the clipboard.
 * @param byte_count Number of bytes to copy.
 * @param format Nonzero clipboard format byte to record.
 * @return Bytes copied or a negative error.
 */
int32_t ClipboardCopy(uint32_t byte_count, uint8_t format);

/** @brief Copy bytes to the clipboard and remove them from the current file.
 * @param byte_count Number of bytes to cut, starting at the current position.
 * @param format Nonzero clipboard format byte to record.
 * @return Bytes cut or a negative error.
 */
int32_t ClipboardCut(uint32_t byte_count, uint8_t format);

/** @brief Paste clipboard data into the current file.
 * @param write_mode Placement policy for the pasted bytes.
 * @param text_mode Raw or plain-text conversion policy.
 * @return Bytes written or a negative error.
 */
int32_t ClipboardPaste(FileWriteMode_e write_mode, ClipboardTextMode_e text_mode);

/** @brief Replace the clipboard contents.
 * @param source Source buffer containing byte_count readable bytes.
 * @param byte_count Requested byte length.
 * @param format Nonzero format byte to record.
 * @return Bytes stored or a negative error.
 */
int32_t ClipboardSet(const void* source, uint32_t byte_count, uint8_t format);

/** @brief Read clipboard data into a caller buffer.
 * In the firmware, plain-text mode uses a stack-dependent output limit.
 * @param destination Writable destination buffer.
 * @param byte_count Requested byte count.
 * @param mode Raw or plain-text conversion policy.
 * @return Bytes written or a negative error.
 */
int32_t ClipboardGet(void* destination, uint32_t byte_count, ClipboardTextMode_e mode);

/** @brief Clear the clipboard length, saved length, position, and format.
 */
void ClipboardClear(void);

/** @brief Get the recorded clipboard format.
 * @return Format byte, or zero after ClipboardClear().
 */
uint8_t ClipboardGetFormat(void);

/** @brief Get the clipboard length for the selected conversion policy.
 * @param mode Raw or plain-text conversion policy.
 * @return Byte length in the chosen mode.
 */
uint32_t ClipboardGetLength(ClipboardTextMode_e mode);

/** @brief Get a pointer to the clipboard backing buffer.
 * Operations that resize file RAM can invalidate this pointer.
 * @return Current clipboard data pointer.
 */
void* ClipboardGetData(void);

/** @brief Get the current clipboard allocation capacity.
 * @return Capacity in bytes.
 */
uint32_t ClipboardGetCapacity(void);

/** @brief Resize the clipboard and update its logical and saved lengths.
 * @param byte_count Requested length in bytes.
 * @return Length granted in bytes.
 */
uint32_t ClipboardResize(uint32_t byte_count);

/** @brief Read the global file-password protection setting.
 * @return Stored setting byte; zero disables protection, nonzero enables it.
 */
uint8_t FileGetPasswordProtection(void);

/** @brief Compare text with the configured master password.
 *
 * Comparison ignores case and requires the complete string to match. This does
 * not change a password or consult FileGetPasswordProtection().
 * @param password NUL-terminated candidate; must not be NULL.
 * @return True for a match, false otherwise.
 */
bool FileCheckMasterPassword(const char* password);

/** @brief Prompt until the master password matches or Escape is pressed.
 *
 * Clears pending keys, masks up to six entered bytes, and
 * retries after an incorrect password. Changes the screen without restoring it.
 * @param row Text row for the optional prompt and input, using ROM coordinates.
 * @param prompt Optional NUL-terminated prompt; NULL omits it.
 * @return True after a matching password, false after cancellation.
 */
bool FilePromptMasterPassword(uint8_t row, const char* prompt);

/** @brief Query a named multi-user profile.
 *
 * Requires enabled multi-user mode and an existing nonempty profile. This does
 * not change the active user. The ROM does not bound the output copy.
 * @param user User slot 1–4, or zero for the active user. For zero, an active
 * named slot must exist; the ROM does not validate an unset active slot.
 * @param name Optional output for the profile name; allow 30 bytes. NULL skips copying.
 * @return Resolved slot 1–4, or zero if multi-user mode is disabled, the requested
 * slot is above 4, or the selected profile name is empty.
 */
uint8_t FileGetUserName(uint8_t user, char* name);

/** @brief Store the global file-password protection byte.
 *
 * This changes the setting only; it does not reset or replace file passwords.
 * @param enabled Zero disables protection; use one to enable it. Stored verbatim.
 */
void FileSetPasswordProtection(uint8_t enabled);

#endif
