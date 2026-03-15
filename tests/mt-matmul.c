/* ================================*
*       Make by thh (01/2025)      *
*==================================*/

#include <riscv-pk/encoding.h>
#include <stdio.h>

#include "dataset.h"
#include "util.h"

static size_t n_cores = 4; // The number of cores
static volatile data_t hart_count = 0;

void matmul(const size_t coreid, const size_t ncores, const size_t lda,  const data_t A[], const data_t B[], data_t C[])
{
  size_t i, j, k;
  size_t block = lda / ncores;
  size_t start = block * coreid;

  for (i = 0; i < lda; i++) {
    for (j = start; j < (start+block); j++) {
      data_t sum = 0;
      for (k = 0; k < lda; k++)
        sum += A[j*lda + k] * B[k*lda + i]; // Row A x Col B
      C[i + j*lda] = sum;
    }
  }
}

void __main(void) {
  size_t mhartid = read_csr(mhartid);

  if (mhartid >= n_cores) while (1);

  // Matrix multiply
  static data_t results_data[ARRAY_SIZE];

//  unsigned long _c = -read_csr(mcycle), _i = -read_csr(minstret); // Start counter
  matmul(mhartid, n_cores, DIM_SIZE, input1_data, input2_data, results_data);
  barrier(n_cores);
//  _c += read_csr(mcycle), _i += read_csr(minstret); // End counter

  while(mhartid != hart_count);
//  printf("\nAt hart %ld: %ld cycles, %ld inst-ret, %ld.%ld CPI\n", mhartid, _c, _i, _c/_i, 10*_c/_i%10);
  hart_count++;
  barrier(n_cores);

  // Spin if not core 0
  if (mhartid > 0) while (1);

  // Verify
  int res = verify(ARRAY_SIZE, results_data, verify_data);
  if(!res)
    printf("Success !\r\n");
  else
    printf("Fail at %d !\r\n", res);
}

int main(void) {
  unsigned long _c = -read_csr(mcycle), _i = -read_csr(minstret); // Start counter
  __main();
  _c += read_csr(mcycle), _i += read_csr(minstret); // End counter
  printf("\nAt: %ld cycles, %ld inst-ret, %ld.%ld CPI\n", _c, _i, _c/_i, 10*_c/_i%10);
  return 0;
}