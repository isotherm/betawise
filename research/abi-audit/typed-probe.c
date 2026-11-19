#include "os3k.h"
typedef struct {unsigned char status; unsigned long count;} Result6;
void typed_probe(void) {
 volatile unsigned long *out=(void*)0x39000;
 void *(*search)(const void*,int,size_t)=(void*)0x37f00;
 const unsigned char *(*fixed)(const unsigned char*,const unsigned char*,unsigned char)=(void*)0x37e00;
 Result6 (*agg)(void*,char*,short,unsigned short*)=(void*)0x37f20;
 out[0]=(uintptr_t)search((void*)0x38100,'b',5);
 out[1]=(uintptr_t)fixed((void*)0x38100,(void*)0x38104,'b');
 Result6 r=agg((void*)0x38300,(void*)0x38200,10,(void*)0x38400);
 out[2]=r.status;out[3]=r.count;
 out[4]=0x70617373;
}
