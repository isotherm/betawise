/* Original firmware, explicit RAM fixtures, production m68k calling convention. */
#include "os3k.h"
#define R ((volatile uint32_t*)PROBE_BUFFER)
#define B ((uint8_t*)PROBE_BUFFER)
_Static_assert(sizeof(SendSpeed_e)==1,"Send speed byte ABI");
_Static_assert(sizeof(BatteryType_e)==4,"battery type full D0 ABI");
_Static_assert(sizeof(BatteryDiagnostic_e)==4,"diagnostic selector full slot ABI");
void dialog_probe(void) {
 DialogInit(true,1,4,40);
 R[0]=DialogAddItem((char*)(B+0x800),3,' ',101,KEY_NONE,(size_t)-1);
 R[1]=DialogAddItem((char*)(B+0x820),7,' ',102,KEY_NONE,(size_t)-1);
 R[2]=(uintptr_t)DialogGetItemText(1,B+0x100);R[3]=B[0x100];
 R[4]=(uintptr_t)DialogGetItemText(2,B+0x101);R[5]=B[0x101];
 R[6]=0x70617373;
}
void speed_probe(void) {
 R[0]=GetSendSpeed();
 for(unsigned i=1;i<=4;i++){R[2*i-1]=SetSendSpeed((SendSpeed_e)i);R[2*i]=GetSendSpeed();}
 R[9]=SetSendSpeed((SendSpeed_e)0);R[10]=GetSendSpeed();
 R[11]=SetSendSpeed((SendSpeed_e)5);R[12]=GetSendSpeed();
 R[13]=SetSendSpeed(SEND_SPEED_FASTEST_AND_DEFAULT);R[14]=GetSendSpeed();R[15]=0x70617373;
}
void battery_probe(void) {
 R[0]=GetHardwareRevision();R[1]=GetBatteryType();R[2]=GetBatteryDischargePercent();R[3]=GetBatteryMillivolts();
 for(unsigned i=0;i<=10;i++)R[4+i]=BatteryDiagnostic((BatteryDiagnostic_e)i);
 R[15]=BatteryDiagnostic((BatteryDiagnostic_e)11);
 R[16]=BatteryDiagnostic((BatteryDiagnostic_e)0x1000000);
 R[17]=BatteryDiagnostic(BATTERY_DIAG_INVALID);
 R[18]=BatteryDiagnostic(BATTERY_DIAG_STORED_CALIBRATION);R[19]=0x70617373;
}
void refresh_probe(void) {
 RefreshBatteryMeasurements();
 R[0]=BatteryDiagnostic(BATTERY_DIAG_RAW_SAMPLE);R[1]=BatteryDiagnostic(BATTERY_DIAG_NORMALIZED_SAMPLE);
 R[2]=GetBatteryType();R[3]=GetBatteryMillivolts();R[4]=0x70617373;
}
void calibrate_probe(void) {
 R[0]=BatteryDiagnostic(BATTERY_DIAG_CALIBRATE);R[1]=BatteryDiagnostic(BATTERY_DIAG_CALIBRATION);R[2]=0x70617373;
}
void display_probe(void) {DrawBatteryStatus(1);for(unsigned i=0;i<4;i++)R[1+i]=IsLocalizationTextByte(0x91+i);R[0]=0x70617373;}
void display_modal_probe(void) {DrawBatteryStatus(0);R[0]=0x70617373;}
