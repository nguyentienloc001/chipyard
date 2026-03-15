#include <stdio.h>
#include <riscv-pk/encoding.h>

#include "mmio.h"
#include "mytimer.h"
#include "accel_prince.h"
#include "accel_chacha.h"
#include "accel_present.h"
#include "accel_dmpresent.h"
//#include "accel_sha3.h"



//#define GCD_STATUS      0x4000
//#define GCD_X           0x4004
//#define GCD_Y           0x4008
//#define GCD_GCD         0x400C
//
//#define ROM8x32_addr    0x5000
//#define ROM8x32_dout    0x5004


//unsigned int gcd_ref(unsigned int x, unsigned int y) {
//  while (y != 0) {
//    if (x > y) x = x - y;
//    else y = y - x;
//  }
//  return x;
//}

uint32_t result;

// MAIN start here <====================================
int main(void) {

  printf("     ____  _____ ____  _        _    ____  \n");
  printf("    |  _ \\| ____/ ___|| |      / \\  | __ ) \n");
  printf("    | | | |  _| \\___ \\| |     / _ \\ |  _ \\ \n");
  printf("    | |_| | |___ ___) | |___ / ___ \\| |_) |\n");
  printf("    |____/|_____|____/|_____/_/   \\_\\____/ \n");
  printf("\n");
  printf("  Dual-Core RISC-V 64-bit - A Chipyard project\n");
  printf("  with Lightweight Cryptography Accelerators ...\n");

  printf("\nRISC-V program started, hello there!\n\n");



//  printf("--------------------------------------\n");
//  uint32_t ref, x = 522, y = 186;
//
//  while ((reg_read8(GCD_STATUS) & 0x2) == 0) ;
//
//  reg_write32(GCD_X, x);
//  reg_write32(GCD_Y, y);
//  while ((reg_read8(GCD_STATUS) & 0x1) == 0) ;
//
//  result = reg_read32(GCD_GCD);
//  ref = gcd_ref(x, y);          // software ref
//
//  if (result != ref) {
//    printf("Hardware result %d does not match reference value %d\n", result, ref);
//    return 1;
//  }
//  printf("Hardware result %d is correct for GCD\n\n", result);




//  // ROM dev here <=========================================
//  printf("\n--------------------------------------\n");
//  for(int i=0;i<4;i++) {
//    reg_write32(ROM8x32_addr, i);
//    result = reg_read32(ROM8x32_dout);
//    printf("Message: 0x%08X\n", result);
//  }


  // Prince dev here <======================================
  printf("\n--------------------------------------\n");
  prince_soft_reset();
  printf("Prince NAME0: 0x%08x\n", prince_read_from_address(0x00));
  printf("Prince NAME1: 0x%08x\n", prince_read_from_address(0x01));
  printf("Prince INFO:  0x%08x\n\n", prince_read_from_address(0x02));
  prince_test_cases();


  // ChaCha dev here <======================================
  printf("\n--------------------------------------\n");
  chacha_soft_reset();
  printf("ChaCha NAME0: 0x%08x\n", chacha_read_from_address(0x00));
  printf("ChaCha NAME1: 0x%08x\n", chacha_read_from_address(0x01));
  printf("ChaCha INFO:  0x%08x\n\n", chacha_read_from_address(0x02));
  chacha_test_cases();

  // PRESENT dev here <======================================
  printf("\n--------------------------------------\n");
  present_soft_reset();
  present_test_cases();


  // DM-PRESENT dev here <===================================
  printf("\n--------------------------------------\n");
  dmpresent_soft_reset();
  dmpresent_test_cases();


  // SHA3 dev here <=========================================
//  printf("\n--------------------------------------\n");
//  sha3_test_cases();


  printf("\nEnd of RISC-V program, bye!!!\n\n");
  return 0;
}
