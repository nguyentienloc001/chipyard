#include <stdio.h>
#include <riscv-pk/encoding.h>

#include "mmio.h"

#define PRESENT_control  0x10008000     // Bit high to low: reset-we-cs
#define PRESENT_addr     0x10008004
#define PRESENT_write    0x10008008
#define PRESENT_read     0x1000800C

#define CTRL_PRESENT_IDLE    0x00
#define CTRL_PRESENT_RESET   0x04
#define CTRL_PRESENT_WRITE   0x03
#define CTRL_PRESENT_READ    0x01

#define	PRESENT_ADDR_CONTROL	0x8
#define	PRESENT_ADDR_LOAD		  0x0

#define	PRESENT_ADDR_KEY0   	0x1
#define	PRESENT_ADDR_KEY1		  0x2
#define PRESENT_ADDR_KEY2   	0x3
#define	PRESENT_ADDR_BLOCK_0	0x4
#define	PRESENT_ADDR_BLOCK_1	0x5
#define	PRESENT_ADDR_RES_0 		0x6
#define	PRESENT_ADDR_RES_1		0x7

#define	PRESENT_ENCRYPT			  0x0
#define	PRESENT_DECRYPT			  0x1


static void present_soft_reset() {
  reg_write32(PRESENT_control, CTRL_PRESENT_RESET);
  reg_write32(PRESENT_control, CTRL_PRESENT_IDLE);
}

static void present_write_to_address(unsigned int address, unsigned int data) {
    reg_write32(PRESENT_addr,  address);
    reg_write32(PRESENT_write, data);
    reg_write32(PRESENT_control, CTRL_PRESENT_WRITE);
    reg_write32(PRESENT_control, CTRL_PRESENT_IDLE);
}

static unsigned int present_read_from_address(unsigned int address) {
    reg_write32(PRESENT_addr,  address);
    reg_write32(PRESENT_write, 0x00);

    reg_write32(PRESENT_control, CTRL_PRESENT_READ);
    unsigned int temp = reg_read32(PRESENT_read);
    reg_write32(PRESENT_control, CTRL_PRESENT_IDLE);
    return temp;
}



static void present_cipher(
  unsigned char mode,
  unsigned int *key,
  unsigned int *block,
  unsigned int *res)
{
	present_write_to_address(PRESENT_ADDR_CONTROL, mode);
  present_write_to_address(PRESENT_ADDR_KEY0, key[0]);
  present_write_to_address(PRESENT_ADDR_KEY1, key[1]);
  present_write_to_address(PRESENT_ADDR_KEY2, key[2]);

  present_write_to_address(PRESENT_ADDR_BLOCK_0, block[0]);
  present_write_to_address(PRESENT_ADDR_BLOCK_1, block[1]);
  present_write_to_address(PRESENT_ADDR_LOAD, 0x1);	// START

  while(present_read_from_address(PRESENT_ADDR_RES_0) == 0);

  res[0] = present_read_from_address(PRESENT_ADDR_RES_0);
  res[1] = present_read_from_address(PRESENT_ADDR_RES_1);
}



static void present_test_cases() {

  unsigned int key[3] 	= {0x0, 0x0, 0x0};
	unsigned int block[2] 	= {0x0, 0x0};
	unsigned int exp_res[2]	= {0x0, 0x0};
	unsigned int result_dump[2] = {0x00, 0x00};

	// =================================== TEST ONE // ============================================
	key[0]	= 0x46657465;		// Key (80 bits)
	key[1]	= 0x6c48636d;
	key[2]	= 0x00007573;

	block[0] = 0x4c746e67;      // Block (message in)
	block[1] = 0x7579656e;

	exp_res[0] = 0x0e1d00d4;    // Expected cipher out
	exp_res[1] = 0xe46ba99c;

	printf("# PRESENT - Encryption ======================================================\n");
	printf("Expected result: ");
	printf("%08x_%08x\n", exp_res[0], exp_res[1]);

	// GOD SPEED!
	present_cipher(PRESENT_ENCRYPT, key, block, result_dump);

	printf("Result dump:     ");
	printf("%08x_%08x\n\n", result_dump[0], result_dump[1]);



  key[0]	= 0x46657465;		// Key (80 bits)
  key[1]	= 0x6c48636d;
  key[2]	= 0x00007573;
  block[0] = 0x0e1d00d4;      // Block (message in)
  block[1] = 0xe46ba99c;
  exp_res[0] = 0x4c746e67;    // Expected cipher out
  exp_res[1] = 0x7579656e;

  printf("# PRESENT - Decryption ======================================================\n");
  printf("Expected result: ");
  printf("%08x_%08x\n", exp_res[0], exp_res[1]);

  // GOD SPEED!
  present_cipher(PRESENT_DECRYPT, key, block, result_dump);

  printf("Result dump:     ");
  printf("%08x_%08x\n\n", result_dump[0], result_dump[1]);
}




static void present_test_elapsed() {

  unsigned int key[3] 	= {0x46657465, 0x6c48636d, 0x00007573};
	unsigned int block[2] 	= {0x4c746e67, 0x7579656e};
	unsigned int exp_res[2]	= {0x0e1d00d4, 0xe46ba99c};
	unsigned int result_dump[2] = {0x00, 0x00};

	unsigned int count_cycles   = 0;

  present_write_to_address(PRESENT_ADDR_CONTROL, PRESENT_ENCRYPT);
  present_write_to_address(PRESENT_ADDR_KEY0, key[0]);
  present_write_to_address(PRESENT_ADDR_KEY1, key[1]);
  present_write_to_address(PRESENT_ADDR_KEY2, key[2]);

  present_write_to_address(PRESENT_ADDR_BLOCK_0, block[0]);
  present_write_to_address(PRESENT_ADDR_BLOCK_1, block[1]);

  // ====================================================
  mytimer_soft_reset();

  present_write_to_address(PRESENT_ADDR_LOAD, 0x1);	// START
  mytimer_start();

  while(present_read_from_address(PRESENT_ADDR_RES_0) == 0);

  count_cycles = mytimer_pause_and_return();
  // ====================================================



  printf("# ELAPSED - PRESENT-80 - Encryption ==============================\n");
	printf("Elapsed cycles: %d\n\n", count_cycles);
}

