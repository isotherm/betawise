#ifndef _FILE_MANAGER_FLASH_H_
#define _FILE_MANAGER_FLASH_H_

/** @file Flash.h
 * @brief Experimental direct AS3000 Intel 28F008B3 archive storage, not syscalls.
 * Supports only the 8-bit 1-MiB mapping and Intel IDs 89:D2 / 89:D3.
 * Physical VPP/WP# wiring and operation still require qualification.
 */
#include <stdint.h>

enum {
    FLASH_OK = 0,
    FLASH_UNSUPPORTED = -1,
    FLASH_INVALID_RANGE = -2,
    FLASH_NEEDS_ERASE = -3,
    FLASH_DEVICE_ERROR = -4,
    FLASH_WAIT_EXCEEDED = -5,
    FLASH_VERIFY_ERROR = -6,
    FLASH_NO_STORAGE = -7
};

#define FLASH_MAIN_BLOCK_SIZE 0x10000UL
#define FLASH_PARAMETER_BLOCK_SIZE 0x2000UL
#define FLASH_DEFAULT_POLL_BUDGET 16000000UL

typedef struct _FlashBlock_t {
    uint32_t offset; /**< Chip-relative block start. */
    uint32_t length;
} FlashBlock_t;

typedef struct _Flash_t {
    uint32_t begin; /**< First complete authorized archive block, CPU address. */
    uint32_t end; /**< Exclusive CPU address. */
    uint32_t completed; /**< Verified bytes in the last write/erase request. */
    uint16_t deviceId; /**< Manufacturer byte followed by device byte. */
    uint8_t status; /**< Raw WSM status. */
    uint8_t waitExceeded; /**< The soft polling limit expired. */
} Flash_t;

/** @brief Decode either B3 boot orientation at a chip-relative address. */
int32_t FlashGetBlock(uint16_t deviceId, uint32_t offset, FlashBlock_t* block);
/** @brief Identify hardware and open an authorized archive partition.
 * No production partition policy is configured yet: returns FLASH_NO_STORAGE
 * on supported hardware. The research build uses virtual storage in MAME.
 * Changes no array contents. Never infers ownership from an erased 0xFF area.
 */
int32_t FlashOpen(Flash_t* flash);
/** @brief Read array bytes into ordinary RAM. */
int32_t FlashRead(const Flash_t* flash, uint32_t address, void* data, uint32_t length);
/** @brief Preflight 1-to-0 transitions, program from RAM, and verify each byte.
 * Never implicitly erases. Partial changes may remain on failure; completed
 * counts verified bytes. Source must be RAM in 0x400..0x3FFFF, outside the
 * active call's stack frame and worker instructions.
 */
int32_t FlashProgram(Flash_t* flash, uint32_t address, const void* data,
                     uint32_t length, uint32_t pollBudget);
/** @brief Erase/verify one full reserved 64-KiB block at its exact start. */
int32_t FlashEraseBlock(Flash_t* flash, uint32_t address, uint32_t pollBudget);

/* The self-contained worker is copied into a temporary stack buffer for each
 * command operation, using the standard linker and CRT. It masks CPU interrupt
 * levels 1..6, clears CSA write protection,
 * and restores the original CSA and SR after Read Array. NMI, other exceptions,
 * watchdog reset, and concurrent flash users are not supported while busy.
 * pollBudget is a SOFT limit, not milliseconds. After expiry, the worker still
 * waits for ready before returning FLASH_WAIT_EXCEEDED: a hung chip can stall
 * indefinitely. Returning to busy flash would be unsafe. No VPP switch,
 * watchdog, suspend/resume, power-off hook, or automatic backup is implemented.
 */
#endif
