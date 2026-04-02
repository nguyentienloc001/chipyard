#ifndef __BENCH_COMMON_H
#define __BENCH_COMMON_H

#include <stdint.h>
#include <riscv-pk/encoding.h>
#include "platform.h"
#include "kprintf.h"

#define N_CORES 4

/* ---- UART init (enable TX) ---- */
static inline void uart_init(void) {
    REG32(uart, UART_REG_TXCTRL) = UART_TXEN;
}

/* ---- Cycle measurement ---- */
static inline unsigned long get_cycles(void) {
    unsigned long c;
    asm volatile ("fence" ::: "memory");
    c = read_csr(mcycle);
    asm volatile ("fence" ::: "memory");
    return c;
}

/* ---- Barrier (simple atomic counter) ---- */
static volatile int _barrier_cnt;
static volatile int _barrier_gen;

static void __attribute__((noinline)) barrier(int ncores)
{
    int gen = _barrier_gen;
    __sync_synchronize();
    if (__sync_add_and_fetch(&_barrier_cnt, 1) == ncores) {
        _barrier_cnt = 0;
        __sync_synchronize();
        _barrier_gen = gen + 1;
    } else {
        while (_barrier_gen == gen)
            ;
    }
    __sync_synchronize();
}

/* ---- Wake secondary harts via CLINT MSIP ---- */
static inline void wake_harts(int n) {
    for (int i = 1; i <= n; i++)
        CLINT_REG(CLINT_MSIP_BASE + i * 4) = CLINT_MSIPEN;
}

/*
 * Secondary hart entry point.
 * crt0 calls main() on hart 0 and __main() on harts 1-3.
 * Secondary harts enable MSI interrupt and wait in WFI loop.
 * When hart 0 writes CLINT MSIP, secondary harts trap into
 * handle_trap() -> handle_msi() defined in each benchmark.
 */
void __main(void) {
    uint32_t mhartid = read_csr(mhartid);
    if (mhartid >= N_CORES) while (1);

    /* Enable machine software interrupt */
    write_csr(mie, read_csr(mie) | MIP_MSIP);
    /* Enable global machine interrupt */
    write_csr(mstatus, read_csr(mstatus) | MSTATUS_MIE);

    while (1) {
        __asm__ volatile ("wfi");
    }
}

#endif /* __BENCH_COMMON_H */
