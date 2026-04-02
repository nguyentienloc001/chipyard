/*
 * Benchmark 3: Multi-core Matrix Multiplication Scaling
 *
 * Measures parallel speedup with 1, 2, and 4 cores on 32x32 matmul.
 * Each run dispatches work to N cores and measures total cycles.
 *
 * Reuses CLINT MSIP pattern from existing MatMul test.
 * Measurement via mcycle CSR on core 0 (master).
 */

#include "bench_common.h"

#define DIM 32
#define ARRAY_SIZE (DIM * DIM)

/* Input matrices (small values to avoid overflow) */
static int A[ARRAY_SIZE];
static int B[ARRAY_SIZE];
static int C[ARRAY_SIZE];

/* Shared config for current run */
static volatile int active_cores = 0;

static void init_matrices(void) {
    for (int i = 0; i < ARRAY_SIZE; i++) {
        A[i] = i % 7;
        B[i] = (i * 3) % 5;
        C[i] = 0;
    }
}

static void clear_result(void) {
    for (int i = 0; i < ARRAY_SIZE; i++)
        C[i] = 0;
}

static void matmul(int coreid, int ncores) {
    int block = DIM / ncores;
    int start = block * coreid;

    for (int i = 0; i < DIM; i++) {
        for (int j = start; j < start + block; j++) {
            int sum = 0;
            for (int k = 0; k < DIM; k++)
                sum += A[j * DIM + k] * B[k * DIM + i];
            C[i + j * DIM] = sum;
        }
    }
}

void handle_msi(void);

void handle_msi(void) {
    uint32_t hart = read_csr(mhartid);
    int nc = active_cores;
    if (hart >= (uint32_t)nc) return;

    matmul(hart, nc);
    barrier(nc);
}

static unsigned long run_matmul(int ncores) {
    active_cores = ncores;
    clear_result();

    unsigned long start = get_cycles();

    /* Wake secondary harts */
    if (ncores > 1)
        wake_harts(ncores - 1);

    /* Core 0 does its share */
    matmul(0, ncores);
    if (ncores > 1)
        barrier(ncores);

    unsigned long end = get_cycles();
    return end - start;
}

int main(void) {
    uint32_t hart = read_csr(mhartid);
    if (hart != 0) return 0;

    uart_init();
    init_matrices();

    kprintf("\r\n===== Benchmark 3: MatMul Scaling (32x32) =====\r\n");

    /* 1 core */
    unsigned long t1 = run_matmul(1);
    kprintf("1 core:  %ld cycles\r\n", t1);

    /* 2 cores */
    unsigned long t2 = run_matmul(2);
    kprintf("2 cores: %ld cycles (speedup vs 1: %ld.%ldx)\r\n",
            t2, t1 / t2, (t1 * 10 / t2) % 10);

    /* 4 cores */
    unsigned long t4 = run_matmul(4);
    kprintf("4 cores: %ld cycles (speedup vs 1: %ld.%ldx)\r\n",
            t4, t1 / t4, (t1 * 10 / t4) % 10);

    kprintf("===== End Benchmark 3 =====\r\n");
    return 0;
}
