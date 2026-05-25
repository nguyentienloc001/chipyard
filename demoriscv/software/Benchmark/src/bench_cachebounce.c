/*
 * Benchmark: Cache-Line Bouncing (Concurrent Pairs)
 *
 * Phase A: 1 cặp (Core 0↔1) ping-pong cache line
 * Phase B: 2 cặp (Core 0↔1 + Core 2↔3) ping-pong đồng thời
 *
 * KEY METRIC: Phase B throughput / Phase A throughput
 * Ratio > 1.0 = interconnect có parallel paths (NoC advantage)
 * Ratio ≈ 1.0 = interconnect serialize (SharedBus/Crossbar limitation)
 *
 * QUAN TRỌNG: Chỉ dùng shared memory flags, KHÔNG dùng CLINT IPI
 * để tránh deadlock đã xảy ra ở CI #40.
 */

#include "bench_common.h"

#define BOUNCE_ITERS  2000

/* Shared flags cho mỗi cặp — cacheline-aligned */
static volatile int32_t flag_pair0[16] __attribute__((aligned(64))); /* Core 0↔1 */
static volatile int32_t flag_pair1[16] __attribute__((aligned(64))); /* Core 2↔3 */

static volatile unsigned long bounce_cycles[N_CORES];
static volatile int bounce_phase = 0;  /* 0=idle, 1=single pair, 2=dual pair */

/* Generic ping-pong: 2 cores trao đổi qua shared flag */
static unsigned long do_bounce(volatile int32_t *flag, int me, int partner, int iters) {
    int is_sender = (me < partner);
    unsigned long start, end;

    /* Init: sender bắt đầu với flag=0 */
    if (is_sender) flag[0] = 0;
    __asm__ volatile ("fence" ::: "memory");

    start = get_cycles();
    for (int i = 0; i < iters; i++) {
        if (is_sender) {
            flag[0] = i + 1;
            __asm__ volatile ("fence" ::: "memory");
            while (flag[0] != -(i + 1))
                __asm__ volatile ("fence" ::: "memory");
        } else {
            while (flag[0] != (i + 1))
                __asm__ volatile ("fence" ::: "memory");
            flag[0] = -(i + 1);
            __asm__ volatile ("fence" ::: "memory");
        }
    }
    end = get_cycles();

    return end - start;
}

void handle_msi(void);

void handle_msi(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart >= N_CORES) return;

    if (bounce_phase == 1) {
        /* Phase A: chỉ Core 0↔1 */
        if (hart == 1) {
            bounce_cycles[1] = do_bounce(flag_pair0, 1, 0, BOUNCE_ITERS);
        }
        /* Core 2,3 chỉ barrier */
        barrier(N_CORES);
    }
    else if (bounce_phase == 2) {
        /* Phase B: Core 0↔1 + Core 2↔3 đồng thời */
        if (hart == 1) {
            bounce_cycles[1] = do_bounce(flag_pair0, 1, 0, BOUNCE_ITERS);
        } else if (hart == 2) {
            bounce_cycles[2] = do_bounce(flag_pair1, 2, 3, BOUNCE_ITERS);
        } else if (hart == 3) {
            bounce_cycles[3] = do_bounce(flag_pair1, 3, 2, BOUNCE_ITERS);
        }
        barrier(N_CORES);
    }
}

int main(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart != 0) return 0;

    uart_init();
    kprintf("\r\n===== Benchmark: Cache-Line Bouncing =====\r\n");
    kprintf("  %d iterations per phase\r\n", BOUNCE_ITERS);

    wake_harts(N_CORES - 1);

    /* Phase A: 1 pair (Core 0↔1) */
    kprintf("\r\n  Phase A: 1 pair (Core 0<->1)\r\n");
    bounce_phase = 1;
    __asm__ volatile ("fence" ::: "memory");
    bounce_cycles[0] = do_bounce(flag_pair0, 0, 1, BOUNCE_ITERS);
    barrier(N_CORES);

    unsigned long phase_a = bounce_cycles[0];
    kprintf("    Core 0: %ld cycles\r\n", bounce_cycles[0]);
    kprintf("    Core 1: %ld cycles\r\n", bounce_cycles[1]);

    /* Phase B: 2 pairs (Core 0↔1 + Core 2↔3) */
    kprintf("\r\n  Phase B: 2 pairs (Core 0<->1 + Core 2<->3)\r\n");
    bounce_phase = 2;
    __asm__ volatile ("fence" ::: "memory");
    /* Wake harts lại cho phase mới */
    wake_harts(N_CORES - 1);
    bounce_cycles[0] = do_bounce(flag_pair0, 0, 1, BOUNCE_ITERS);
    barrier(N_CORES);

    unsigned long phase_b = bounce_cycles[0];
    for (int i = 0; i < N_CORES; i++)
        kprintf("    Core %d: %ld cycles\r\n", i, bounce_cycles[i]);

    /* Key metric: parallel efficiency */
    kprintf("\r\n  Phase A (1 pair): %ld cycles\r\n", phase_a);
    kprintf("  Phase B (2 pairs): %ld cycles\r\n", phase_b);
    kprintf("  Slowdown: %ld.%02ldx (1.00=perfect parallel, 2.00=fully serial)\r\n",
            phase_b / phase_a, (phase_b * 100 / phase_a) % 100);

    kprintf("===== End Benchmark =====\r\n");
    return 0;
}
