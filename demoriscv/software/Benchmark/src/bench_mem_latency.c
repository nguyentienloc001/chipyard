/*
 * Benchmark 1: Memory Access Latency
 *
 * Measures memory load latency under different contention scenarios:
 *   Phase A: Single core, no contention
 *   Phase B: 4 cores, each accessing private region
 *   Phase C: 4 cores, all accessing shared region
 *
 * Uses pointer-chasing to defeat prefetch and ensure serial dependency.
 * Measurement via mcycle CSR.
 */

#include "bench_common.h"

#define CHAIN_LEN   256     /* number of nodes in pointer chase chain */
#define WARMUP      100
#define ITERATIONS  1000

/* Pointer chase node - cacheline sized to avoid spatial prefetch */
struct node {
    struct node *next;
    char pad[56];  /* pad to 64 bytes (1 cacheline) */
} __attribute__((aligned(64)));

/* Per-core private arrays and a shared array */
static struct node private_chain[N_CORES][CHAIN_LEN] __attribute__((aligned(64)));
static struct node shared_chain[CHAIN_LEN] __attribute__((aligned(64)));

/* Build a simple sequential chain (deterministic, but defeats prefetch via indirection) */
static void build_chain(struct node *chain, int len) {
    for (int i = 0; i < len - 1; i++)
        chain[i].next = &chain[i + 1];
    chain[len - 1].next = &chain[0];
}

/* Chase the pointer chain and return average latency in cycles */
static unsigned long chase(struct node *head, int warmup, int iters) {
    volatile struct node *p = head;

    /* Warmup - fill cache */
    for (int i = 0; i < warmup * CHAIN_LEN; i++)
        p = p->next;

    /* Timed run */
    unsigned long start = get_cycles();
    for (int i = 0; i < iters * CHAIN_LEN; i++)
        p = p->next;
    unsigned long end = get_cycles();

    /* Prevent dead-code elimination */
    if ((uintptr_t)p == 1) kprintf("never\r\n");

    return (end - start) / (iters * CHAIN_LEN);
}

/* --- Shared state for multi-core coordination --- */
static volatile int phase = 0;
static volatile unsigned long results[N_CORES];

void handle_msi(void);

void handle_msi(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart >= N_CORES) return;

    if (phase == 1) {
        /* Phase B: private region */
        results[hart] = chase(&private_chain[hart][0], WARMUP, ITERATIONS);
    } else if (phase == 2) {
        /* Phase C: shared region */
        results[hart] = chase(&shared_chain[0], WARMUP, ITERATIONS);
    }
    barrier(N_CORES);
}

int main(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart != 0) return 0;

    uart_init();

    /* Build chains */
    for (int i = 0; i < N_CORES; i++)
        build_chain(private_chain[i], CHAIN_LEN);
    build_chain(shared_chain, CHAIN_LEN);

    kprintf("\r\n===== Benchmark 1: Memory Latency (cycles/access) =====\r\n");

    /* Phase A: Single core */
    unsigned long lat_a = chase(&private_chain[0][0], WARMUP, ITERATIONS);
    kprintf("Phase A (1 core, private):  %ld cycles/access\r\n", lat_a);

    /* Phase B: 4 cores, private regions */
    phase = 1;
    results[0] = 0;
    wake_harts(N_CORES - 1);
    results[hart] = chase(&private_chain[0][0], WARMUP, ITERATIONS);
    barrier(N_CORES);

    kprintf("Phase B (4 cores, private):\r\n");
    for (int i = 0; i < N_CORES; i++)
        kprintf("  Core %d: %ld cycles/access\r\n", i, results[i]);

    /* Phase C: 4 cores, shared region */
    phase = 2;
    wake_harts(N_CORES - 1);
    results[hart] = chase(&shared_chain[0], WARMUP, ITERATIONS);
    barrier(N_CORES);

    kprintf("Phase C (4 cores, shared):\r\n");
    for (int i = 0; i < N_CORES; i++)
        kprintf("  Core %d: %ld cycles/access\r\n", i, results[i]);

    kprintf("===== End Benchmark 1 =====\r\n");
    return 0;
}
