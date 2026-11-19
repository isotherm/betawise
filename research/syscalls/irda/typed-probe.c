#include "os3k.h"

#ifndef PROBE_DATA
#error PROBE_DATA is required
#endif
#define OUT ((volatile uint32_t*)PROBE_DATA)
#define CHECK(n, test) do { if (!(test)) { OUT[0] = (n); return; } } while (0)
_Static_assert(sizeof(AppletResource_t) == 6, "resource header layout");
_Static_assert(sizeof(IrdaDevice_t) == 6, "discovery slot layout");
_Static_assert(sizeof(IrdaStatus_e) == 1, "byte status ABI");
_Static_assert(sizeof(IrdaChannel_e) == 1, "byte channel ABI");

void receive_probe(const uint8_t* bytes, uint8_t length) {
    OUT[0x140] = length;
    OUT[0x141] = bytes[0];
    OUT[0x142] = bytes[length - 1];
    OUT[0x143]++;
}
void status_probe(IrdaNotification_e status) {
    OUT[0x144] = status;
    OUT[0x145]++;
}
void typed_probe(void) {
    char name[32];
    const void* payload = 0;
    const AppletResource_t* read;
    AppletResource_t* write;
    uint16_t type = 0, id = 0, length = 0;
    uint16_t buffer[24];
    uint32_t marker, address, pc;
    OUT[0] = 0;
    FileSetPasswordProtection(1);
    CHECK(1, FileGetPasswordProtection() == 1);
    CHECK(2, FileCheckMasterPassword("secret"));
    CHECK(3, !FileCheckMasterPassword("wrong"));
    CHECK(4, FileGetUserName(1, name) == 1 && name[0] == 'F');
    CHECK(5, SystemHandleEvents((SystemEventFlags_e)0, (KeyMod_e)KEY_NONE) == 0);
    AppletClearDisableMask(2);
    AppletDisable(2);
    CHECK(6, AppletGetDisableMask(2) == 1);
    AppletAddContextLock(2);
    CHECK(7, AppletGetDisableMask(2) == 3);
    AppletRemoveContextLock(2);
    CHECK(8, AppletGetDisableMask(2) == 1);
    CHECK(9, AppletGetResources() == (const AppletResource_t*)(PROBE_DATA + 0x1120));
    *(volatile uint32_t*)(PROBE_DATA + 0x100c) = 0;
    CHECK(20, AppletGetResources() == 0);
    *(volatile uint32_t*)(PROBE_DATA + 0x100c) = 0x120;
    write = (AppletResource_t*)buffer;
    AppletWriteResource(&write, 0x1234, 7, 3, "abc");
    CHECK(10, (uintptr_t)write == (uintptr_t)buffer + 10);
    AppletWriteResource(&write, 0, 0, 0, 0);
    read = (const AppletResource_t*)buffer;
    AppletReadResource(&read, &type, &id, &length, &payload);
    CHECK(11, type == 0x1234 && id == 7 && length == 3 && ((const char*)payload)[2] == 'c');
    read = (const AppletResource_t*)buffer;
    CHECK(12, AppletFindResource(&read, 0x1234, 7, &length, &payload) == 0);
    CHECK(13, AppletFindResource(&read, 0x1234, 99, 0, 0) == -1);
    CHECK(14, IrdaGetMaxPayload() == 58);
    CHECK(15, IrdaConfigureService("Test", "Value", receive_probe, status_probe) == IRDA_OK);
    CHECK(16, IrdaOpenChannel((IrdaChannel_e)5, 0) == IRDA_INVALID_CHANNEL);
    CHECK(17, IrdaCloseChannel((IrdaChannel_e)5) == IRDA_INVALID_CHANNEL);
    CHECK(18, IrdaSend(0, (IrdaChannel_e)5, 0, 0) == IRDA_INVALID_CHANNEL);
    GetExceptionInfo(&marker, &address, &pc);
    CHECK(19, marker == 0xdeadbe01 && address == 0x11223344 && pc == 0x55667788);
    OUT[0] = 0x70617373;
}
