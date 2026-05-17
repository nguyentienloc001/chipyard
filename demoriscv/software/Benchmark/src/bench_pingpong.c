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
 *
 * NOC note: secondary cores are woken ONCE via IPI (CLINT/PBUS), then all
 * subsequent test coordination uses shared memory (SBUS) only.  Repeated
 * IPIs during coherence-heavy pingpong traffic deadlock constellation NOC
 * configs because simultaneous PBUS + SBUS traffic exhausts virtual channels.
 */

#include "bench_common.h"

#define WARMUP      10
#define ITERATIONS  40
#define TOTAL_ITERS (WARMUP + ITERATIONS)

/* Fewer iterations for concurrent test (4 cores = very slow in Verilator) */
#define CONC_WARMUP     5
#define CONC_ITERATIONS 15
#define CONC_TOTAL      (CONC_WARMUP + CONC_ITERATIONS)

/* Ping-pong flags - each in its own cacheline to avoid false sharing */
static volatile int flag01 __attribute__((aligned(64))) = 0;
static volatile int flag02 __attribute__((aligned(64))) = 0;
static volatile int flag03 __attribute__((aligned(64))) = 0;
static volatile int flag23 __attribute__((aligned(64))) = 0;

/*
 * Test sequencer: core 0 writes 1-4 to start each test, secondary cores
 * spin-wait on this value.  Always increases monotonically so no reset
 * needed — secondary cores spin on (test_id != expected_tid).
 */
static volatile int test_id __attribute__((aligned(64))) = 0;

/* Results written by secondary cores */
static volatile unsigned long pp_result_2 __attribute__((aligned(64))) = 0;

static inline void spin_wait_eq(volatile int *p, int expect) {
    while (*p != expect)
        __asm__ volatile ("fence" ::: "memory");
}

static unsigned long pingpong_initiator(volatile int *flag, int total) {
    unsigned long start = get_cycles();
    for (int i = 0; i < total; i++) {
        *flag = 1;
        __asm__ volatile ("fence" ::: "memory");
        spin_wait_eq(flag, 2);
        *flag = 0;
        __asm__ volatile ("fence" ::: "memory");
    }
    return get_cycles() - start;
}

static void pingpong_responder(volatile int *flag, int total) {
    for (int i = 0; i < total; i++) {
        spin_wait_eq(flag, 1);
        *flag = 2;
        __asm__ volatile ("fence" ::: "memory");
        spin_wait_eq(flag, 0);
    }
}

void handle_msi(void);

/*
 * Secondary hart loop — entered once via IPI, runs all 4 tests via
 * shared-memory signaling, then exits back to the WFI loop in __main.
 */
void handle_msi(void) {
    uint32_t hart = read_csr(mhartid);

    barrier(N_CORES);   /* sync: all harts active before core 0 starts */

    for (int tid = 1; tid <= 4; tid++) {
        /* Wait for core 0 to set test_id to this test */
        spin_wait_eq((volatile int *)&test_id, tid);

        switch (tid) {
        case 1:
            if (hart == 1) pingpong_responder(&flag01, TOTAL_ITERS);
            break;
        case 2:
            if (hart == 2) pingpong_responder(&flag02, TOTAL_ITERS);
            break;
        case 3:
            if (hart == 3) pingpong_responder(&flag03, TOTAL_ITERS);
            break;
        case 4:
            if (hart == 1) pingpong_responder(&flag01, CONC_TOTAL);
            if (hart == 2) pp_result_2 = pingpong_initiator(&flag23, CONC_TOTAL);
            if (hart == 3) pingpong_responder(&flag23, CONC_TOTAL);
            break;
        }

        barrier(N_CORES);   /* sync: test complete */
    }

    barrier(N_CORES);   /* final sync before returning to WFI */
}

static void print_result(const char *label, unsigned long total_cycles, int iters) {
    unsigned long avg = total_cycles / iters;
    unsigned long frac = (total_cycles * 10 / iters) % 10;
    kprintf("  %s: %ld.%ld cycles/round-trip (total %ld cycles, %d iters)\r\n",
            label, avg, frac, total_cycles, iters);
}

int main(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart != 0) return 0;

    uart_init();

    kprintf("\r\n===== Benchmark 4: Inter-core Ping-Pong Latency =====\r\n");

    /* Wake secondary cores ONCE — all further coordination via shared memory */
    wake_harts(N_CORES - 1);
    barrier(N_CORES);   /* wait until all harts are in handle_msi */

    /* Test 1: Core 0 <-> Core 1 */
    flag01 = 0;
    __asm__ volatile ("fence" ::: "memory");
    test_id = 1;
    unsigned long t1_total = pingpong_initiator(&flag01, TOTAL_ITERS);
    barrier(N_CORES);
    unsigned long t1 = t1_total * ITERATIONS / TOTAL_ITERS;
    print_result("Core 0 <-> Core 1 (1 hop)", t1, ITERATIONS);

    /* Test 2: Core 0 <-> Core 2 */
    flag02 = 0;
    __asm__ volatile ("fence" ::: "memory");
    test_id = 2;
    unsigned long t2_total = pingpong_initiator(&flag02, TOTAL_ITERS);
    barrier(N_CORES);
    unsigned long t2 = t2_total * ITERATIONS / TOTAL_ITERS;
    print_result("Core 0 <-> Core 2 (2 hops)", t2, ITERATIONS);

    /* Test 3: Core 0 <-> Core 3 */
    flag03 = 0;
    __asm__ volatile ("fence" ::: "memory");
    test_id = 3;
    unsigned long t3_total = pingpong_initiator(&flag03, TOTAL_ITERS);
    barrier(N_CORES);
    unsigned long t3 = t3_total * ITERATIONS / TOTAL_ITERS;
    print_result("Core 0 <-> Core 3 (3 hops)", t3, ITERATIONS);

    /* Test 4: Concurrent (0<->1) + (2<->3) */
    flag01 = 0;
    flag23 = 0;
    pp_result_2 = 0;
    __asm__ volatile ("fence" ::: "memory");
    test_id = 4;
    unsigned long t4_total = pingpong_initiator(&flag01, CONC_TOTAL);
    barrier(N_CORES);
    unsigned long t4_0 = t4_total * CONC_ITERATIONS / CONC_TOTAL;
    unsigned long t4_2 = pp_result_2 * CONC_ITERATIONS / CONC_TOTAL;
    kprintf("  Concurrent pairs:\r\n");
    print_result("    Pair 0<->1", t4_0, CONC_ITERATIONS);
    print_result("    Pair 2<->3", t4_2, CONC_ITERATIONS);

    barrier(N_CORES);   /* final sync — secondary cores exit handle_msi */

    kprintf("===== End Benchmark 4 =====\r\n");
    return 0;
}
