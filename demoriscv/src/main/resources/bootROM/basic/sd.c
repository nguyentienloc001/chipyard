// See LICENSE.Sifive for license details.
#include <stdint.h>

#include <platform.h>

#include "common.h"

#define DEBUG
#include "kprintf.h"

int main(int mhartid, char** dump)
{
	REG32(uart, UART_REG_TXCTRL) = UART_TXEN;

	kputs("Hello world!!!\r\n");

	__asm__ __volatile__ ("fence.i" : : : "memory");

	while(1);

	return 0;
}
