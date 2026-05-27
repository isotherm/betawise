/* extern_module.c
 * Provides the extern symbols referenced by neo_test.c.
 * Compiled as a separate translation unit to ensure the compiler
 * genuinely treats them as extern (unknown distance, unknown address).
 *
 * This file is compiled with the same flags as neo_test.c.
 * Under stock -msep-data: extern data accessed via GOT (wrong for Neo).
 * Under -mneo: extern data accessed via direct disp(a5) (correct).
 */

/* Extern data — defined here, referenced in neo_test.c */
int extern_int = 100;
int extern_array[64];       /* zero-initialized, but in .data not .bss
                             * so crt0 copy covers it */

/* extern_near_func — called from test_case_1_and_2 and test_case_8.
 * Sets a known value in extern_int so tests can verify it was called. */
void extern_near_func(void)
{
    extern_int = 0xBEEF;
}

/* extern_far_func — called from test_case_3.
 * Writes a different sentinel. */
void extern_far_func(void)
{
    extern_int = 0xCAFE;
}

/* extern_binary — used as a function pointer in the vtable (case 5).
 * Writes yet another sentinel. */
void extern_binary(void)
{
    extern_int = 0xF00D;
}
