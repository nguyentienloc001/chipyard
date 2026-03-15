#include <stdio.h>
#include <riscv-pk/encoding.h>

#include "mmio.h"

#define DMPRESENT_control  0x10009000     // Bit high to low: reset-we-cs
#define DMPRESENT_addr     0x10009004
#define DMPRESENT_write    0x10009008
#define DMPRESENT_read     0x1000900C

#define CTRL_DMPRESENT_IDLE     0x00
#define CTRL_DMPRESENT_RESET    0x04
#define CTRL_DMPRESENT_WRITE    0x03
#define CTRL_DMPRESENT_READ     0x01

#define	DMPRESENT_ADDR_LOAD		  0x00
#define	DMPRESENT_ADDR_KEY0   	0x01
#define	DMPRESENT_ADDR_KEY1		  0x02
#define DMPRESENT_ADDR_KEY2   	0x03
#define	DMPRESENT_ADDR_BLOCK_0	0x04
#define	DMPRESENT_ADDR_BLOCK_1	0x05
#define	DMPRESENT_ADDR_RES_0 	  0x06
#define	DMPRESENT_ADDR_RES_1	  0x07



static void dmpresent_soft_reset() {
  reg_write32(DMPRESENT_control, CTRL_DMPRESENT_RESET);
  reg_write32(DMPRESENT_control, CTRL_DMPRESENT_IDLE);
}

static void dmpresent_write_to_address(unsigned int address, unsigned int data) {
    reg_write32(DMPRESENT_addr,  address);
    reg_write32(DMPRESENT_write, data);
    reg_write32(DMPRESENT_control, CTRL_DMPRESENT_WRITE);
    reg_write32(DMPRESENT_control, CTRL_DMPRESENT_IDLE);
}

static unsigned int dmpresent_read_from_address(unsigned int address) {
    reg_write32(DMPRESENT_addr,  address);
    reg_write32(DMPRESENT_write, 0x00);

    reg_write32(DMPRESENT_control, CTRL_DMPRESENT_READ);
    unsigned int temp = reg_read32(DMPRESENT_read);
    reg_write32(DMPRESENT_control, CTRL_DMPRESENT_IDLE);
    return temp;
}



static void dmpresent_hash(unsigned int *key, unsigned int *block, unsigned int *res) {
    dmpresent_write_to_address(DMPRESENT_ADDR_KEY0, key[0]);
    dmpresent_write_to_address(DMPRESENT_ADDR_KEY1, key[1]);
    dmpresent_write_to_address(DMPRESENT_ADDR_KEY2, key[2]);

    dmpresent_write_to_address(DMPRESENT_ADDR_BLOCK_0, block[0]);
    dmpresent_write_to_address(DMPRESENT_ADDR_BLOCK_1, block[1]);

    dmpresent_write_to_address(DMPRESENT_ADDR_LOAD, 0x1);	// START
    while(dmpresent_read_from_address(DMPRESENT_ADDR_RES_0) == 0);

    res[0] = dmpresent_read_from_address(DMPRESENT_ADDR_RES_0);
    res[1] = dmpresent_read_from_address(DMPRESENT_ADDR_RES_1);
}




static void dmpresent_test_cases() {
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
	exp_res[0] = 0x42696eb3;    // Expected cipher out
	exp_res[1] = 0x9112ccf2;

	printf("# DMPRESENT - Hash ==========================================================\n");
	printf("Expected result: ");
	printf("%08x_%08x\n", exp_res[0], exp_res[1]);

	dmpresent_hash(key, block, result_dump);

	printf("Result dump:     ");
	printf("%08x_%08x\n\n", result_dump[0], result_dump[1]);



  // =================================== TEST TWO // ============================================
	key[0]	= 0x00000000;		// Key (80 bits)
	key[1]	= 0x00000000;
	key[2]	= 0x00000001;
	block[0] = 0x46657465;      // Block (message in)
	block[1] = 0x6c5f5553;
	exp_res[0] = 0xd52384e3;    // Expected cipher out
	exp_res[1] = 0xdcee9ce7;

	printf("# DMPRESENT - Hash ==========================================================\n");
	printf("Expected result: ");
	printf("%08x_%08x\n", exp_res[0], exp_res[1]);

	dmpresent_hash(key, block, result_dump);

	printf("Result dump:     ");
	printf("%08x_%08x\n\n", result_dump[0], result_dump[1]);

}



static void dmpresent_test_elapsed() {
  unsigned int key[3] 	= {0x46657465, 0x6c48636d, 0x00007573};
	unsigned int block[2] 	= {0x4c746e67, 0x7579656e};
	unsigned int exp_res[2]	= {0x42696eb3, 0x9112ccf2};
	unsigned int result_dump[2] = {0x00, 0x00};

	unsigned int count_cycles   = 0;


  dmpresent_write_to_address(DMPRESENT_ADDR_KEY0, key[0]);
  dmpresent_write_to_address(DMPRESENT_ADDR_KEY1, key[1]);
  dmpresent_write_to_address(DMPRESENT_ADDR_KEY2, key[2]);

  dmpresent_write_to_address(DMPRESENT_ADDR_BLOCK_0, block[0]);
  dmpresent_write_to_address(DMPRESENT_ADDR_BLOCK_1, block[1]);


  // ====================================================
  mytimer_soft_reset();
  mytimer_clear();

  dmpresent_write_to_address(DMPRESENT_ADDR_LOAD, 0x1);	// START
  mytimer_start();

  while(dmpresent_read_from_address(DMPRESENT_ADDR_RES_0) == 0);
  count_cycles = mytimer_pause_and_return();
  // ====================================================



  printf("# ELAPSED - DM-PRESENT-80 ========================================\n");
  printf("Elapsed cycles: %d\n\n", count_cycles);
}
