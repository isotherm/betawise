#include "os3k.h"
#include <stddef.h>
#define OUT ((volatile uint32_t*)0x73c00)
_Static_assert(sizeof(FontDialogContent_t)==16,"content pointers");
_Static_assert(sizeof(DialogBitmap_t)==8,"bitmap descriptor");
_Static_assert(offsetof(DialogBitmap_t,pixels)==4,"bitmap pointer");
static FontDialog_t* current;
void typed_probe(void)
{
    FontDialog_t* d=FontDialogCreate(FONT_DIALOG_LAYOUT_7);
    uint32_t errors=0;
    if(!d){OUT[0]=0xff;return;}
    current=d;
    FontDialogActivate(d,0);
    FontDialogSetProperty(d,0,FONT_DIALOG_ENABLE_INPUT,false);
    FontDialogSetTitle(d,0,"Typed dialog");
    FontDialogSetRightTitle(d,0,"right");
    FontDialogSetBody(d,0,"first\rsecond");
    FontDialogSetInput(d,0,"Prompt:");
    FontDialogContent_t content={"Header","R","one\rtwo",NULL};
    FontDialogSetContent(d,0,&content);
    FontDialogUpdate(d,0,NULL,NULL,NULL,NULL,12);
    FontDialogSetSelection(0);
    if(FontDialogGetSelection()!=0)++errors;
    FontDialogSetTitleAnimation(d,0,FONT_DIALOG_ANIMATION_THIN_CURSOR,false);
    FontDialogSetRowAnimation(d,0,FONT_DIALOG_ANIMATION_THIN_CURSOR,false,0);
    FontDialogSetInputAnimation(d,0,FONT_DIALOG_ANIMATION_BLOCK_CURSOR,false);
    FontDialogSetRowAttributes(0x10);
    FontDialogClearItemAttributes(0x10);
    FontDialogDrawRegion(d,0);
    FontDialogUpdateAnimations(0xff);
    FontDialogHandleKey(KEY_NONE);
    if(!FontDialogTextWidth("abc",3))++errors;
    FontDialogWrapText(d,0,"abc");
    char input[82];
    if(FontDialogGetInput(NULL,input)!=0)++errors;
    if(input[0]!=0)++errors; /* Only the protected Prompt: prefix was supplied. */
    const uint8_t pixels[1]={0};
    DialogBitmap_t bitmap={1,1,pixels};
    FontDialogSetBitmap(1,&bitmap,2,3);
    FontDialogSetBitmap(1,NULL,0,0);
    FontDialogDestroy(d);
    OUT[0]=errors?errors:0x70617373;
    OUT[1]=(uintptr_t)current;
}
