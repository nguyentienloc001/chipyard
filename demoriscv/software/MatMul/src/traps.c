#include <stdint.h>
#include <riscv-pk/encoding.h>
#include "platform.h"
#include "kprintf.h"

#define SOFT_INTERRUPT_TRAP 0x8000000000000003 // software interrupt trap

void _exit(int) __attribute__ ((noreturn));
void handle_msi();

uintptr_t handle_trap(uintptr_t epc, uintptr_t cause, uintptr_t tval, uintptr_t regs[32]) {
  if (cause == SOFT_INTERRUPT_TRAP) {
    uint32_t mhartid = read_csr(mhartid);

    // clear the interrupt
    CLINT_REG(CLINT_MSIP_BASE + mhartid * 4) = CLINT_MSIPCLR;
    // execute the interrupt handler
    handle_msi();

    return epc;
  }

  int code = cause & ((1UL << ((sizeof(int)<<3)-1)) - 1);
  code = ((intptr_t)cause < 0) ? -code : code;
  kprintf("Hart %d got trap: cause=%lx, epc=%lx\r\n",
          read_csr(mhartid), cause, epc);

  // Default behavior for unhandled traps
  _exit(code);
  __builtin_unreachable(); // _exit doesn't return
}