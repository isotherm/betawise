#include "os3k.h"
#include <stddef.h>
#ifndef PROBE_DATA
#error PROBE_DATA is required
#endif
#define OUT ((volatile uint32_t*)PROBE_DATA)
#define CHECK(n,t) do { if (!(t)) { OUT[0]=(n); return; } } while (0)
_Static_assert(sizeof(TextEditor_t)==94,"editor layout");
_Static_assert(sizeof(TextScan_t)==44,"scanner layout");
_Static_assert(sizeof(TextTransfer_t)==124,"transfer layout");
_Static_assert(sizeof(TextForm_t)==74,"form layout");
_Static_assert(sizeof(double)==8,"statistics floating output");
_Static_assert(offsetof(TextEditor_t,fonts)==0x4e,"Neo font pointers");
_Static_assert(offsetof(TextEditor_t,caret_offset)==0x24,"caret offset");
_Static_assert(offsetof(TextScan_t,conversion_text)==0x26,"scanner conversion pointer");
_Static_assert(offsetof(TextTransfer_t,position)==0x50,"transfer scanner");
_Static_assert(offsetof(TextForm_t,active_control)==0x2c,"form focus");

void typed_probe(void) {
    TextEditor_t editor;
    TextScan_t scan;
    TextTransfer_t transfer;
    uint8_t text[16];
    int32_t status;
    uint32_t chars=0,separators=0,words=0,paragraphs=0,newlines=0,sentences=0,forms=99;
    union { double value; uint32_t word[2]; } units;
    struct { uint32_t length; void* bytes; uint32_t reserved[3]; } request;
    uint32_t reply;
    uint16_t escape=0x48;
    char name[]="ProbeName";
    char insert[]="XYZ";
    OUT[0]=0;
    TextGetStatistics(1,0,UINT32_MAX,&chars,&separators,&words,&paragraphs,&newlines,&sentences,&units.value,&forms);
    CHECK(1,chars==3&&separators==0&&words==0&&paragraphs==1&&newlines==0&&sentences==0&&forms==0);
    CHECK(2,units.word[0]==0x3f580000&&units.word[1]==0);
    CHECK(3,TextEditorInit(&editor,1,TEXT_EDITOR_EDITABLE,1000,0,0,40,4)==0);
    CHECK(4,editor.file_id==1&&editor.gap_end-editor.gap_start==512&&editor.read_only==0);
    CHECK(5,TextEditorHandleMessage(&editor,MSG_IDLE,0,0,&status)==1&&status==0);
    CHECK(6,TextScanInit(&editor,&scan,TEXT_POSITION_START)==0);
    CHECK(7,TextScanRead(&scan,3,text,false,TEXT_BYTES_PLAIN,false,false,false,false,0)==3);
    CHECK(8,text[0]=='a'&&text[1]=='b'&&text[2]=='c');
    CHECK(9,TextScanInsert(&scan,insert,3,TEXT_BYTES_PLAIN)==0);
    CHECK(10,scan.offset==515); /* raw offset includes the leading 512-byte gap */
    CHECK(11,TextScanDelete(&scan,1,TEXT_BYTES_PLAIN,false,false)==0);
    CHECK(12,TextEditorSetScanPosition(&scan,false)==0);
    CHECK(13,TextEditorSavePosition(&editor)==0);
    CHECK(14,TextEditorRefresh(&editor)==0);
#ifdef PROBE_NEO
    CHECK(15,TextEditorSetFonts(&editor,0,0,0,0,false)==0);
#else
    CHECK(15,TextEditorSetFonts(&editor,0,0,0,0,false)==TEXT_ERROR_UNIMPLEMENTED);
#endif
    TextEditorTransferInit(&editor,&transfer);
    CHECK(16,transfer.file_id==1&&transfer.position.editor==&editor);
    request.length=9;request.bytes=name;
    CHECK(17,TextEditorTransferHandleMessage(&transfer,(Message_e)0x60001,&request,&reply)&&reply==17&&transfer.phase==6);
    CHECK(18,transfer.name[0]=='P'&&transfer.name[8]=='e'&&transfer.name[9]==0);
    request.bytes=&escape;
    CHECK(19,TextEditorTransferHandleMessage(&transfer,(Message_e)0x60020,&request,&reply)&&reply==8);
    CHECK(20,TextEditorClear(&editor)==0);
    OUT[0]=0x70617373;
}

void typed_form_probe(void) {
    TextForm_t form;
    TextTransfer_t transfer;
    int32_t status;
    uint32_t reply;
    OUT[0]=0;
    CHECK(21,TextFormInit(1,&form,true)==0);
    CHECK(22,form.file_id==1&&form.control_count==1&&form.active_control==0);
    CHECK(23,!TextFormHandleMessage(&form,(Message_e)0xff,0,0,&status)&&status==0);
    TextFormTransferInit(&transfer,&form);
    CHECK(24,transfer.file_id==1&&transfer.phase==0);
    CHECK(25,!TextFormTransferHandleMessage(&transfer,(Message_e)0xff,0,&reply)&&reply==0);
    CHECK(26,TextFormFind(&form,0,"HELLO"));
    CHECK(27,FileOpen(1,FILE_OPEN_KEEP_POSITION)==1);
    TextFormReset(&form);
    CHECK(28,form.control_count==1&&form.data_end-form.data_start==20);
    OUT[0]=0x70617373;
}
