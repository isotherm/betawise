int extern_int = 100;
int extern_array[64];

/* extern_near_func — called from test_case_1_and_2 and test_case_8.
 * Sets a known value in extern_int so tests can verify it was called. */
void extern_near_func(void)
{
    extern_int = 0xBEEF;
}

/* extern_far_func — called from test_case_3.
 * Writes a different sentinel. */
#ifdef QEMU_TEST
__attribute__((section(".far.text")))
#endif
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
