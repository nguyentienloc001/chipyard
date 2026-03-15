// See LICENSE.Sifive for license details.
#include <platform.h>
#include <smp.h>
#include "common.h"

    .equ  SCRATCH_PAGE_SIZE,  0xffff000
    .equ  STACK_SIZE,         0x800   // 2kB each hart
    .equ  STACK_SHIFT,        11
    .equ  SCRATCH_TOP,        (PAYLOAD_DEST + SCRATCH_PAGE_SIZE)
    .section .text.init
    .option  norvc
    .globl   _prog_start
_prog_start:
    csrr    s0, mhartid
    beqz    s0, _setup
    li      t0, 0x8
    csrw    mie, t0
    wfi                             // sleep if is not hart 0
    j       _entry
_setup:                             // hart 0 setup only
    li      t0, 1
    li      t1, (UART_CTRL_ADDR + UART_REG_TXCTRL)
    sw      t0, 0(t1)
_entry:
    csrw    mie, zero               // ensure no new interrupt while harts set up their stacks and move to main()
    csrr    a0, mhartid             // a0 is the first function argument of main()
    addi    a1, a0, 1               // sp = SCRATCH_TOP - (hartid+1)*STACK_SIZE + (STACK_SIZE-16)
    slli    a1, a1, STACK_SHIFT
    li      t0, SCRATCH_TOP
    sub     sp, t0, a1
    addi    sp, sp, STACK_SIZE-16   // 16-byte alignment gap
    call    main
_loopback:
    li      t0, 0x8
    csrw    mie, t0
    wfi                             // all harts sleep when main() finish
    j       _loopback
