/*
 * Smoke test for BusUtilMonitor peripheral.
 *
 * Three assertions:
 *   (1) After reset, counter[0] is small (~0).
 *   (2) After 1000 forced cache misses on hart 0, counter[0] >> 100.
 *   (3) counter[1..3] remain small because secondary harts never ran.
 *
 * Prints "Smoke OK" on success, "FAIL: <reason>" on failure.
 * The CI Show-Results step grep's for these markers.
 */

#include "bench_common.h"
#include "bus_util.h"

#define MISS_COUNT  1000
/* Span 1000 cachelines (64 KB) starting at the DRAM base to force misses. */
#define DRAM_BASE   0x80000000UL
#define LINE_BYTES  64

void handle_msi(void);

void handle_msi(void) {
    /* Smoke test only exercises hart 0; secondaries idle. */
}

int main(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart != 0) return 0;

    uart_init();
    kprintf("\r\n===== Bus Util Smoke =====\r\n");

    /* Assertion 1: counter[0] is ~0 immediately after reset. */
    bus_util_reset();
    uint64_t after_reset = bus_util_read(0);
    kprintf("  after reset: counter[0] = %ld (expect < 16)\r\n", after_reset);
    if (after_reset > 16) {
        kprintf("  FAIL: counter not zero after reset\r\n");
        return 1;
    }

    /* Assertion 2: counter[0] grows under forced TL traffic. */
    bus_util_reset();
    volatile uint8_t sink = 0;
    for (int i = 0; i < MISS_COUNT; i++)
        sink += *(volatile uint8_t *)(DRAM_BASE + ((unsigned long)i << 6));
    if (sink == 0xff) kprintf("never\r\n"); /* prevent dead-code elimination */

    uint64_t after_load = bus_util_read(0);
    kprintf("  after %d forced misses: counter[0] = %ld (expect > 100)\r\n",
            MISS_COUNT, after_load);
    if (after_load < 100) {
        kprintf("  FAIL: counter did not advance under load\r\n");
        return 1;
    }

    /* Assertion 3: secondary harts never ran, so their counters stay small. */
    for (int i = 1; i < N_CORES; i++) {
        uint64_t c = bus_util_read(i);
        kprintf("  counter[%d] = %ld (expect < 16)\r\n", i, c);
        if (c > 16) {
            kprintf("  FAIL: counter[%d] non-zero without hart activity\r\n", i);
            return 1;
        }
    }

    kprintf("===== Smoke OK =====\r\n");
    return 0;
}
