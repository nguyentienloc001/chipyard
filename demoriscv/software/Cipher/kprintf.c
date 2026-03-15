// See LICENSE.Sifive for license details.
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#include "kprintf.h"

static inline void _kputs(const char *s)
{
	char c;
	for (; (c = *s) != '\0'; s++)
		kputc(c);
}

void kputs(const char *s)
{
	_kputs(s);
	kputc('\r');
	kputc('\n');
}

void kprintf(const char *fmt, ...)
{
    va_list vl;
    bool is_format, is_long, is_char;
    char c;

    va_start(vl, fmt);
    is_format = false;
    is_long = false;
    is_char = false;

    while ((c = *fmt++) != '\0') {
        if (is_format) {
            switch (c) {
            case 'l':
                /* 'l' indicates a "long" modifier (e.g. %ld) */
                is_long = true;
                continue;
            case 'h':
                /* 'h' indicates a "short" modifier (e.g. %hd) */
                is_char = true;  /* re-using is_char to represent short */
                continue;
            case 'x': {
                /* Print in hexadecimal. Existing code. */
                unsigned long n;
                long i;
                if (is_long) {
                    n = va_arg(vl, unsigned long);
                    i = (sizeof(unsigned long) << 3) - 4;
                } else {
                    n = va_arg(vl, unsigned int);
                    i = is_char ? 4 : (sizeof(unsigned int) << 3) - 4;
                }
                for (; i >= 0; i -= 4) {
                    long d = (n >> i) & 0xF;
                    kputc(d < 10 ? '0' + d : 'a' + d - 10);
                }
                break;
            }
            case 'd': {
                /*
                 * Print signed decimal.
                 * If 'l' was present: treat as 'long'
                 * If 'h' was present: treat as 'short'
                 * otherwise treat as regular 'int'.
                 */
                long num;
                if (is_long) {
                    num = va_arg(vl, long);
                } else {
                    /* Variadic arguments of type short/char
                     * are promoted to int, but we can cast later if needed. */
                    num = va_arg(vl, int);
                    if (is_char) {
                        /* For %hd, cast down to short (optional). */
                        num = (short)num;
                    }
                }

                /* Handle negative sign if needed */
                if (num < 0) {
                    kputc('-');
                    num = -num;
                }

                /* Convert number to decimal string (in reverse) */
                char buffer[32];
                int pos = 0;
                do {
                    buffer[pos++] = '0' + (num % 10);
                    num /= 10;
                } while (num > 0 && pos < (int)sizeof(buffer));

                /* Write out the digits in correct order */
                while (pos > 0) {
                    kputc(buffer[--pos]);
                }
                break;
            }
            case 's':
                _kputs(va_arg(vl, const char *));
                break;
            case 'c':
                kputc(va_arg(vl, int));
                break;
            case 'w': /* Force 32-bit hex output */
                unsigned int n = va_arg(vl, unsigned int);
                /* We want exactly 32 bits (8 hex digits). Start at bit 28 (the top nibble). */
                for (int i = 28; i >= 0; i -= 4) {
                    unsigned int d = (n >> i) & 0xF;
                    kputc(d < 10 ? '0' + d : 'a' + d - 10);
                }
                break;
            }

            /* Reset state after handling a format specifier */
            is_format = false;
            is_long   = false;
            is_char   = false;
        } else if (c == '%') {
            /* Next character(s) will define a format specifier */
            is_format = true;
        } else {
            /* Normal character output */
            kputc(c);
        }
    }

    va_end(vl);
}

