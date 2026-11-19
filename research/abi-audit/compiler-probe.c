#include "os3k.h"
void *p_ret(void *p) { return p; }
extern void *p_external(void);
unsigned char p_use(void) { return *(unsigned char *)p_external(); }
extern void args(void *, unsigned char, unsigned short, unsigned long long, void *);
void args_call(void *p) { args(p,0xab,0xcdef,0x1122334455667788ULL,p); }
unsigned long long q_ret(void) { return 0x1122334455667788ULL; }
struct S4 { unsigned x; }; struct S8 { unsigned x,y; }; struct S10 { unsigned x,y; unsigned short z; };struct S12 {unsigned x,y,z;};
struct S4 s4(void) { return (struct S4){0x12345678}; }
struct S8 s8(void) { return (struct S8){1,2}; }
struct S10 s10(void) { return (struct S10){1,2,3}; }
struct S12 s12(void) { return (struct S12){1,2,3}; }
double dret(void){return 1.0;}
float fret(void){return 1.0f;}
extern _Bool b_external(void); extern unsigned char c_external(void); extern unsigned short w_external(void);
unsigned b_use(void){return b_external();} unsigned c_use(void){return c_external();} unsigned w_use(void){return w_external();}
void callback(TimerEvent_e e){asm volatile(""::"g"(e));}
unsigned sizes[]={sizeof(void*),sizeof(int),sizeof(long),sizeof(double),sizeof(long double),sizeof(KeyMod_e),sizeof(Message_e),sizeof(CursorMode_e),sizeof(BufferSearch_t),_Alignof(unsigned long long)};
struct S6 {unsigned char c; unsigned x;};
struct S6 s6(void){return (struct S6){1,2};}
extern struct S6 sx(void*);
unsigned sx_call(void *p){return sx(p).x;}
unsigned consume_callback(TimerEvent_e e){return e;}
_Static_assert(sizeof(void*)==4 && sizeof(int)==4 && sizeof(long)==4,"ILP32 required");
_Static_assert(sizeof(double)==8 && sizeof(long double)==12,"floating layout changed");
_Static_assert(sizeof(BufferSearch_t)==18 && offsetof(BufferSearch_t,case_sensitive)==16,"search layout");
_Static_assert(sizeof(FileInfo_t)==0x48 && offsetof(FileInfo_t,name)==0x34 && offsetof(FileInfo_t,file_id)==0x44,"file layout");
_Static_assert(sizeof(AppletHeader_t)==0x94 && offsetof(AppletHeader_t,entryPoint)==0x84,"applet layout");
_Static_assert(sizeof(FontHeader_t)==16 && offsetof(FontHeader_t,bitmap_data)==12,"font layout");
_Static_assert(sizeof(TextPosition_t)==2 && sizeof(FirmwareVersion_t)==4,"byte structures");
_Static_assert(sizeof(CursorMode_e)==1 && sizeof(KeyMod_e)==2 && sizeof(Message_e)==4,"enum ABI");
_Static_assert(sizeof(struct S6)==6 && offsetof(struct S6,x)==2,"six-byte result layout");
