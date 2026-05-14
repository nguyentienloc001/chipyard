/*
 * Benchmark 6: Hot-Spot Contention & Fairness
 *
 * All cores perform atomic add on a shared 4KB region.
 * KEY METRIC: per-core variance (lower = fairer interconnect).
 * Expected: SharedBus has high variance, NoC has lower variance.
 */

#include "bench_common.h"

#define HOTSPOT_WORDS  1024  /* 4KB shared region */
#define ITERATIONS     500
#define WARMUP         50

static volatile int32_t hotspot[HOTSPOT_WORDS] __attribute__((aligned(64)));
static volatile unsigned long hs_cycles[N_CORES];
static volatile int hs_phase = 0;

static unsigned long hotspot_work(int total_iters) {
    unsigned long start = get_cycles();
    for (int iter = 0; iter < total_iters; iter++) {
        for (int i = 0; i < HOTSPOT_WORDS; i += 16) {
            __sync_fetch_and_add(&hotspot[i], 1);
        }
    }
    unsigned long end = get_cycles();
    return end - start;
}

void handle_msi(void);

void handle_msi(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart >= N_CORES) return;

    if (hs_phase == 1) {
        hotspot_work(WARMUP);
        barrier(N_CORES);
        hs_cycles[hart] = hotspot_work(ITERATIONS);
    }

    barrier(N_CORES);
}

int main(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart != 0) return 0;

    uart_init();
    for (int i = 0; i < HOTSPOT_WORDS; i++) hotspot[i] = 0;

    kprintf("\r\n===== Benchmark 6: Hot-Spot Contention =====\r\n");

    hs_phase = 1;
    __sync_synchronize();
    wake_harts(N_CORES - 1);

    hotspot_work(WARMUP);
    barrier(N_CORES);
    hs_cycles[0] = hotspot_work(ITERATIONS);
    barrier(N_CORES);

    unsigned long sum = 0, max_c = 0, min_c = hs_cycles[0];
    for (int i = 0; i < N_CORES; i++) {
        kprintf("  Core %d: %ld cycles\r\n", i, hs_cycles[i]);
        sum += hs_cycles[i];
        if (hs_cycles[i] > max_c) max_c = hs_cycles[i];
        if (hs_cycles[i] < min_c) min_c = hs_cycles[i];
    }

    unsigned long avg = sum / N_CORES;
    unsigned long spread_pct = (max_c - min_c) * 100 / avg;
    kprintf("  Avg: %ld cycles\r\n", avg);
    kprintf("  Spread: %ld%% (lower=fairer)\r\n", spread_pct);
    kprintf("  Max/Min: %ld.%ldx\r\n",
            max_c / min_c, (max_c * 10 / min_c) % 10);
    kprintf("===== End Benchmark 6 =====\r\n");

    return 0;
}
