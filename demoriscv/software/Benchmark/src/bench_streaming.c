/*
 * Benchmark 7: Producer-Consumer Streaming
 *
 * Core 0 writes chunks to shared buffer, signals ready.
 * Cores 1-3 read chunks as they become available.
 * KEY METRIC: consumer spread (max/min) -- lower = better.
 * NoC advantage: independent producer->consumer paths.
 */

#include "bench_common.h"

#define CHUNK_WORDS   512    /* 2KB per chunk */
#define NUM_CHUNKS    16
#define BUF_WORDS     (CHUNK_WORDS * NUM_CHUNKS)

static volatile int32_t stream_buf[BUF_WORDS] __attribute__((aligned(64)));
static volatile int chunk_ready[NUM_CHUNKS] __attribute__((aligned(64)));
static volatile unsigned long prod_cycles = 0;
static volatile unsigned long cons_cycles[N_CORES];
static volatile int stream_phase = 0;

static unsigned long consume_all(int hart_id) {
    volatile int32_t sink = 0;
    unsigned long start = get_cycles();
    for (int c = 0; c < NUM_CHUNKS; c++) {
        while (chunk_ready[c] == 0)
            __asm__ volatile ("fence" ::: "memory");
        int base = c * CHUNK_WORDS;
        for (int i = 0; i < CHUNK_WORDS; i++)
            sink += stream_buf[base + i];
    }
    unsigned long end = get_cycles();
    if (sink == 0x7FFFFFFF) kprintf("never\r\n");  /* prevent dead-code elimination */
    return end - start;
}

void handle_msi(void);

void handle_msi(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart >= N_CORES || hart == 0) return;

    if (stream_phase == 1)
        cons_cycles[hart] = consume_all(hart);

    barrier(N_CORES);
}

int main(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart != 0) return 0;

    uart_init();
    for (int i = 0; i < BUF_WORDS; i++) stream_buf[i] = 0;
    for (int i = 0; i < NUM_CHUNKS; i++) chunk_ready[i] = 0;

    kprintf("\r\n===== Benchmark 7: Producer-Consumer Streaming =====\r\n");
    kprintf("  Producer: Core 0, Consumers: Core 1-3\r\n");
    kprintf("  %d chunks x %ld bytes = %ld bytes total\r\n",
            NUM_CHUNKS, (unsigned long)(CHUNK_WORDS * 4),
            (unsigned long)BUF_WORDS * 4UL);

    stream_phase = 1;
    __sync_synchronize();
    wake_harts(N_CORES - 1);

    unsigned long pstart = get_cycles();
    for (int c = 0; c < NUM_CHUNKS; c++) {
        int base = c * CHUNK_WORDS;
        for (int i = 0; i < CHUNK_WORDS; i++)
            stream_buf[base + i] = (int32_t)(c * CHUNK_WORDS + i);
        __asm__ volatile ("fence" ::: "memory");
        chunk_ready[c] = 1;
        __asm__ volatile ("fence" ::: "memory");
    }
    prod_cycles = get_cycles() - pstart;

    barrier(N_CORES);

    kprintf("  Producer (Core 0): %ld cycles (%ld bytes)\r\n",
            prod_cycles, (unsigned long)BUF_WORDS * 4UL);

    unsigned long cmax = cons_cycles[1], cmin = cons_cycles[1];
    for (int i = 1; i < N_CORES; i++) {
        kprintf("  Consumer (Core %d): %ld cycles\r\n", i, cons_cycles[i]);
        if (cons_cycles[i] > cmax) cmax = cons_cycles[i];
        if (cons_cycles[i] < cmin) cmin = cons_cycles[i];
    }

    kprintf("  Consumer spread: %ld.%ldx (max/min)\r\n",
            cmax / cmin, (cmax * 10 / cmin) % 10);
    kprintf("===== End Benchmark 7 =====\r\n");

    return 0;
}
