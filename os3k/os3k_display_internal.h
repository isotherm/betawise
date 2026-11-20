#ifndef _OS3K_DISPLAY_INTERNAL_H_
#define _OS3K_DISPLAY_INTERNAL_H_

#include "os3k.h"

bool _OS3K_CallSysInt(uint32_t unused_zero, SysInt_e info, void* data);

void _DisplayProcessMessage(Message_e message, uint32_t* status);
bool _DisplayQuery(SysInt_e info, void* output);
int _DisplayPutChar(int c);
void _DisplayGetSize(uint8_t* rows, uint8_t* cols);
void _DisplayEraseCells(uint8_t row, uint8_t first, uint8_t last);
void _ConsoleReset(void);
int _ConsolePutChar(int c);
int _ConsoleGetChar(void);

#endif
