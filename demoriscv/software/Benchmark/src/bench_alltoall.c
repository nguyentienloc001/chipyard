/*
 * Benchmark: All-to-All Cross-Traffic
 *
 * 4 cores, mỗi core ghi BLOCK_SIZE bytes đến vùng nhớ của 3 core còn lại.
 * Tạo 12 luồng traffic đồng thời trên sbus.
 *
 * KEY METRIC: aggregate throughput (bytes/cycle) và fairness (max/min ratio)
 * EXPECTED: SharedBus << Crossbar < NoC topologies
 */

#include "bench_common.h"

#define BLOCK_WORDS  1024   /* 4KB per block (1024 x 4 bytes) */

/* Per-core destination buffers — mỗi core sở hữu 1 row.
 * buffers[target][source] = data written by source to target.
 * Cacheline-aligned để tránh false sharing */
static volatile int32_t buffers[N_CORES][N_CORES][BLOCK_WORDS]
    __attribute__((aligned(64)));

static volatile unsigned long a2a_cycles[N_CORES];
static volatile int a2a_done[N_CORES];

/* Core `me` ghi BLOCK_WORDS đến mỗi core khác */
static unsigned long do_alltoall(int me) {
    unsigned long start = get_cycles();
    for (int target = 0; target < N_CORES; target++) {
        if (target == me) continue;
        volatile int32_t *dst = buffers[target][me];
        for (int i = 0; i < BLOCK_WORDS; i++)
            dst[i] = (int32_t)(me * BLOCK_WORDS + i);
    }
    __asm__ volatile ("fence" ::: "memory");
    unsigned long end = get_cycles();
    return end - start;
}

void handle_msi(void);

void handle_msi(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart >= N_CORES) return;

    /* Warmup */
    do_alltoall(hart);
    barrier(N_CORES);

    /* Timed run */
    a2a_cycles[hart] = do_alltoall(hart);
    barrier(N_CORES);
}

int main(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart != 0) return 0;

    uart_init();
    kprintf("\r\n===== Benchmark: All-to-All Traffic =====\r\n");
    kprintf("  %d cores, %ld bytes per transfer, %d flows\r\n",
            N_CORES, (unsigned long)(BLOCK_WORDS * 4),
            N_CORES * (N_CORES - 1));

    wake_harts(N_CORES - 1);

    /* Warmup */
    do_alltoall(0);
    barrier(N_CORES);

    /* Timed run */
    a2a_cycles[0] = do_alltoall(0);
    barrier(N_CORES);

    unsigned long max_cyc = 0, min_cyc = a2a_cycles[0];
    for (int i = 0; i < N_CORES; i++) {
        kprintf("  Core %d: %ld cycles\r\n", i, a2a_cycles[i]);
        if (a2a_cycles[i] > max_cyc) max_cyc = a2a_cycles[i];
        if (a2a_cycles[i] < min_cyc) min_cyc = a2a_cycles[i];
    }

    unsigned long total_bytes = (unsigned long)(N_CORES * (N_CORES - 1) * BLOCK_WORDS * 4);
    kprintf("  Aggregate: %ld bytes in %ld cycles\r\n", total_bytes, max_cyc);
    kprintf("  Throughput (x1000): %ld bytes/Kcycle\r\n",
            total_bytes * 1000 / max_cyc);
    kprintf("  Fairness (max/min): %ld.%02ldx\r\n",
            max_cyc / min_cyc, (max_cyc * 100 / min_cyc) % 100);

    kprintf("===== End Benchmark =====\r\n");
    return 0;
}
