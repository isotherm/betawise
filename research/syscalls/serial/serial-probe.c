/* No applet globals: callbacks intentionally remain executable with a foreign A5.
 * Compiled with the repository ABI flags; controlled UART samples come from Lua.
 */
#include "os3k.h"
_Static_assert(sizeof(SerialConfig_e) == 4, "configuration width");
_Static_assert(sizeof(SerialReadFlags_e) == 1, "read flags width");
_Static_assert(sizeof(SerialWriteFlags_e) == 1, "write flags width");
_Static_assert(sizeof(SerialRxStatus_e) == 1, "receive status width");
_Static_assert(sizeof(SerialTxStatus_e) == 1, "transmit status width");
_Static_assert(sizeof(SerialTxEvent_e) == 1, "event mask width");
#define R ((volatile uint32_t*)PROBE_BUFFER)
#define B ((uint8_t*)(PROBE_BUFFER+0x100))
void rx_callback(SerialRxStatus_e status,uint8_t data) {
 uint32_t index=R[8]++;
 R[9]=status;R[10]=data;
 if (index<3) {R[20+2*index]=status;R[21+2*index]=data;}
 asm volatile("move.l %%a5,%0":"=m"(R[11]));
}
void tx_callback(SerialTxStatus_e status) {
 R[12]++;R[13]=status;
 asm volatile("move.l %%a5,%0":"=m"(R[29]));
}
void probe(void) {
 R[0]=SerialReset();
 R[1]=SerialConfigure(38400,SERIAL_LINK_WIRED|SERIAL_IGNORE_CTS|SERIAL_DATA_BITS_8|SERIAL_PARITY_NONE|SERIAL_STOP_BITS_1);
 R[2]=SerialSetBaudRate(57600);
 SerialGetStatus(B+20,B+21);
 SerialDisableReceiver();SerialEnableReceiver();
 R[3]=SerialRead(B,8,(uint32_t*)(R+4),SERIAL_READ_AVAILABLE|SERIAL_READ_TERMINATOR,0xd3,0);
 R[5]=SerialWrite(B,2,(uint32_t*)(R+6),SERIAL_WRITE_AVAILABLE,0,tx_callback,SERIAL_TX_EVENT_EMPTY);
 SerialSetReceiveCallback(rx_callback);
 R[14]=SerialSetBaudRate(4800);
 R[7]=0x70617373;
}

void timeout_probe(void) {
 R[15]=SerialRead(B,1,(uint32_t*)(R+16),SERIAL_READ_IDLE_TIMEOUT,0,10001);
 R[17]=SerialWrite(B,1,(uint32_t*)(R+18),SERIAL_WRITE_CTS_TIMEOUT,10001,0,SERIAL_TX_EVENT_NONE);
 R[19]=0x70617373;
}
