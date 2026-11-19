/* Original ROM calls only; fixed probe memory avoids a dependency on libc. */
#include "os3k.h"
#define R ((volatile uint32_t*)PROBE_BUFFER)
#define B ((uint8_t*)PROBE_BUFFER)
_Static_assert(sizeof(KeyboardLayout_e)==1,"layout output is one byte");
_Static_assert(sizeof(Key_e)==1,"matrix result is one byte");
void character_probe(void) {
 for (unsigned i=0;i<256;i++) {
  B[0x400+i]=ToLowerTextByte(i);
  B[0x500+i]=IsLocalizationTextByte(i);
 }
 const uint16_t keys[]={KEY_A,KEY_A|KEY_MOD_LEFTSHIFT,KEY_A|KEY_MOD_CAPS_LOCK,
  KEY_A|KEY_MOD_LEFTSHIFT|KEY_MOD_CAPS_LOCK,KEY_A|KEY_MOD_CTRL,KEY_A|KEY_MOD_KEY_UP,KEY_NONE,
  KEY_E|KEY_MOD_ALT,KEY_A,KEY_E|KEY_MOD_ALT,KEY_A|KEY_MOD_LEFTSHIFT,
  KEY_E|KEY_MOD_ALT,KEY_E|KEY_MOD_ALT,KEY_E|KEY_MOD_ALT,KEY_B};
 for (unsigned i=0;i<sizeof(keys)/sizeof(keys[0]);i++)
  B[0x300+i]=(uint8_t)_OS3K_TranslateKeyToChar((KeyMod_e)keys[i]);
 R[0]=0x70617373;
}
void layout_probe(void) {
 for(unsigned i=0;i<256;i++)B[0x600+i]=TranslateMatrixKey(i);
 R[0]=ConfigureKeyboardLayout(false,(KeyboardLayout_e*)(B+0x101),(char*)(B+0x120));
 R[1]=ConfigureKeyboardLayout(false,0,0);
 R[2]=0x70617373;
}
void layout_prompt_probe(void) {
 R[0]=ConfigureKeyboardLayout(true,(KeyboardLayout_e*)(B+0x101),(char*)(B+0x120));
 R[2]=0x70617373;
}
void power_probe(void) {
 R[0]=IsIdleSleepEnabled();SetIdleSleepEnabled(false);R[1]=IsIdleSleepEnabled();
 SetIdleSleepEnabled(true);R[2]=IsIdleSleepEnabled();
 R[3]=IsAutoPowerOffEnabled();SetAutoPowerOffEnabled(false);R[4]=IsAutoPowerOffEnabled();
 R[5]=GetAutoPowerOffMinutes();R[6]=SetAutoPowerOffMinutes(59);R[7]=GetAutoPowerOffMinutes();
 R[8]=SetAutoPowerOffMinutes(60);R[9]=GetAutoPowerOffMinutes();
 R[10]=SetAutoPowerOffMinutes(3);R[11]=GetAutoPowerOffMinutes();
 R[12]=SetAutoPowerOffMinutes(4);R[13]=GetAutoPowerOffMinutes();
 RestoreAutoPowerOffMinutes();R[14]=GetAutoPowerOffMinutes();
 R[15]=0x70617373;
}
void power_enable_probe(void) {SetAutoPowerOffEnabled(true);R[0]=IsAutoPowerOffEnabled();R[1]=0x70617373;}
void power_minutes_probe(void) {R[0]=SetAutoPowerOffMinutes(59);R[1]=0x70617373;}
void accent_context_probe(void) {
 B[0x330]=(uint8_t)_OS3K_TranslateKeyToChar((KeyMod_e)(KEY_E|KEY_MOD_ALT));
 B[0x331]=(uint8_t)_OS3K_TranslateKeyToChar((KeyMod_e)KEY_A);
 R[0]=0x70617373;
}
