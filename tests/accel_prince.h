#include <stdio.h>
#include <riscv-pk/encoding.h>

#include "mmio.h"

#define PRINCE_control  0x1000B000     // Bit high to low: reset-we-cs
#define PRINCE_addr     0x1000B004
#define PRINCE_write    0x1000B008
#define PRINCE_read     0x1000B00C

#define CTRL_PRINCE_IDLE    0x00
#define CTRL_PRINCE_RESET   0x04
#define CTRL_PRINCE_WRITE   0x03
#define CTRL_PRINCE_READ    0x01

#define PRINCE_ADDR_KEY0        0x10
#define PRINCE_ADDR_BLOCK0      0x20
#define PRINCE_ADDR_BLOCK1      0x21
#define PRINCE_ADDR_RESULT0     0x30
#define PRINCE_ADDR_RESULT1     0x31

#define PRINCE_ADDR_CTRL        0x08
#define PRINCE_ADDR_STATUS      0x09
#define PRINCE_ADDR_CONFIG      0x0a

#define PRINCE_OP_EN            0x1
#define PRINCE_OP_DE            0x0

// PRINCE base functions

static void prince_soft_reset() {
  reg_write32(PRINCE_control, CTRL_PRINCE_RESET);
  reg_write32(PRINCE_control, CTRL_PRINCE_IDLE);
}

static void prince_write_to_address(unsigned int address, unsigned int data) {
    reg_write32(PRINCE_addr,  address);
    reg_write32(PRINCE_write, data);

    reg_write32(PRINCE_control, CTRL_PRINCE_WRITE);
    reg_write32(PRINCE_control, CTRL_PRINCE_IDLE);
}

static unsigned int prince_read_from_address(unsigned int address) {
    reg_write32(PRINCE_addr,  address);
    reg_write32(PRINCE_write, 0x00);

    reg_write32(PRINCE_control, CTRL_PRINCE_READ);
    unsigned int temp = reg_read32(PRINCE_read);
    reg_write32(PRINCE_control, CTRL_PRINCE_IDLE);
    return temp;
}


static void prince_cipher(unsigned char mode, unsigned int *key, unsigned int *block, unsigned int *res) {
	for (int i = 0; i < 4; ++i)
		prince_write_to_address(PRINCE_ADDR_KEY0 + i, key[i]);

  prince_write_to_address(PRINCE_ADDR_CONFIG, mode);
  prince_write_to_address(PRINCE_ADDR_BLOCK0, block[0]);
  prince_write_to_address(PRINCE_ADDR_BLOCK1, block[1]);

  prince_write_to_address(PRINCE_ADDR_CTRL, 0x1);		// START

  while(prince_read_from_address(PRINCE_ADDR_STATUS) == 0);

//  prince_ctrl_write(CSR_CTRL_PRINCE_SEL_RD);  		// SELECTED and READ
  res[0] = prince_read_from_address(PRINCE_ADDR_RESULT0);
  res[1] = prince_read_from_address(PRINCE_ADDR_RESULT1);
}



static void prince_test_cases() {
  unsigned int key[4] 		    = {0x0, 0x0, 0x0, 0x0};
  unsigned int block[2] 		  = {0x0, 0x0};
  unsigned int exp_res[2]		  = {0x0, 0x0};
  unsigned int result_dump[2]   = {0x00, 0x00};

  key[3]		= 0x00000000;    // Key (128 bits)
  key[2]		= 0x00000000;
  key[1]		= 0x00000000;
  key[0]		= 0x00000000;
  block[1]  = 0x00000000;    // Block (message in)
  block[0]  = 0x00000000;
  exp_res[1] 	= 0x818665aa;  // Expected cipher out
  exp_res[0] 	= 0x0d02dfda;

  printf("# PRINCE test 1 - Encryption ===================================\n");
//  printf("Key input:       %08x_%08x_%08x_%08x\n", key[3], key[2], key[1], key[0]);
//  printf("Block input:     %08x_%08x\n", block[1], block[0]);
  printf("Expected result: %08x_%08x\n", exp_res[1], exp_res[0]);

  prince_cipher(PRINCE_OP_EN, key, block, result_dump);
  printf("Result dump:     ");
  printf("%08x_%08x\n", result_dump[1], result_dump[0]);



  key[3]  	= 0x00000000;       // Key (128 bits)
  key[2]  	= 0x00000000;
  key[1]  	= 0x00000000;
  key[0]  	= 0x00000000;
  block[1] 	= 0x818665aa;      // Block (message in)
  block[0] 	= 0x0d02dfda;
  exp_res[1] 	= 0x00000000;    // Expected cipher out
  exp_res[0] 	= 0x00000000;

  printf("# PRINCE test 1 - Decryption ===================================\n");
//  printf("Key input:       %08x_%08x_%08x_%08x\n", key[3], key[2], key[1], key[0]);
//  printf("Block input:     %08x_%08x\n", block[1], block[0]);
  printf("Expected result: %08x_%08x\n", exp_res[1], exp_res[0]);

  prince_cipher(PRINCE_OP_DE, key, block, result_dump);
  printf("Result dump:     ");
  printf("%08x_%08x\n", result_dump[1], result_dump[0]);


  key[3]	= 0x00112233;       // Key (128 bits)
  key[2]	= 0x44556677;
  key[1]	= 0x8899aabb;
  key[0]	= 0xccddeeff;
  block[1] = 0x01234567;      // Block (message in)
  block[0] = 0x89abcdef;
  exp_res[1] = 0xd6dcb597;    // Expected cipher out
  exp_res[0] = 0x8de756ee;

  printf("# PRINCE test 2 - Encryption ===================================\n");
//  printf("Key input:       %08x_%08x_%08x_%08x\n", key[3], key[2], key[1], key[0]);
//  printf("Block input:     %08x_%08x\n", block[1], block[0]);
  printf("Expected result: %08x_%08x\n", exp_res[1], exp_res[0]);

  prince_cipher(PRINCE_OP_EN, key, block, result_dump);
  printf("Result dump:     ");
  printf("%08x_%08x \n", result_dump[1], result_dump[0]);



  key[3]	= 0x00112233;       // Key (128 bits)
  key[2]	= 0x44556677;
  key[1]	= 0x8899aabb;
  key[0]	= 0xccddeeff;
  block[1] = 0xd6dcb597;      // Block (message in)
  block[0] = 0x8de756ee;
  exp_res[1] = 0x01234567;    // Expected cipher out
  exp_res[0] = 0x89abcdef;

  printf("# PRINCE test 2 - Decryption ===================================\n");
//  printf("Key input:       %08x_%08x_%08x_%08x\n", key[3], key[2], key[1], key[0]);
//  printf("Block input:     %08x_%08x\n", block[1], block[0]);
  printf("Expected result: %08x_%08x\n", exp_res[1], exp_res[0]);

  prince_cipher(PRINCE_OP_DE, key, block, result_dump);
  printf("Result dump:     ");
  printf("%08x_%08x\n", result_dump[1], result_dump[0]);
}






static void prince_test_elapsed() {
  unsigned int key[4] 		    = {0x0, 0x0, 0x0, 0x0};
  unsigned int block[2] 		  = {0x0, 0x0};
  unsigned int exp_res[2]		  = {0x0d02dfda, 0x818665aa};
  unsigned int result_dump[2]   = {0x00, 0x00};
  unsigned int count_cycles   = 0;

  for (int i = 0; i < 4; ++i)
    prince_write_to_address(PRINCE_ADDR_KEY0 + i, key[i]);

  prince_write_to_address(PRINCE_ADDR_CONFIG, PRINCE_OP_EN);
  prince_write_to_address(PRINCE_ADDR_BLOCK0, block[0]);
  prince_write_to_address(PRINCE_ADDR_BLOCK1, block[1]);

  // ====================================================
  mytimer_soft_reset();

  prince_write_to_address(PRINCE_ADDR_CTRL, 0x1);		// START
  mytimer_start();

  while(prince_read_from_address(PRINCE_ADDR_STATUS) == 0);

  count_cycles = mytimer_pause_and_return();
  // ====================================================


  printf("# ELAPSED - PRINCE  - Encryption =================================\n");
  printf("Elapsed cycles: %d\n\n", count_cycles);
}
