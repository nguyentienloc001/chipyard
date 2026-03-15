#include <stdio.h>
#include <riscv-pk/encoding.h>

#include "mmio.h"


#define CHACHA_control  0x10007000     // Bit high to low: reset-we-cs
#define CHACHA_addr     0x10007004
#define CHACHA_write    0x10007008
#define CHACHA_read     0x1000700C

#define CTRL_CHACHA_IDLE    0x00
#define CTRL_CHACHA_RESET   0x04
#define CTRL_CHACHA_WRITE   0x03
#define CTRL_CHACHA_READ    0x01

#define CHACHA_ADDR_CTRL           0x08
#define CHACHA_ADDR_STATUS         0x09
#define CHACHA_ADDR_KEYLEN         0x0a
#define CHACHA_ADDR_ROUNDS         0x0b

#define CHACHA_ADDR_KEY_BASE       0x10     // 0x10 -> 0x17
#define CHACHA_ADDR_IV0            0x20
#define CHACHA_ADDR_IV1            0x21

#define CHACHA_ADDR_DATA_IN_BASE   0x40     // 0x40 -> 0x4F
#define CHACHA_ADDR_DATA_OUT_BASE  0x80     // 0x80 -> 0x8F

#define CHACHA_KEYLEN_128 		     0x00
#define CHACHA_KEYLEN_256 		     0x01



static void chacha_soft_reset() {
  reg_write32(CHACHA_control, CTRL_CHACHA_RESET);
  reg_write32(CHACHA_control, CTRL_CHACHA_IDLE);
}

static void chacha_write_to_address(unsigned int address, unsigned int data) {
    reg_write32(CHACHA_addr,  address);
    reg_write32(CHACHA_write, data);
    reg_write32(CHACHA_control, CTRL_CHACHA_WRITE);
    reg_write32(CHACHA_control, CTRL_CHACHA_IDLE);
}

static unsigned int chacha_read_from_address(unsigned int address) {
    reg_write32(CHACHA_addr,  address);
    reg_write32(CHACHA_write, 0x00);

    reg_write32(CHACHA_control, CTRL_CHACHA_READ);
    unsigned int temp = reg_read32(CHACHA_read);
    reg_write32(CHACHA_control, CTRL_CHACHA_IDLE);
    return temp;
}

static void chacha_cipher(
        unsigned int key_len,
        unsigned int *key,
        unsigned int *iv,
        unsigned int num_round,
        unsigned int *data_in,
        unsigned int *data_out)
{
    for (int i = 0; i < 8; ++i)
        chacha_write_to_address(CHACHA_ADDR_KEY_BASE + i, key[i]);

    for (int i = 0; i < 16; ++i)
        chacha_write_to_address(CHACHA_ADDR_DATA_IN_BASE + i, data_in[i]);

    chacha_write_to_address(CHACHA_ADDR_IV0, iv[0]);
    chacha_write_to_address(CHACHA_ADDR_IV1, iv[1]);

    chacha_write_to_address(CHACHA_ADDR_KEYLEN, key_len);
    chacha_write_to_address(CHACHA_ADDR_ROUNDS, num_round);
    chacha_write_to_address(CHACHA_ADDR_CTRL, 0x01);

    while(chacha_read_from_address(CHACHA_ADDR_STATUS) == 0);

    for (int i = 0; i < 16; ++i)
        data_out[i] = chacha_read_from_address(CHACHA_ADDR_DATA_OUT_BASE + i);
}




static void chacha_cipher_next_block(unsigned int *data_out) {
    chacha_write_to_address(CHACHA_ADDR_CTRL, 0x02);

    while(chacha_read_from_address(CHACHA_ADDR_STATUS) == 0);

    for (int i = 0; i < 16; ++i)
        data_out[i] = chacha_read_from_address(CHACHA_ADDR_DATA_OUT_BASE + i);
}






static void chacha_test_cases() {
  unsigned int chacha_key[8] 	= {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	unsigned int chacha_iv[2] 	= {0x0, 0x0};
	unsigned int chacha_data_in[16] 	= {
			0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0};

	unsigned int chacha_dump[16] 	= {
			0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0};

	unsigned int chacha_expected[16] 	= {
			0xe28a5fa4, 0xa67f8c5d, 0xefed3e6f, 0xb7303486,
			0xaa8427d3, 0x1419a729, 0x572d7779, 0x53491120,
			0xb64ab8e7, 0x2b8deb85, 0xcd6aea7c, 0xb6089a10,
			0x1824beeb, 0x08814a42, 0x8aab1fa2, 0xc816081b};




    // ================================= TEST ONE =================================

  chacha_cipher(CHACHA_KEYLEN_128, chacha_key, chacha_iv, 0x08, chacha_data_in, chacha_dump);

  printf("# ChaCha - 8 rounds, 128-bit key ============================================\n");
//	printf("\nKey:             ");
//  for(int i = 0; i < 8; i++)
//      printf("%08x", chacha_key[i]);
//
//  printf("\nIV (nonce):      ");
//  for(int i = 0; i < 2; i++)
//      printf("%08x", chacha_iv[i]);

  printf("Expected result: ");
  for(int i = 0; i < 16; i++)
      printf("%08x", chacha_expected[i]);

	printf("\nResult dump:     ");
	for(int i = 0; i < 16; i++)
	    printf("%08x", chacha_dump[i]);

  printf("\n\n");



  // ================================= TEST TWO =================================
  chacha_key[0] = 0x00;
  chacha_key[1] = 0x00;
  chacha_key[2] = 0x00;
  chacha_key[3] = 0x00;
  chacha_key[4] = 0x00;
  chacha_key[5] = 0x00;
  chacha_key[6] = 0x00;
  chacha_key[7] = 0x00;

  chacha_iv[0] = 0x00;
  chacha_iv[1] = 0x00;

  chacha_expected[0] = 0x76b8e0ad;
  chacha_expected[1] = 0xa0f13d90;
  chacha_expected[2] = 0x405d6ae5;
  chacha_expected[3] = 0x5386bd28;
  chacha_expected[4] = 0xbdd219b8;
  chacha_expected[5] = 0xa08ded1a;
  chacha_expected[6] = 0xa836efcc;
  chacha_expected[7] = 0x8b770dc7;
  chacha_expected[8] = 0xda41597c;
  chacha_expected[9] = 0x5157488d;
  chacha_expected[10] = 0x7724e03f;
  chacha_expected[11] = 0xb8d84a37;
  chacha_expected[12] = 0x6a43b8f4;
  chacha_expected[13] = 0x1518a11c;
  chacha_expected[14] = 0xc387b669;
  chacha_expected[15] = 0xb2ee6586;

  chacha_cipher(CHACHA_KEYLEN_256, chacha_key, chacha_iv, 0x14, chacha_data_in, chacha_dump);

  printf("# ChaCha - 20 rounds, 256-bit key ============================================\n");
//	printf("\nKey:             ");
//  for(int i = 0; i < 8; i++)
//      printf("%08x", chacha_key[i]);
//
//  printf("\nIV (nonce):      ");
//  for(int i = 0; i < 2; i++)
//      printf("%08x", chacha_iv[i]);

  printf("\nExpected result: ");
  for(int i = 0; i < 16; i++)
      printf("%08x", chacha_expected[i]);

	printf("\nResult dump:     ");
	for(int i = 0; i < 16; i++)
	    printf("%08x", chacha_dump[i]);

  printf("\n\n");



  chacha_cipher_next_block(chacha_dump);

  printf("# ChaCha - 20 rounds, 256-bit key, next block ================================\n");
	printf("Expected result: 9f07e7be5551387a98ba977c732d080dcb0f29a048e3656912c6533e32ee7aed29b721769ce64e43d57133b074d839d531ed1f28510afb45ace10a1f4b794d6f");
	printf("\n");
//	printf("Result dump:     9f07e7be5551387a98ba977c732d080dcb0f29a048e3656912c6533e32ee7aed29b721769ce64e43d57133b074d839d531ed1f28510afb45ace10a1f4b794d6f");
	printf("Result dump:     ");
	for(int i = 0; i < 16; i++)
	    printf("%08x", chacha_dump[i]);

  printf("\n\n");

}





static void chacha_test_elapsed() {
  unsigned int chacha_key[8] 	= {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	unsigned int chacha_iv[2] 	= {0x0, 0x0};
	unsigned int chacha_data_in[16] 	= {
			0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0};

	unsigned int chacha_dump[16] 	= {
			0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0};

  unsigned int chacha_expected[16] = {
      0x76b8e0ad, 0xa0f13d90, 0x405d6ae5, 0x5386bd28,
      0xbdd219b8, 0xa08ded1a, 0xa836efcc, 0x8b770dc7,
      0xda41597c, 0x5157488d, 0x7724e03f, 0xb8d84a37,
      0x6a43b8f4, 0x1518a11c, 0xc387b669, 0xb2ee6586
  };

  unsigned int count_cycles   = 0;


  for (int i = 0; i < 8; ++i)
      chacha_write_to_address(CHACHA_ADDR_KEY_BASE + i, chacha_key[i]);

  for (int i = 0; i < 16; ++i)
      chacha_write_to_address(CHACHA_ADDR_DATA_IN_BASE + i, chacha_data_in[i]);

  chacha_write_to_address(CHACHA_ADDR_IV0, chacha_iv[0]);
  chacha_write_to_address(CHACHA_ADDR_IV1, chacha_iv[1]);

  chacha_write_to_address(CHACHA_ADDR_KEYLEN, CHACHA_KEYLEN_256);
  chacha_write_to_address(CHACHA_ADDR_ROUNDS, 0x14);

  // ====================================================
  mytimer_soft_reset();

  chacha_write_to_address(CHACHA_ADDR_CTRL, 0x01);
  mytimer_start();

  while(chacha_read_from_address(CHACHA_ADDR_STATUS) == 0);
  count_cycles = mytimer_pause_and_return();
  // ====================================================


  printf("# ELAPSED - ChaCha - 20 rounds, 256-bit key ======================\n");
  printf("Elapsed cycles: %d\n\n", count_cycles);

}

