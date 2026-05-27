#ifndef __BUS_UTIL_H
#define __BUS_UTIL_H

#include <stdint.h>

#define BUS_UTIL_BASE   0x10011000UL
#define BUS_UTIL_RESET  (BUS_UTIL_BASE + 0x20)

/* Reset all 4 counters. Call BEFORE waking secondary harts.
 * Convention: only hart 0 calls this. */
static inline void bus_util_reset(void) {
    *(volatile uint32_t *)BUS_UTIL_RESET = 1;
    __asm__ volatile ("fence" ::: "memory");
}

/* Read the active-cycle counter for a specific hart (0..3). */
static inline uint64_t bus_util_read(int hart) {
    return *(volatile uint64_t *)(BUS_UTIL_BASE + hart * 8);
}

/* Snapshot all 4 counters into the provided array.
 * Call AFTER barrier(N_CORES). Convention: only hart 0 calls this. */
static inline void bus_util_snapshot(uint64_t out[4]) {
    for (int i = 0; i < 4; i++)
        out[i] = bus_util_read(i);
}

/* Probe whether the peripheral is present in this config.
 * Returns 1 if available, 0 otherwise. Benchmarks can use this to
 * gracefully skip util reporting on configs that lack the mixin. */
static inline int bus_util_available(void) {
    bus_util_reset();
    return bus_util_read(0) < 16;
}

#endif /* __BUS_UTIL_H */
