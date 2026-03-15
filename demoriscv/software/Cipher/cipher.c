// #include <stdio.h>
#include <stdint.h>
#include <riscv-pk/encoding.h>
#include <string.h>
#include "platform.h"
#include "kprintf.h"

#include "mmio.h"

//#include "cipher/accel_ascon.h"
#include "cipher/accel_chacha.h"
//#include "cipher/accel_blake2s.h"
#include "cipher/accel_klein.h"
#include "cipher/accel_sha3.h"
//#include "cipher/accel_poly1305.h"

//#include "soft_cipher/klein64.h"
//#include "soft_cipher/chacha_soft.h"
//#include "soft_cipher/sha3_soft.h"

#define REG32(p, i)	((p)[(i) >> 2])

int main(int hartid, char **arv) {

  if(hartid == 0)
    kprintf("Hello world!\r\n");

  // Performance evaluation
//  kprintf("\r\n# KLEIN-64 - Cryptography Accelerator ===============================\r\n");
//  klein_test_encryption_elapsed();
//  klein_test_decryption_elapsed();
//  klein_test_software();
//
//  chacha_test_elapsed();
//  chacha_soft_main();
//
//  sha3_test_elapsed();
//  sha3_512_simple();

  // Function evaluation
  klein_test();
  chacha_test_cases();
  sha3_test_cases();
//  poly_test_rfc8439();

  return 0;
}
