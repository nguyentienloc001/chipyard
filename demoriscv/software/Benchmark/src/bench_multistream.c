/*
 * Benchmark: Multi-Directional Bandwidth
 *
 * Ring traffic: Core 0→1→2→3→0 (mỗi core gửi đến core kế tiếp)
 * Tạo 4 luồng bandwidth đồng thời, đa hướng.
 *
 * KEY METRIC: aggregate throughput (bytes/cycle) so với single-stream
 * EXPECTED: NoC throughput >> SharedBus throughput ở multi-stream
 */

#include "bench_common.h"

#define STREAM_WORDS  4096  /* 16KB per stream */

/* Receive buffers — mỗi core có 1 vùng nhận */
static volatile int32_t recv_buf[N_CORES][STREAM_WORDS]
    __attribute__((aligned(64)));

static volatile unsigned long ms_cycles[N_CORES];
static volatile int ms_phase = 0;

/* Core `me` ghi đến core `target` */
static unsigned long do_stream_to(int me, int target) {
    volatile int32_t *dst = recv_buf[target];
    unsigned long start = get_cycles();
    for (int i = 0; i < STREAM_WORDS; i++)
        dst[i] = (int32_t)(me * STREAM_WORDS + i);
    __asm__ volatile ("fence" ::: "memory");
    unsigned long end = get_cycles();
    return end - start;
}

void handle_msi(void);

void handle_msi(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart >= N_CORES) return;

    int target = (hart + 1) % N_CORES;

    if (ms_phase == 1) {
        /* Single-stream baseline: chỉ Core 0 gửi */
        /* Các core khác chờ */
        barrier(N_CORES);
    }
    else if (ms_phase == 2) {
        /* Multi-stream: tất cả core gửi đồng thời */
        /* Warmup */
        do_stream_to(hart, target);
        barrier(N_CORES);
        /* Timed */
        ms_cycles[hart] = do_stream_to(hart, target);
        barrier(N_CORES);
    }
}

int main(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart != 0) return 0;

    uart_init();
    unsigned long bytes_per_stream = (unsigned long)(STREAM_WORDS * 4);

    kprintf("\r\n===== Benchmark: Multi-Directional Bandwidth =====\r\n");
    kprintf("  Pattern: Core 0->1->2->3->0 (ring)\r\n");
    kprintf("  %ld bytes per stream\r\n", bytes_per_stream);

    /* Phase A: Single stream (Core 0→1 only) — baseline */
    kprintf("\r\n  Phase A: Single stream (Core 0->1)\r\n");
    ms_phase = 1;
    __asm__ volatile ("fence" ::: "memory");
    wake_harts(N_CORES - 1);
    /* Warmup */
    do_stream_to(0, 1);
    unsigned long single_cycles = do_stream_to(0, 1);
    barrier(N_CORES);
    kprintf("    Core 0->1: %ld cycles (%ld bytes)\r\n", single_cycles, bytes_per_stream);

    /* Phase B: 4 concurrent streams */
    kprintf("\r\n  Phase B: 4 concurrent streams (ring)\r\n");
    ms_phase = 2;
    __asm__ volatile ("fence" ::: "memory");
    wake_harts(N_CORES - 1);
    /* Warmup */
    do_stream_to(0, 1);
    barrier(N_CORES);
    /* Timed */
    ms_cycles[0] = do_stream_to(0, 1);
    barrier(N_CORES);

    unsigned long max_cyc = 0;
    for (int i = 0; i < N_CORES; i++) {
        int tgt = (i + 1) % N_CORES;
        kprintf("    Core %d->%d: %ld cycles\r\n", i, tgt, ms_cycles[i]);
        if (ms_cycles[i] > max_cyc) max_cyc = ms_cycles[i];
    }

    unsigned long agg_bytes = bytes_per_stream * N_CORES;
    kprintf("\r\n  Single-stream: %ld bytes in %ld cycles\r\n",
            bytes_per_stream, single_cycles);
    kprintf("  Multi-stream:  %ld bytes in %ld cycles\r\n",
            agg_bytes, max_cyc);
    kprintf("  BW ratio (multi/single): %ld.%02ldx (higher=better parallel BW)\r\n",
            (agg_bytes * single_cycles) / (bytes_per_stream * max_cyc),
            ((agg_bytes * single_cycles * 100) / (bytes_per_stream * max_cyc)) % 100);

    kprintf("===== End Benchmark =====\r\n");
    return 0;
}
