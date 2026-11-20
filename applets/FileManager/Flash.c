#include "os3k.h"
#include "Flash.h"

/* Shared request offsets are documented in FlashWorker.S. */
typedef struct _FlashRequest_t {
    uint32_t operation, address;
    const uint8_t* source;
    uint32_t length, pollBudget, completed;
    uint16_t deviceId;
    uint8_t status, waitExceeded;
} FlashRequest_t;

_Static_assert(sizeof(FlashRequest_t) == 28, "Flash worker request ABI");
_Static_assert(offsetof(FlashRequest_t, completed) == 20, "Flash worker offsets");
extern const uint8_t _FlashWorker[], _FlashWorkerEnd[];

static bool FlashHardwareSupported(void) {
    uint32_t level = 0;
    if(!CallSysInt(0, SYS_INT_GET_HW_LEVEL, &level) || level != 1) return false;
    /* EZ328 group A base 0200 -> 400000; CSA enabled, 1 MiB, 8-bit.
     * Preserve the board's existing wait states and FLASH timing bit. */
    return *(volatile uint16_t*)0xFFFFF100 == 0x0200 &&
           (*(volatile uint16_t*)0xFFFFF110 & 0x008F) == 0x0007;
}

static bool FlashStorageBounds(uint32_t* begin, uint32_t* end) {
#ifdef FILE_MANAGER_FLASH_PROBE
    /* Virtual storage supplied by the scoped MAME fixture, not a reservation
     * on real hardware. This build is used only by the armed research probe. */
    *begin = 0x410000;
    *end = 0x420000;
    return true;
#else
    /* No firmware-enforced archive partition is configured yet. Never claim
     * ownership based on unused-looking flash or on File Manager placement. */
    *begin = *end = 0;
    return false;
#endif
}

static bool FlashValid(const Flash_t* flash) {
    uint32_t begin, end;
    return flash && (flash->deviceId == 0x89D2 || flash->deviceId == 0x89D3) &&
           FlashStorageBounds(&begin, &end) && flash->begin == begin && flash->end == end;
}

static bool FlashRange(const Flash_t* flash, uint32_t address, uint32_t length) {
    return FlashValid(flash) && address >= flash->begin && address <= flash->end &&
           length <= flash->end - address;
}

static bool FlashRamRange(const void* data, uint32_t length) {
    uint32_t address = (uint32_t)data;
    return address >= 0x400 && address <= 0x40000 && length <= 0x40000 - address;
}

static int32_t FlashRun(Flash_t* flash, FlashRequest_t* request) {
    /* Ordinary stack RAM; no special ELF section, CRT copy or permanent RAM
     * allocation. The blob contains only internal PC-relative branches. */
    volatile uint16_t worker[256] __attribute__((aligned(2)));
    uint32_t size = (uintptr_t)_FlashWorkerEnd - (uintptr_t)_FlashWorker;
    uint32_t i;
    int32_t result;
    flash->completed = 0;
    flash->status = flash->waitExceeded = 0;
    if(!size || size > sizeof worker || (size & 1)) return FLASH_INVALID_RANGE;
    /* Volatile stores are required: GCC does not model execution through a
     * data-buffer function pointer as a read of that buffer's initialized bytes. */
    for(i = 0; i < size; ++i) ((volatile uint8_t*)worker)[i] = _FlashWorker[i];
    result = ((int32_t (*)(FlashRequest_t*))worker)(request);
    flash->completed = request->completed;
    flash->status = request->status;
    flash->waitExceeded = request->waitExceeded;
    return result;
}

int32_t FlashGetBlock(uint16_t deviceId, uint32_t offset, FlashBlock_t* block) {
    bool parameter;
    if(!block || offset >= 0x100000 || (deviceId != 0x89D2 && deviceId != 0x89D3)) {
        return FLASH_UNSUPPORTED;
    }
    parameter = deviceId == 0x89D3 ? offset < 0x10000 : offset >= 0xF0000;
    block->length = parameter ? FLASH_PARAMETER_BLOCK_SIZE : FLASH_MAIN_BLOCK_SIZE;
    block->offset = offset & ~(block->length - 1);
    return FLASH_OK;
}

int32_t FlashOpen(Flash_t* flash) {
    FlashRequest_t request;
    if(!flash || !FlashRamRange(flash, sizeof *flash)) return FLASH_INVALID_RANGE;
    flash->begin = flash->end = flash->completed = 0;
    flash->deviceId = 0;
    flash->status = flash->waitExceeded = 0;
    if(!FlashHardwareSupported()) return FLASH_UNSUPPORTED;
    if(!FlashStorageBounds(&flash->begin, &flash->end)) return FLASH_NO_STORAGE;
    request.operation = 0;
    FlashRun(flash, &request);
    flash->deviceId = request.deviceId;
    return flash->deviceId == 0x89D2 || flash->deviceId == 0x89D3 ? FLASH_OK : FLASH_UNSUPPORTED;
}

int32_t FlashRead(const Flash_t* flash, uint32_t address, void* data, uint32_t length) {
    uint8_t* output = data;
    const volatile uint8_t* source = (const volatile uint8_t*)address;
    if(!FlashRange(flash, address, length) || !FlashRamRange(data, length)) return FLASH_INVALID_RANGE;
    while(length--) *output++ = *source++;
    return FLASH_OK;
}

int32_t FlashProgram(Flash_t* flash, uint32_t address, const void* data,
                     uint32_t length, uint32_t pollBudget) {
    FlashRequest_t request;
    const uint8_t* source = data;
    const volatile uint8_t* destination = (const volatile uint8_t*)address;
    uint32_t i;
    if(!FlashRange(flash, address, length) || !FlashRamRange(data, length) || !pollBudget) {
        return FLASH_INVALID_RANGE;
    }
    flash->completed = 0;
    flash->status = flash->waitExceeded = 0;
    if(!FlashHardwareSupported()) return FLASH_UNSUPPORTED;
    for(i = 0; i < length; ++i) {
        if((destination[i] & source[i]) != source[i]) return FLASH_NEEDS_ERASE;
    }
    request.operation = 1;
    request.address = address;
    request.source = source;
    request.length = length;
    request.pollBudget = pollBudget;
    return FlashRun(flash, &request);
}

int32_t FlashEraseBlock(Flash_t* flash, uint32_t address, uint32_t pollBudget) {
    FlashRequest_t request;
    if(!FlashRange(flash, address, FLASH_MAIN_BLOCK_SIZE) ||
       (address & (FLASH_MAIN_BLOCK_SIZE - 1)) || !pollBudget) return FLASH_INVALID_RANGE;
    if(!FlashHardwareSupported()) return FLASH_UNSUPPORTED;
    request.operation = 2;
    request.address = address;
    request.length = FLASH_MAIN_BLOCK_SIZE;
    request.pollBudget = pollBudget;
    return FlashRun(flash, &request);
}
