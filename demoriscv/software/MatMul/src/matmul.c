#include <stdint.h>
#include <riscv-pk/encoding.h>
#include "platform.h"
#include "kprintf.h"
#include "dataset.h"
#include "util.h"

static uint32_t n_cores = 4;
static int results_data[ARRAY_SIZE];


void matmul(const int coreid, const int ncores, const int lda, const int A[], const int B[], int C[])
{
  int i, j, k;
  int block = lda / ncores;
  int start = block * coreid;

  for (i = 0; i < lda; i++) {
    for (j = start; j < (start+block); j++) {
      int sum = 0;
      for (k = 0; k < lda; k++)
        sum += A[j*lda + k] * B[k*lda + i]; // Row A x Col B
      C[i + j*lda] = sum;
    }
  }
}


void handle_msi(void) {
  uint32_t mhartid = read_csr(mhartid);
  // Do matrix multiplication
  matmul(mhartid, n_cores, DIM_SIZE, input1_data, input2_data, results_data);
  barrier(n_cores);
}


void __main(void) {
  uint32_t mhartid = read_csr(mhartid);
  if (mhartid >= n_cores) while (1);

  // enable msi
  write_csr(mie, read_csr(mie) | MIP_MSIP);
  // enable global interrupt
  write_csr(mstatus, read_csr(mstatus) | MSTATUS_MIE);

  while (1) {
    __asm__ volatile ("wfi");  // wfi
  }
}


int main(void) {
  uint32_t mhartid = read_csr(mhartid);

  for (volatile int i = 0; i < 1000000; i++) { }

  unsigned long _c = -read_csr(mcycle); // Start counter
//  unsigned long _i = -read_csr(minstret);
  // Call secondary harts
  CLINT_REG(CLINT_MSIP1) = CLINT_MSIPEN; // call hart 1
  CLINT_REG(CLINT_MSIP2) = CLINT_MSIPEN; // call hart 2
  CLINT_REG(CLINT_MSIP3) = CLINT_MSIPEN; // call hart 3
  // Do matrix multiplication
  matmul(mhartid, n_cores, DIM_SIZE, input1_data, input2_data, results_data);
  barrier(n_cores);
  _c += read_csr(mcycle); // End counter
//  _i += read_csr(minstret);

  kprintf("\r\nDoing matrix multiplication in %d cycles.\r\n", _c);
//  kprintf("\r\nDoing matrix multiplication in %d inst-ret.\r\n", _i);

  // Verify
  int res = verify(ARRAY_SIZE, results_data, verify_data);
  if(!res)
    kprintf("Success !\r\n");
  else
    kprintf("Fail at %d !\r\n", res);

  return 0;
}
