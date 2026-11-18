#include "logo.h"
#include "globals.h"
#include <stdint.h>
#include <time.h>
#include <setjmp.h>

#ifndef LOGO_HEAP_KIB
#define LOGO_HEAP_KIB 48
#endif
enum { HEAP_SIZE = LOGO_HEAP_KIB * 1024 };
typedef struct Block {
    unsigned size;
    struct Block *next;
    int available;
} Block;
static union { Block align; unsigned char bytes[HEAP_SIZE]; } arena;
static Block *first;
static unsigned bytes_in_use;
/* Read from the emulator RAM dump when measuring this profile. */
typedef struct {
    uint32_t used, peak, rejected, slots, occupied, free_slots, retained;
} HeapSample;

volatile struct {
    uint32_t signature;
    uint32_t peak;
    uint32_t rejected;
    uint32_t capacity;
    uint32_t node_size;
    uint32_t samples;
    HeapSample sample[4];
} neo_heap_stats = {
    .signature = 0x4c4f474f,
    .capacity = HEAP_SIZE,
    .node_size = sizeof(NODE),
};

void *malloc(size_t size)
{
    if (size > HEAP_SIZE - sizeof(Block)) {
        ++neo_heap_stats.rejected;
        return NULL;
    }
    size = (size + 3) & ~3u;
    if (!size)
        size = 4;
    if (!first) {
        first = (Block *)arena.bytes;
        *first = (Block){HEAP_SIZE - sizeof(Block), NULL, 1};
    }
    for (Block *block = first; block; block = block->next) {
        if (!block->available || block->size < size)
            continue;
        if (block->size >= size + sizeof(Block) + 4) {
            Block *rest = (Block *)((unsigned char *)(block + 1) + size);
            *rest = (Block){block->size - size - sizeof(Block), block->next, 1};
            block->next = rest;
            block->size = size;
        }
        block->available = 0;
        bytes_in_use += block->size + sizeof(Block);
        if (bytes_in_use > neo_heap_stats.peak)
            neo_heap_stats.peak = bytes_in_use;
        return block + 1;
    }
    ++neo_heap_stats.rejected;
    return NULL;
}

void free(void *pointer)
{
    if (!pointer)
        return;
    Block *released = (Block *)pointer - 1;
    bytes_in_use -= released->size + sizeof(Block);
    released->available = 1;
    for (Block *block = first; block && block->next;) {
        Block *next = block->next;
        if (block->available && next->available) {
            block->size += sizeof(Block) + next->size;
            block->next = next->next;
        } else {
            block = next;
        }
    }
}

void *realloc(void *pointer, size_t size)
{
    if (!pointer)
        return malloc(size);
    if (!size) {
        free(pointer);
        return NULL;
    }
    size_t old_size = ((Block *)pointer - 1)->size;
    if (old_size >= size)
        return pointer;
    void *new_pointer = malloc(size);
    if (new_pointer) {
        memcpy(new_pointer, pointer, old_size);
        free(pointer);
    }
    return new_pointer;
}

extern void neo_output(int);
extern void neo_clear(void);
char *getenv(const char *name) { (void)name; return NULL; }
time_t time(time_t *out) { if (out) *out = 0; return 0; }
FILE *fopen(const char *path, const char *mode)
{
    (void)path;
    (void)mode;
    return NULL;
}
int fclose(FILE *stream) { (void)stream; return EOF; }
int feof(FILE *stream) { (void)stream; return 1; }
void clearerr(FILE *stream) { (void)stream; }
int getc(FILE *stream) { (void)stream; return EOF; }
char *neo_fgets(char *text, int size, FILE *stream)
{
    (void)text;
    (void)size;
    (void)stream;
    return NULL;
}

int chdir(const char *path) { (void)path; return -1; }
int unlink(const char *path) { (void)path; return -1; }
int getpid(void) { return 0; }
int fflush(FILE *stream) { (void)stream; return 0; }
int neo_fputc(int ch, FILE *stream) { (void)stream; neo_output(ch); return ch; }
int neo_putc(int ch, FILE *stream) { return neo_fputc(ch, stream); }
int neo_fputs(const char *text, FILE *stream)
{
    while (*text)
        neo_fputc(*text++, stream);
    return 0;
}

double atof(const char *text)
{
    int negative = *text == '-';
    if (*text == '-' || *text == '+')
        ++text;
    double value = 0;
    while (*text >= '0' && *text <= '9')
        value = 10 * value + (*text++ - '0');
    if (*text == '.') {
        double place = 0.1;
        for (++text; *text >= '0' && *text <= '9'; ++text) {
            value += (*text - '0') * place;
            place *= 0.1;
        }
    }
    if (*text == 'e' || *text == 'E') {
        ++text;
        int exponent_negative = *text == '-';
        if (*text == '-' || *text == '+')
            ++text;
        int power = 0;
        while (*text >= '0' && *text <= '9' && power < 308)
            power = power * 10 + (*text++ - '0');
        if (power > 308)
            power = 308;
        if (exponent_negative)
            power = -power;
        while (power > 0) { value *= 10; --power; }
        while (power < 0) { value *= 0.1; ++power; }
    }
    return negative ? -value : value;
}

typedef void (*SignalHandler)(int);
SignalHandler signal(int number, SignalHandler handler)
{
    (void)number;
    return handler;
}

static jmp_buf session;
jmp_buf iblk_buf; /* Upstream reader state, normally defined by main.c. */
static int failed;
void exit(int status)
{
    failed = 1;
    longjmp(session, status ? status : 1);
}

void charmode_on(void) {}
void charmode_off(void) {}
void term_init(void) {}

FILE *stdin;
FILE *stdout = (FILE *)1;
FILE *stderr = (FILE *)2;
NODE **bottom_stack;
NODE *command_line;
NODE *current_line;
int interactive;
int x_coord, y_coord, x_max = 22, y_max = 1;
void delayed_int(void) {}

static unsigned steps;
int neo_tick(void)
{
    if (!NOT_THROWING)
        return 0;
    if (steps) {
        --steps;
        return 1;
    }
    err_logo(STOP_ERROR, NIL);
    return 0;
}

extern NODE *free_list;
extern long mem_nodes;
#ifdef LOGO_PROFILE_GC
extern void do_gc(BOOLEAN full);
#endif

static void sample_heap(NODE **anchor)
{
    unsigned index = neo_heap_stats.samples;
    if (index >= 4)
        return;
    HeapSample sample = {
        .used = bytes_in_use,
        .peak = neo_heap_stats.peak,
        .rejected = neo_heap_stats.rejected,
        .occupied = mem_nodes,
    };
    for (struct segment *segment = segment_list; segment; segment = segment->next)
        sample.slots += segment->size;
    for (NODE *node = free_list; node; node = node->next)
        ++sample.free_slots;
#ifdef LOGO_PROFILE_GC
    /* Anchor at this frame so the completed command's parser temporaries
       are out of scope when the conservative collector scans the stack. */
    bottom_stack = anchor;
    do_gc(TRUE);
    sample.retained = mem_nodes;
#else
    (void)anchor;
#endif
    neo_heap_stats.sample[index] = sample;
    neo_heap_stats.samples = index + 1;
}

void neo_eval(char *text)
{
    static int initialized;
    if (failed || setjmp(session)) {
        neo_fputs("Logo runtime stopped", stdout);
        return;
    }
    NODE *stack_anchor = NIL;
    bottom_stack = &stack_anchor;
    if (!initialized) {
        if (!addseg())
            exit(1);
        init();
        math_init();
        initialized = 1;
        sample_heap(&stack_anchor);
    }
    check_reserve_tank();
    steps = 1200;
    current_line = make_static_strnode(text);
    NODE *program = parser(current_line, TRUE);
    if (program != NIL && NOT_THROWING)
        eval_driver(program);
    if (stopping_flag == THROWING)
        err_print(NULL);
    stopping_flag = RUN;
    current_line = NIL;
    sample_heap(&stack_anchor);
    bottom_stack = NULL;
}
