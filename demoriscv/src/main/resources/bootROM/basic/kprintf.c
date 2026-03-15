// See LICENSE.Sifive for license details.
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
