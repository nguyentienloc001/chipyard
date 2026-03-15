#include <stdio.h>
#include <riscv-pk/encoding.h>

#include "mmio.h"
#include "mytimer.h"

#include "accel_prince.h"
#include "accel_chacha.h"
#include "accel_present.h"
#include "accel_dmpresent.h"
//#include "accel_sha3.h"
#include "accel_klein.h"

//#include "prince_soft.h"
//#include "chacha_soft.h"
//#include "sha3_soft.h"

// MAIN start here <====================================
int main(void) {

  printf("     ____  _____ ____  _        _    ____  \n");
  printf("    |  _ \\| ____/ ___|| |      / \\  | __ ) \n");
  printf("    | | | |  _| \\___ \\| |     / _ \\ |  _ \\ \n");
  printf("    | |_| | |___ ___) | |___ / ___ \\| |_) |\n");
  printf("    |____/|_____|____/|_____/_/   \\_\\____/ \n");
  printf("\n");
  printf("  Quad-Core RISC-V 64-bit - A Chipyard project\n");
  printf("  with NoC and Lightweight Cryptography Accelerators ...\n");

  printf("\nRISC-V program started, hello there!\n\n");



  // Klein dev here <=====================================
  klein_test_encryption_elapsed();
  klein_test_decryption_elapsed();

  klein_test_software();

  // Prince dev here <=====================================
//  prince_test_elapsed();

  // ChaCha dev here <======================================
//  chacha_test_elapsed();

  // PRESENT dev here <=====================================
//  present_test_elapsed();

  // DM-PRESENT dev here <===================================
//  dmpresent_test_elapsed();

  // SHA3 dev here <=========================================
//  sha3_test_elapsed();



  printf("#============================================================================\n");
  printf("#============================================================================\n");
  printf("\n");

//  prince_main();
//  chacha_soft_main();
//  sha3_512_simple();



  printf("\nEnd of RISC-V program, bye!!!\n\n");
  return 0;
}
