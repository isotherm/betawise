#include "os3k.h"
#include <stddef.h>
#define OUT ((volatile uint32_t*)0x73c00)
_Static_assert(sizeof(WirelessPacket_t)==6,"wire header");
_Static_assert(offsetof(WirelessPacket_t,command)==4,"wire command");
static void association(WirelessAssociationPhase_e phase, bool success, const void* info)
{ OUT[1]=phase;OUT[2]=success;OUT[3]=(uintptr_t)info; }
static uint8_t transfer(uint32_t offset, uint16_t length, WirelessTransferEvent_e event)
{ OUT[4]=offset;OUT[5]=length;OUT[6]=event;return 0; }
void typed_probe(void)
{
    uint32_t errors=0;
    if(!WirelessIsAvailable() || WirelessIsOperationPending())++errors;
    WirelessBeginAssociation(association);
    if(!WirelessIsOperationPending())++errors;
    WirelessBeginDisassociation(association); /* Busy manager must retain callback. */
    if(WirelessSendPacket(0x8001,"abc",3,WIRELESS_PACKET_FIRST|WIRELESS_PACKET_LAST))++errors;
    uint8_t buffer[8];
    if(WirelessReadTransfer(buffer)!=3 || buffer[0]!='x' || buffer[2]!='z')++errors;
    if(WirelessConfigureTransfer(WIRELESS_TRANSFER_SEND,0x5a,2049,transfer))++errors;
    if(WirelessConfigureTransfer(WIRELESS_TRANSFER_SEND,0x5a,2049,transfer)!=0)++errors;
    OUT[0]=errors?errors:0x70617373;
}
void typed_receive(void)
{
    uint8_t buffer[2068];
    OUT[0]=WirelessReceivePacket((WirelessPacket_t*)buffer)==0?0x70617373:1;
}
void typed_write(void)
{
    OUT[0]=WirelessWriteTransfer("1234567",7)==7?0x70617373:1;
}
