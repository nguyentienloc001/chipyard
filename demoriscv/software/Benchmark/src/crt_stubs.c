/* Minimal stubs to avoid libc_nano (compiled without -mcmodel=medany).
   crt0 + libgloss_htif need these; we provide lightweight versions. */

extern void _exit(int code) __attribute__((noreturn));

/* No static constructors/destructors in this baremetal program */
void __libc_init_array(void) {}
void __libc_fini_array(void) {}

int atexit(void (*f)(void)) { (void)f; return 0; }

void exit(int code) { _exit(code); }

/* environ required by crtmain.S (envp arg to main) */
char *environ[] = { 0 };

/* Minimal memset / memcpy for BSS clear and TLS init */
void *memset(void *s, int c, __SIZE_TYPE__ n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

void *memcpy(void *dst, const void *src, __SIZE_TYPE__ n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s2 = (const unsigned char *)src;
    while (n--) *d++ = *s2++;
    return dst;
}
