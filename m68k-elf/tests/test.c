/* neo_test2.c - Revised and extended Neo relocation test harness
 *
 * Tests all relocation scenarios relevant to the Neo memory model:
 *
 *   Case  1: Code -> local code (PC-relative 16-bit jbsr)
 *   Case  2: Code -> extern code (GOT or 2-insn synthetic call)
 *   Case  3: Code -> far extern code (same as case 2)
 *   Case  4: const function pointer in .rodata (ROM addr, GOT entry)
 *   Case  5: volatile const vtable struct in .rodata (forced emission)
 *   Case  6: local mutable data, A5-relative 16-bit
 *   Case  7: extern mutable data, A5-relative (not GOT-indirect)
 *   Case  8: .data code pointer (single)
 *   Case  9: .data data pointer, A5 base
 *   Case 10: switch/case jump table (PC-relative, no reloc needed)
 *   Case 11: .data array of function pointers (multiple reloc entries)
 *   Case 12: .rodata array of function pointers
 *   Case 13: .data array of data pointers (data -> data)
 *   Case 14: .data array of .rodata string pointers (data -> rodata)
 *   Case 15: mixed struct in .data (ROM ptr + A5 ptr + code ptr)
 *   Case 16: pointer-to-pointer chain (fn_ptr_p -> fn_ptr -> func_a)
 *
 * Returns 0 on full pass, bitmask of failed cases otherwise.
 * Bit N set in return value = case N+1 failed.
 *
 * Ordering note:
 *   Cases 11 and 12 accumulate into results[10] via func_a/b/c.
 *   Case 12 snapshots results[10] into results[11] before case 16.
 *   Case 16 resets results[10] to 0, then calls func_a only.
 *   Final check: results[10] == 0x000000AA (case 16 only)
 *                results[11] == 0x00CCBBAA (cases 11+12 accumulated)
 *
 */

/* ================================================================
 * Extern declarations (defined in extern_module.c)
 * ================================================================ */
extern int  extern_int;
extern int  extern_array[64];
extern void extern_near_func(void);   /* sets extern_int = 0x0000BEEF */
extern void extern_far_func(void);    /* sets extern_int = 0x0000CAFE */
extern void extern_binary(void);      /* sets extern_int = 0x0000F00D */

typedef void (*func_ptr_t)(void);

/* Forward declarations */
static void local_helper(void);
static void local_print_fn(void);
static void local_unary_fn(void);
static void func_a(void);
static void func_b(void);
static void func_c(void);
static int  streq(const char *a, const char *b);

/* ================================================================
 * Result storage — .bss, zero-initialized, A5-relative
 * Index corresponds to case number minus 1 (results[0] = case 1).
 * ================================================================ */
static volatile unsigned int results[16];

/* ================================================================
 * Local mutable data — .data, A5-relative
 * ================================================================ */
static int  local_int      = 42;
static int  local_array[4] = { 1, 2, 3, 4 };
static char local_char     = 'X';
static int  val_a          = 0xA;
static int  val_b          = 0xB;

/* ================================================================
 * CASE 8: Single .data code pointer
 * Access: A5-relative load; call: indirect through loaded pointer.
 * ================================================================ */
volatile func_ptr_t data_code_ptr = (func_ptr_t)&extern_near_func;

/* ================================================================
 * CASE 9: Single .data data pointer
 * ================================================================ */
volatile int *data_data_ptr = (int *)&local_int;

/* ================================================================
 * CASE 11: .data array of function pointers
 * Each element initialised to 0; crt0 patches individually.
 * ================================================================ */
volatile func_ptr_t dispatch_table[3] = {
    (func_ptr_t)&func_a,
    (func_ptr_t)&func_b,
    (func_ptr_t)&func_c,
};

/* ================================================================
 * CASE 13: .data array of data pointers (data -> data)
 * Each element patched with A5 base.
 * ================================================================ */
volatile int *ptr_array[2] = {
    (int *)&val_a,
    (int *)&val_b,
};

/* ================================================================
 * CASE 14: .data array of .rodata string pointers (data -> rodata)
 * Each element patched with ROM base.
 * ================================================================ */
const char hello_str[] = "hello";
const char world_str[] = "world";
const char *str_ptrs[2] = {
    (const char *)&hello_str,
    (const char *)&world_str,
};

/* ================================================================
 * CASE 15: Mixed struct in .data
 * Three fields, three different relocation types:
 *   name:    data -> rodata  (ROM base patch)
 *   state:   data -> data    (A5 base patch)
 *   handler: data -> text    (ROM base patch)
 * ================================================================ */
typedef struct {
    const char *name;
    int        *state;
    func_ptr_t  handler;
} mixed_t;

volatile static mixed_t mixed = {
    .name    = (const char *)&hello_str,
    .state   = (int *)&local_int,
    .handler = (func_ptr_t)&local_helper,
};

/* ================================================================
 * CASE 16: Pointer-to-pointer chain
 * fn_ptr:   .data -> .text  (ROM base patch -> &func_a)
 * fn_ptr_p: .data -> .data  (A5 base patch  -> &fn_ptr)
 * Runtime:  (*fn_ptr_p)() double-dereferences to call func_a.
 * ================================================================ */
func_ptr_t  fn_ptr   = (func_ptr_t)&func_a;
func_ptr_t *fn_ptr_p = (func_ptr_t *)&fn_ptr;

/* ================================================================
 * CASE 4: const function pointer in .rodata
 * Absolute ROM address; accessed PC-relative.
 * After crt0/GOT processing, value is correct absolute address.
 * ================================================================ */
const func_ptr_t rodata_code_ptr = extern_near_func;

/* ================================================================
 * CASE 5: volatile const vtable struct in .rodata
 * volatile forces the compiler to emit a real memory structure
 * with genuine R_68K_32 relocation entries for each pointer field.
 * Without volatile, the optimizer may inline the calls directly.
 *
 * Expected accumulation in results[4]:
 *   local_print_fn:  results[4] |= 0xAAAA0000
 *   local_unary_fn:  results[4] |= 0x0000BBBB
 *   extern_binary:   extern_int  = 0x0000F00D
 *   test_case_5:     results[4] |= extern_int
 *
 *   0xAAAA0000 | 0x0000BBBB | 0x0000F00D
 *   = 1010 1010 1010 1010 0000 0000 0000 0000
 *   | 0000 0000 0000 0000 1011 1011 1011 1011
 *   | 0000 0000 0000 0000 1111 0000 0000 1101
 *   = 1010 1010 1010 1010 1111 1011 1011 1111
 *   = 0xAAAAFBBF
 * ================================================================ */
typedef struct {
    const char *name;
    func_ptr_t  print_fn;
    func_ptr_t  unary_fn;
    func_ptr_t  binary_fn;
} neo_type_t;

const char type_name_str[] = "testtype";

volatile const neo_type_t test_type = {
    .name      = type_name_str,
    .print_fn  = local_print_fn,
    .unary_fn  = local_unary_fn,
    .binary_fn = extern_binary,
};

/* ================================================================
 * CASE 12: .rodata array of function pointers
 * ROM-resident; each element is an absolute ROM address requiring
 * a relocation entry for ROM-base patching.
 * ================================================================ */
const func_ptr_t rodata_handlers[] = {
    func_a, func_b, func_c
};

/* ================================================================
 * Local function implementations
 * ================================================================ */
static void local_helper(void)   { results[0]  = 0x11111111; }
static void local_print_fn(void) { results[4] |= 0xAAAA0000; }
static void local_unary_fn(void) { results[4] |= 0x0000BBBB; }
static void func_a(void)         { results[10] |= 0x000000AA; }
static void func_b(void)         { results[10] |= 0x0000BB00; }
static void func_c(void)         { results[10] |= 0x00CC0000; }

static int streq(const char *a, const char *b)
{
    while (*a && *b) { if (*a++ != *b++) return 0; }
    return *a == *b;
}

/* ================================================================
 * Test functions
 * ================================================================ */

/* Case 1: local static call — jbsr local_helper:w(pc) */
void test_case_1(void)
{
    local_helper();
}

/* Case 2: extern call — GOT or 2-insn synthetic */
void test_case_2(void)
{
    extern_near_func();
    results[1] = (unsigned int)extern_int;
}

/* Case 3: far extern call — same mechanism as case 2 */
void test_case_3(void)
{
    extern_far_func();
    results[2] = (unsigned int)extern_int;
}

/* Case 4: call through const rodata function pointer */
void test_case_4(void)
{
    extern_int = 0;
    rodata_code_ptr();
    results[3] = (unsigned int)extern_int;
}

/* Case 5: call through volatile const vtable struct fields */
void test_case_5(void)
{
    extern_int = 0;
    test_type.print_fn();
    test_type.unary_fn();
    test_type.binary_fn();
    results[4] |= (unsigned int)extern_int;
}

/* Case 6: local mutable data, A5-relative */
void test_case_6(void)
{
    unsigned int ok = 0;
    if (local_int      == 42)  ok |= 0x10000000;
    if (local_array[0] == 1)   ok |= 0x01000000;
    if (local_array[3] == 4)   ok |= 0x00100000;
    if (local_char     == 'X') ok |= 0x00010000;
    local_int = 0x6666;
    if (local_int      == 0x6666) ok |= 0x00006666;
    results[5] = ok;
}

/* Case 7: extern mutable data, direct A5-relative (not GOT-indirect) */
void test_case_7(void)
{
    unsigned int ok = 0;
    extern_int       = 0x77777777;
    extern_array[0]  = 0x77000001;
    extern_array[63] = 0x77000002;
    if (extern_int        == 0x77777777) ok |= 0x77000000;
    if (extern_array[0]   == 0x77000001) ok |= 0x00000001;
    if (extern_array[63]  == 0x77000002) ok |= 0x00000002;
    results[6] = ok;
}

/* Case 8: call through .data code pointer (ROM-patched) */
void test_case_8(void)
{
    extern_int = 0;
    data_code_ptr();
    results[7] = (unsigned int)extern_int;
}

/* Case 9: write through .data data pointer (A5-patched) */
void test_case_9(void)
{
    *data_data_ptr = 0x99999999;
    results[8] = (unsigned int)local_int;
}

/* Case 10: switch/case jump table — PC-relative on m68k, no reloc */
static int switch_helper(int x)
{
    switch (x) {
        case 0:  return 0xAA;
        case 1:  return 0xBB;
        case 2:  return 0xCC;
        case 3:  return 0xDD;
        case 4:  return 0xEE;
        default: return 0xFF;
    }
}

void test_case_10(void)
{
    unsigned int ok = 0;
    if (switch_helper(0) == 0xAA) ok |= 0x00000001;
    if (switch_helper(1) == 0xBB) ok |= 0x00000002;
    if (switch_helper(2) == 0xCC) ok |= 0x00000004;
    if (switch_helper(3) == 0xDD) ok |= 0x00000008;
    if (switch_helper(4) == 0xEE) ok |= 0x00000010;
    if (switch_helper(9) == 0xFF) ok |= 0x00000020;
    results[9] = ok;
}

/* Case 11: .data array of function pointers */
void test_case_11(void)
{
    dispatch_table[0]();
    dispatch_table[1]();
    dispatch_table[2]();
    /* results[10] == 0x00CCBBAA after all three */
}

/* Case 12: .rodata array of function pointers
 * func_a/b/c OR more bits into results[10] (no change, same bits).
 * Snapshot into results[11] for independent checking. */
void test_case_12(void)
{
    rodata_handlers[0]();
    rodata_handlers[1]();
    rodata_handlers[2]();
    results[11] = results[10];   /* snapshot: 0x00CCBBAA */
}

/* Case 13: .data array of data pointers */
void test_case_13(void)
{
    *ptr_array[0] = 0xAAAA;
    *ptr_array[1] = 0xBBBB;
    unsigned int ok = 0;
    if (val_a == 0xAAAA) ok |= 0x0000AAAA;
    if (val_b == 0xBBBB) ok |= 0xBBBB0000;
    results[12] = ok;
}

/* Case 14: .data array of .rodata string pointers */
void test_case_14(void)
{
    unsigned int ok = 0;
    if (streq(str_ptrs[0], "hello")) ok |= 0x0000AAAA;
    if (streq(str_ptrs[1], "world")) ok |= 0xBBBB0000;
    results[13] = ok;
}

/* Case 15: mixed struct (.data), three relocation types */
void test_case_15(void)
{
    unsigned int ok = 0;
    results[0] = 0;                       /* clear case 1 sentinel */
    if (streq(mixed.name, "hello")) ok |= 0x00000001;
    *mixed.state = 0x15151515;
    if (local_int == 0x15151515)    ok |= 0x00000002;
    mixed.handler();                      /* -> local_helper -> results[0] */
    if (results[0] == 0x11111111)   ok |= 0x00000004;
    results[14] = ok;
}

/* Case 16: pointer-to-pointer chain */
void test_case_16(void)
{
    results[10] = 0;              /* reset accumulator */
    (*fn_ptr_p)();                /* -> fn_ptr -> func_a -> results[10] |= 0xAA */
    results[15] = results[10];
}

/* ================================================================
 * main
 * ================================================================ */
int main(void)
{
    unsigned int failures = 0;

    test_case_1();
    test_case_2();
    test_case_3();
    test_case_4();
    test_case_5();
    test_case_6();
    test_case_7();
    test_case_8();
    test_case_9();
    test_case_10();
    test_case_11();
    test_case_12();   /* snapshots results[10] into results[11] */
    test_case_13();
    test_case_14();
    test_case_15();   /* resets results[0], re-calls local_helper */
    test_case_16();   /* resets results[10], calls func_a only */

    /* ---- check results ---- */

    /* Case  1: local call wrote sentinel */
    /* Note: case 15 resets and re-triggers results[0] via local_helper */
    if (results[0]  != 0x11111111) failures |= (1u <<  0);

    /* Case  2: extern_near_func set extern_int = 0xBEEF */
    if (results[1]  != 0x0000BEEF) failures |= (1u <<  1);

    /* Case  3: extern_far_func set extern_int = 0xCAFE */
    if (results[2]  != 0x0000CAFE) failures |= (1u <<  2);

    /* Case  4: rodata_code_ptr() called extern_near_func */
    if (results[3]  != 0x0000BEEF) failures |= (1u <<  3);

    /* Case  5: vtable calls accumulated 0xAAAAFBBF */
    if (results[4]  != 0xAAAAFBBF) failures |= (1u <<  4);

    /* Case  6: local data read/write */
    if (results[5]  != 0x11116666) failures |= (1u <<  5);

    /* Case  7: extern data read/write */
    if (results[6]  != 0x77000003) failures |= (1u <<  6);

    /* Case  8: .data code pointer call */
    if (results[7]  != 0x0000BEEF) failures |= (1u <<  7);

    /* Case  9: .data data pointer write */
    if (results[8]  != 0x99999999) failures |= (1u <<  8);

    /* Case 10: switch/case — all 6 branches correct */
    if (results[9]  != 0x0000003F) failures |= (1u <<  9);

    /* Case 11/16: results[10] reset by case 16 then func_a only */
    if (results[10] != 0x000000AA) failures |= (1u << 10);

    /* Case 12: snapshot of full accumulation from cases 11+12 */
    if (results[11] != 0x00CCBBAA) failures |= (1u << 11);

    /* Case 13: data pointer array writes to val_a / val_b */
    if (results[12] != 0xBBBBAAAA) failures |= (1u << 12);

    /* Case 14: rodata string pointer array */
    if (results[13] != 0xBBBBAAAA) failures |= (1u << 13);

    /* Case 15: mixed struct, three relocation types */
    if (results[14] != 0x00000007) failures |= (1u << 14);

    /* Case 16: pointer-to-pointer chain */
    if (results[15] != 0x000000AA) failures |= (1u << 15);

    return (int)failures;
}
