/*
 * Benchmark 5: All-to-All Traffic
 *
 * Each core writes BLOCK_SIZE bytes to every other core's region.
 * Creates N*(N-1) simultaneous cross-traffic flows on sbus.
 *
 * KEY METRIC: aggregate throughput (bytes/cycle) and fairness (max/min).
 * Expected: SharedBus << Crossbar < Ring <= Mesh ~ Tree
 */

#include "bench_common.h"

#define BLOCK_WORDS  1024   /* 4KB per block (1024 x 4 bytes) */

/* Per-core destination buffers -- each core owns one row.
 * buffers[target][source] = data written by `source` to `target`.
 * Cacheline-aligned to avoid false sharing. */
static volatile int32_t buffers[N_CORES][N_CORES][BLOCK_WORDS]
    __attribute__((aligned(64)));

static volatile unsigned long a2a_cycles[N_CORES];
static volatile int a2a_phase = 0;

/* Core `me` writes BLOCK_WORDS to every other core's buffer */
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

    if (a2a_phase == 1) {
        a2a_cycles[hart] = do_alltoall(hart);
    }

    barrier(N_CORES);
}

int main(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart != 0) return 0;

    uart_init();

    kprintf("\r\n===== Benchmark 5: All-to-All Traffic =====\r\n");
    kprintf("  %d cores, %ld bytes per transfer\r\n",
            N_CORES, (unsigned long)(BLOCK_WORDS * 4));

    a2a_phase = 1;
    __sync_synchronize();
    wake_harts(N_CORES - 1);
    a2a_cycles[0] = do_alltoall(0);
    barrier(N_CORES);

    unsigned long max_cyc = 0;
    unsigned long min_cyc = a2a_cycles[0];
    for (int i = 0; i < N_CORES; i++) {
        kprintf("  Core %d: %ld cycles\r\n", i, a2a_cycles[i]);
        if (a2a_cycles[i] > max_cyc) max_cyc = a2a_cycles[i];
        if (a2a_cycles[i] < min_cyc) min_cyc = a2a_cycles[i];
    }

    unsigned long total_bytes = (unsigned long)N_CORES * (N_CORES - 1) * BLOCK_WORDS * 4UL;
    kprintf("  Aggregate: %ld bytes in %ld cycles\r\n", total_bytes, max_cyc);
    kprintf("  Throughput: %ld bytes/cycle (x1000)\r\n",
            total_bytes * 1000 / max_cyc);
    kprintf("  Fairness (max/min): %ld.%ldx\r\n",
            max_cyc / min_cyc, (max_cyc * 10 / min_cyc) % 10);
    kprintf("===== End Benchmark 5 =====\r\n");

    return 0;
}
