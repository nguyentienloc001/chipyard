#include <stdio.h>
#include "mmio.h"

//#include "accel_klein.h"
#include "accel_blake2s.h"
//#include "accel_chacha.h"
//#include "accel_present.h"
//#include "accel_dmpresent.h"


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

  // test klein
//  klein_test();

  // ChaCha dev here <======================================
//  printf("\n--------------------------------------\n");
//  chacha_soft_reset();
//  printf("ChaCha NAME0: 0x%08x\n", chacha_read_from_address(0x00));
//  printf("ChaCha NAME1: 0x%08x\n", chacha_read_from_address(0x01));
//  printf("ChaCha INFO:  0x%08x\n\n", chacha_read_from_address(0x02));
//  chacha_test_cases();

  // PRESENT dev here <======================================
//  printf("\n--------------------------------------\n");
//  present_soft_reset();
//  present_test_cases();


  // DM-PRESENT dev here <===================================
//  printf("\n--------------------------------------\n");
//  dmpresent_soft_reset();
//  dmpresent_test_cases();

  // test blake2s
  blake2s_test_empty_message();
  blake2s_test_RFC_7693();

  return 0;
}