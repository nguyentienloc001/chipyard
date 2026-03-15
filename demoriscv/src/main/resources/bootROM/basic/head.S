// See LICENSE.Sifive for license details.
#include <platform.h>

  .section .sdata
  .section .text.init
  .option norvc
  .globl _prog_start
_prog_start:
  li sp, (MEMORY_MEM_ADDR + 0x7f0)
  csrr a0, mhartid // hartid for next level bootloader
  call main
  li s1, MEMORY_MEM_ADDR
  jr s1
  j _prog_start
