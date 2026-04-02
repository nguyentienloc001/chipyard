/*
 * Benchmark 2: Memory Bandwidth
 *
 * Measures memory throughput (bytes/cycle) under different scenarios:
 *   Phase A: Single core sequential read (64KB)
 *   Phase B: 4 cores, each reads private 16KB region
 *   Phase C: 4 cores, all read same 64KB region
 *
 * Measurement via mcycle CSR.
 */

#include "bench_common.h"

#define BUF_SIZE_WORDS  16384   /* 64KB = 16384 x 4 bytes */
#define PER_CORE_WORDS  4096    /* 16KB per core */

static volatile int32_t shared_buf[BUF_SIZE_WORDS] __attribute__((aligned(64)));
static volatile int32_t private_buf[N_CORES][PER_CORE_WORDS] __attribute__((aligned(64)));

static volatile int bw_phase = 0;
static volatile unsigned long bw_cycles[N_CORES];

/* Sequential read - return total cycles */
static unsigned long seq_read(volatile int32_t *buf, int count) {
    volatile int32_t sink = 0;
    unsigned long start = get_cycles();
    for (int i = 0; i < count; i++)
        sink += buf[i];
    unsigned long end = get_cycles();
    /* Prevent optimization */
    if (sink == 0x7FFFFFFF) kprintf("never\r\n");
    return end - start;
}

/* Sequential write - return total cycles */
static unsigned long seq_write(volatile int32_t *buf, int count) {
    unsigned long start = get_cycles();
    for (int i = 0; i < count; i++)
        buf[i] = (int32_t)i;
    unsigned long end = get_cycles();
    return end - start;
}

void handle_msi(void);

void handle_msi(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart >= N_CORES) return;

    if (bw_phase == 1) {
        /* Phase B: private region */
        bw_cycles[hart] = seq_read(private_buf[hart], PER_CORE_WORDS);
    } else if (bw_phase == 2) {
        /* Phase C: shared region */
        bw_cycles[hart] = seq_read(shared_buf, BUF_SIZE_WORDS);
    }
    barrier(N_CORES);
}

int main(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart != 0) return 0;

    uart_init();

    /* Init buffers */
    for (int i = 0; i < BUF_SIZE_WORDS; i++)
        shared_buf[i] = (int32_t)i;
    for (int c = 0; c < N_CORES; c++)
        for (int i = 0; i < PER_CORE_WORDS; i++)
            private_buf[c][i] = (int32_t)(c * PER_CORE_WORDS + i);

    kprintf("\r\n===== Benchmark 2: Memory Bandwidth =====\r\n");

    /* Phase A: Single core read 64KB */
    unsigned long cyc_a_rd = seq_read(shared_buf, BUF_SIZE_WORDS);
    unsigned long cyc_a_wr = seq_write(shared_buf, BUF_SIZE_WORDS);
    kprintf("Phase A (1 core, 64KB):\r\n");
    kprintf("  Read:  %ld cycles (%ld bytes)\r\n", cyc_a_rd, (unsigned long)(BUF_SIZE_WORDS * 4));
    kprintf("  Write: %ld cycles (%ld bytes)\r\n", cyc_a_wr, (unsigned long)(BUF_SIZE_WORDS * 4));

    /* Phase B: 4 cores, private 16KB each */
    bw_phase = 1;
    wake_harts(N_CORES - 1);
    bw_cycles[0] = seq_read(private_buf[0], PER_CORE_WORDS);
    barrier(N_CORES);

    unsigned long max_cyc_b = 0;
    kprintf("Phase B (4 cores, private 16KB each):\r\n");
    for (int i = 0; i < N_CORES; i++) {
        kprintf("  Core %d: %ld cycles (%ld bytes)\r\n", i, bw_cycles[i], (unsigned long)(PER_CORE_WORDS * 4));
        if (bw_cycles[i] > max_cyc_b) max_cyc_b = bw_cycles[i];
    }
    unsigned long agg_bytes_b = (unsigned long)(N_CORES * PER_CORE_WORDS * 4);
    kprintf("  Aggregate: %ld bytes in %ld cycles\r\n", agg_bytes_b, max_cyc_b);

    /* Phase C: 4 cores, shared 64KB */
    bw_phase = 2;
    wake_harts(N_CORES - 1);
    bw_cycles[0] = seq_read(shared_buf, BUF_SIZE_WORDS);
    barrier(N_CORES);

    unsigned long max_cyc_c = 0;
    kprintf("Phase C (4 cores, shared 64KB):\r\n");
    for (int i = 0; i < N_CORES; i++) {
        kprintf("  Core %d: %ld cycles\r\n", i, bw_cycles[i]);
        if (bw_cycles[i] > max_cyc_c) max_cyc_c = bw_cycles[i];
    }

    kprintf("===== End Benchmark 2 =====\r\n");
    return 0;
}
