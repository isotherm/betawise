/* Host-side parser/stream contracts; hardware integration is tested separately. */
#include "os3k_display_internal.h"
#include <assert.h>

static char cells[4][40];
static uint8_t row = 1, col = 1;
static CursorMode_e visibility;
static KeyMod_e key_queue[16];
static unsigned key_read, key_count, raw_calls;
void _DisplayGetSize(uint8_t *rows, uint8_t *cols) { *rows = 4; *cols = 40; }
void GetCursorPos(uint8_t *r, uint8_t *c) { *r = row; *c = col; }
void GetCursorMode(CursorMode_e *v) { *v = visibility; }
void SetCursor(uint8_t r, uint8_t c, CursorMode_e v) { assert(r>=1 && r<=4 && c>=1 && c<=40); row=r;col=c;visibility=v; }
void _DisplayEraseCells(uint8_t r, uint8_t a, uint8_t b) {
    assert(r>=1 && r<=4 && a>=1 && a<=b && b<=40);
    for(unsigned x=a;x<=b;++x) cells[r-1][x-1]=' ';
}
int _DisplayPutChar(int c) {
    if(c=='\r') col=1;
    else if(c=='\n') { if(row<4) ++row; col=1; }
    else if(c=='\b') { if(col>1) --col; }
    else if(c>=32) { cells[row-1][col-1]=c;if(col<40)++col; }
    return c;
}
bool IsKeyReady(void) { return key_read<key_count; }
void ScanKeyboard(void) { assert(0 && "unexpected blocking keyboard read"); }
KeyMod_e GetKey(bool special) { (void)special; assert(key_read<key_count); return key_queue[key_read++]; }
char _OS3K_TranslateKeyToChar(KeyMod_e k) { return (unsigned)k==KEY_A?'a':(unsigned)k==KEY_SPACE?' ':0; }
int32_t SystemHandleEvents(SystemEventFlags_e e, KeyMod_e k) { (void)e;(void)k;return 0; }
int _OS3K_fgetc(FILE *s) { (void)s;return -1; }
int _OS3K_fputc(int c, FILE *s) { (void)s;++raw_calls;return c; }
void _OS3K_FormatInt32(int32_t v,uint32_t radix,char *b) { (void)v;(void)radix;(void)b; }
static void equal(const char *a,const char *b) { while(*a || *b) assert(*a++==*b++); }
static void queue(KeyMod_e k) { key_queue[key_count++]=k; }
int main(void) {
    _ConsoleReset();
    fputs("\033[2J\033[2;",stdout);fputs("5Hhello",stdout);
    assert(row==2 && col==10 && cells[1][4]=='h' && cells[0][0]==' ');
    fputs("\033[6n",stdout);
    char reply[16]; unsigned n=0;int c;
    while((c=ConsoleReadPending())>=0) reply[n++]=c;
    reply[n]=0;
    equal(reply,"\033[2;10R");
    fputs("\033[999D\033[999A",stdout);assert(row==1 && col==1);
    fputs("\033[999B\033[999C",stdout);assert(row==4 && col==40);
    fputs("\033[Habcde\033[1;3H\033[K",stdout);
    assert(cells[0][0]=='a' && cells[0][1]=='b' && cells[0][2]==' ' && col==3);
    fputs("\033[1K",stdout);assert(cells[0][0]==' ' && col==3);
    fputs("\033[Habcdefgh\033[2;5Hzz\033[2;5H\033[1J",stdout);
    assert(cells[0][0]==' ' && cells[1][4]==' ' && cells[1][5]=='z');
    fputs("\033[0J",stdout);assert(cells[1][5]==' ' && row==2 && col==5);
    fputs("\033[H\tX",stdout);assert(cells[0][8]=='X');
    fputs("\033[99999999999999H\033[?25l\033[1;2;3H\033[HOK",stdout);
    assert(cells[0][0]=='O' && cells[0][1]=='K');
    putchar(27);_ConsoleReset();puts("Z");assert(cells[0][2]=='Z');
    char out[8];assert(ConsoleKeySequence((KeyMod_e)KEY_LEFT,out)==3);assert(out[0]==27 && out[1]=='[' && out[2]=='D');
    assert(ConsoleKeySequence(KEY_MOD_CTRL|KEY_A,out)==1 && out[0]==1);
    assert(ConsoleKeySequence(KEY_MOD_CTRL|KEY_SPACE,out)==1 && out[0]==0);
    assert(ConsoleKeySequence(KEY_MOD_KEY_UP|KEY_A,out)==0);
    queue((KeyMod_e)KEY_UP);assert(getc(stdin)==27 && fgetc(stdin)=='[' && getchar()=='A');
    queue(KEY_MOD_CTRL|KEY_SPACE);assert(getchar()==0);
    queue((KeyMod_e)KEY_RIGHT);assert(getchar()==27);
    fputs("\033[6n",stdout);assert(getchar()=='[' && getchar()=='C');
    while(ConsoleReadPending()>=0) {}
    queue((KeyMod_e)KEY_A);queue((KeyMod_e)KEY_ENTER);char line[8];assert(fgets(line,8,stdin)==line);equal(line,"a\r");
    line[0]='x';assert(fgets(line,1,stdin)==line && line[0]==0);assert(fgets(line,0,stdin)==NULL);
    assert(fgetc(stdout)==-1 && fgets(line,8,stdout)==NULL);
    assert(fputc('x',stderr)=='x' && raw_calls==1);
    queue((KeyMod_e)KEY_LEFT);assert(getchar()==27);_ConsoleReset();queue((KeyMod_e)KEY_A);assert(getchar()=='a');
    fputs("\033[3;8H\033[6n",stdout);assert(fgets(line,7,stdin)==line);equal(line,"\033[3;8R");
    return 0;
}
