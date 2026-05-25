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
                is_long = true;
                continue;
            case 'h':
                is_char = true;
                continue;
            case 'x': {
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
                long num;
                if (is_long) {
                    num = va_arg(vl, long);
                } else {
                    num = va_arg(vl, int);
                    if (is_char) {
                        num = (short)num;
                    }
                }

                if (num < 0) {
                    kputc('-');
                    num = -num;
                }

                char buffer[32];
                int pos = 0;
                do {
                    buffer[pos++] = '0' + (num % 10);
                    num /= 10;
                } while (num > 0 && pos < (int)sizeof(buffer));

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
            case 'w':
                unsigned int n = va_arg(vl, unsigned int);
                for (int i = 28; i >= 0; i -= 4) {
                    unsigned int d = (n >> i) & 0xF;
                    kputc(d < 10 ? '0' + d : 'a' + d - 10);
                }
                break;
            }

            is_format = false;
            is_long   = false;
            is_char   = false;
        } else if (c == '%') {
            is_format = true;
        } else {
            kputc(c);
        }
    }

    va_end(vl);
}

