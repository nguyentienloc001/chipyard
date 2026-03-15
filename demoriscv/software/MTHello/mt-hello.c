// #include <stdio.h>
#include <stdint.h>
#include <riscv-pk/encoding.h>
#include "include/platform.h"
#include "kprintf.h"

#define REG32(p, i)	((p)[(i) >> 2])
#define DELAY_TIME 1000000

static volatile uint32_t *msip = (uint32_t *)(CLINT_CTRL_ADDR);

void delay() {
	for (int i = 0; i < DELAY_TIME; i = i + 1);
  for (int i = 0; i < DELAY_TIME; i = i + 1);
}

const char* get_march(uint32_t march_id) {
  switch(march_id) {
    case 1:
      return "rocket";
    case 2:
      return "sonicboom";
    default:
      return "unknown";
  }
}

int main(int hartid, char **arv) {\
  const char* march = get_march(read_csr(marchid));

  if(hartid == 0) {
    kprintf("Hello from core %c, a %s\r\n", hartid + 48, march);
    delay();
    REG32(msip, CLINT_MSIP0) = CLINT_MSIPCLR;
    REG32(msip, CLINT_MSIP1) = CLINT_MSIPEN;
  }
  if(hartid == 1) {
    kprintf("Hello from core %c, a %s\r\n", hartid + 48, march);
    delay();
    REG32(msip, CLINT_MSIP1) = CLINT_MSIPCLR;
    REG32(msip, CLINT_MSIP2) = CLINT_MSIPEN;
  }
  if(hartid == 2) {
    kprintf("Hello from core %c, a %s\r\n", hartid + 48, march);
    delay();
    REG32(msip, CLINT_MSIP2) = CLINT_MSIPCLR;
    REG32(msip, CLINT_MSIP3) = CLINT_MSIPEN;
  }
  if(hartid == 3) {
    kprintf("Hello from core %c, a %s\r\n", hartid + 48, march);
    delay();
    REG32(msip, CLINT_MSIP3) = CLINT_MSIPCLR;
    REG32(msip, CLINT_MSIP0) = CLINT_MSIPEN;
  }

  return 0;
}
