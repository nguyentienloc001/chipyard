/*
 * Benchmark 4: Inter-core Ping-Pong Latency
 *
 * Measures core-to-core communication latency via shared memory.
 * Core 0 sends (writes flag), target core responds (writes flag back).
 * Round-trip latency measured in cycles.
 *
 * Tests different core pairs to show topology effects:
 *   Pair A: Core 0 <-> Core 1 (1 hop in Ring)
 *   Pair B: Core 0 <-> Core 2 (2 hops in Ring)
 *   Pair C: Core 0 <-> Core 3 (3 hops in Ring)
 * Also tests concurrent pairs: (0<->1) and (2<->3) simultaneously.
 *
 * Measurement via mcycle CSR.
 */

#include "bench_common.h"

#define WARMUP      50
#define ITERATIONS  150
#define TOTAL_ITERS (WARMUP + ITERATIONS)

/* Ping-pong flags - each in its own cacheline to avoid false sharing */
static volatile int flag01 __attribute__((aligned(64))) = 0;
static volatile int flag02 __attribute__((aligned(64))) = 0;
static volatile int flag03 __attribute__((aligned(64))) = 0;
static volatile int flag23 __attribute__((aligned(64))) = 0;

/* Which test is running */
static volatile int test_id = 0;

/* Results */
static volatile unsigned long pp_result_0 = 0;
static volatile unsigned long pp_result_2 = 0;

/* Single-pair ping-pong: core 0 is initiator, target_hart is responder */
static unsigned long pingpong_initiator(volatile int *flag, int total) {
    unsigned long start = get_cycles();
    for (int i = 0; i < total; i++) {
        *flag = 1;                      /* ping */
        while (*flag != 2) ;            /* wait for pong */
        *flag = 0;                      /* reset */
    }
    unsigned long end = get_cycles();
    return end - start;
}

static void pingpong_responder(volatile int *flag, int total) {
    for (int i = 0; i < total; i++) {
        while (*flag != 1) ;            /* wait for ping */
        *flag = 2;                      /* pong */
        while (*flag != 0) ;            /* wait for reset */
    }
}

void handle_msi(void);

void handle_msi(void) {
    uint32_t hart = read_csr(mhartid);

    switch (test_id) {
    case 1: /* Core 0 <-> Core 1 */
        if (hart == 1) pingpong_responder(&flag01, TOTAL_ITERS);
        break;
    case 2: /* Core 0 <-> Core 2 */
        if (hart == 2) pingpong_responder(&flag02, TOTAL_ITERS);
        break;
    case 3: /* Core 0 <-> Core 3 */
        if (hart == 3) pingpong_responder(&flag03, TOTAL_ITERS);
        break;
    case 4: /* Concurrent: (0<->1) and (2<->3) */
        if (hart == 1) pingpong_responder(&flag01, TOTAL_ITERS);
        if (hart == 2) {
            /* Core 2 is initiator for pair 2<->3 */
            pp_result_2 = pingpong_initiator(&flag23, TOTAL_ITERS);
        }
        if (hart == 3) pingpong_responder(&flag23, TOTAL_ITERS);
        break;
    }

    barrier(N_CORES);
}

static void print_result(const char *label, unsigned long total_cycles) {
    unsigned long avg = total_cycles / ITERATIONS;
    unsigned long frac = (total_cycles * 10 / ITERATIONS) % 10;
    kprintf("  %s: %ld.%ld cycles/round-trip (total %ld cycles, %d iters)\r\n",
            label, avg, frac, total_cycles, ITERATIONS);
}

int main(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart != 0) return 0;

    uart_init();

    kprintf("\r\n===== Benchmark 4: Inter-core Ping-Pong Latency =====\r\n");

    /* Test 1: Core 0 <-> Core 1 */
    test_id = 1;
    flag01 = 0;
    wake_harts(N_CORES - 1);
    unsigned long t1_total = pingpong_initiator(&flag01, TOTAL_ITERS);
    barrier(N_CORES);
    /* Subtract warmup: approximate by ratio */
    unsigned long t1 = t1_total * ITERATIONS / TOTAL_ITERS;
    print_result("Core 0 <-> Core 1 (1 hop)", t1);

    /* Test 2: Core 0 <-> Core 2 */
    test_id = 2;
    flag02 = 0;
    wake_harts(N_CORES - 1);
    unsigned long t2_total = pingpong_initiator(&flag02, TOTAL_ITERS);
    barrier(N_CORES);
    unsigned long t2 = t2_total * ITERATIONS / TOTAL_ITERS;
    print_result("Core 0 <-> Core 2 (2 hops)", t2);

    /* Test 3: Core 0 <-> Core 3 */
    test_id = 3;
    flag03 = 0;
    wake_harts(N_CORES - 1);
    unsigned long t3_total = pingpong_initiator(&flag03, TOTAL_ITERS);
    barrier(N_CORES);
    unsigned long t3 = t3_total * ITERATIONS / TOTAL_ITERS;
    print_result("Core 0 <-> Core 3 (3 hops)", t3);

    /* Test 4: Concurrent pairs (0<->1) + (2<->3) */
    test_id = 4;
    flag01 = 0;
    flag23 = 0;
    pp_result_2 = 0;
    wake_harts(N_CORES - 1);
    unsigned long t4_total = pingpong_initiator(&flag01, TOTAL_ITERS);
    barrier(N_CORES);
    unsigned long t4_0 = t4_total * ITERATIONS / TOTAL_ITERS;
    unsigned long t4_2 = pp_result_2 * ITERATIONS / TOTAL_ITERS;
    kprintf("  Concurrent pairs:\r\n");
    print_result("    Pair 0<->1", t4_0);
    print_result("    Pair 2<->3", t4_2);

    kprintf("===== End Benchmark 4 =====\r\n");
    return 0;
}
