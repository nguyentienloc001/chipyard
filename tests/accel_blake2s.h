#include <stdio.h>
#include <riscv-pk/encoding.h>
#include "mmio.h"

#define BLAKE2S_TRIGGER   0x1000A000
#define BLAKE2S_DATA_A    0x1000A004
#define BLAKE2S_DATA_B    0x1000A008
#define BLAKE2S_DATA_C    0x1000A00C

#define BLAKE2S_ADDR_CTRL     0x08
#define BLAKE2S_ADDR_STATUS   0x09
#define BLAKE2S_ADDR_BLOCKLEN 0x0A

#define BLAKE2S_ADDR_BLOCK0   0x10
#define BLAKE2S_ADDR_BLOCK15  0x1F

#define BLAKE2S_ADDR_DIGEST0  0x40
#define BLAKE2S_ADDR_DIGEST7  0x47

void blake2s_write_to_address(uint32_t address, uint32_t data)
{
  reg_write32(BLAKE2S_DATA_A, address);
  reg_write32(BLAKE2S_DATA_B, data);
  reg_write32(BLAKE2S_TRIGGER, 0x00000101);
  reg_write32(BLAKE2S_TRIGGER, 0x00000000);
}

uint32_t blake2s_read_from_address(uint32_t address)
{
  reg_write32(BLAKE2S_DATA_A, address);
  reg_write32(BLAKE2S_DATA_B, 0x0);
  reg_write32(BLAKE2S_TRIGGER, 0x00000001);
  uint32_t temp = reg_read32(BLAKE2S_DATA_C);
  reg_write32(BLAKE2S_TRIGGER, 0x00000000);
  return temp;
}

void blake2s_clear_block(void)
{
  for(int i = 0; i < 16; i++)
	  blake2s_write_to_address(BLAKE2S_ADDR_BLOCK0 + i, 0x0);
}

void blake2s_test_empty_message(void) {
	// init
	blake2s_write_to_address(BLAKE2S_ADDR_CTRL, 0x1);
	while(blake2s_read_from_address(BLAKE2S_ADDR_STATUS) == 0);

	// finish
	for(int i = 0; i < 16; i++)
		blake2s_write_to_address(BLAKE2S_ADDR_BLOCK0 + i, 0x0);
	blake2s_write_to_address(BLAKE2S_ADDR_BLOCKLEN, 0x0);
	blake2s_write_to_address(BLAKE2S_ADDR_CTRL, 0x4);
	while(blake2s_read_from_address(BLAKE2S_ADDR_STATUS) == 0);

	// read output
	printf("\r\n----------2. Test black2s with empty message\r\n");
	printf("Input:    \r\n");
	printf("Output:   ");
	for(int i = 0; i < 8; i++) {
		printf("%08x", blake2s_read_from_address(BLAKE2S_ADDR_DIGEST0 + i));
	}
	printf("\r\nExpected: 69217a3079908094e11121d042354a7c1f55b6482ca1a51e1b250dfd1ed0eef9\r\n");
}

void blake2s_test_RFC_7693(void)
{
  // init
  blake2s_write_to_address(BLAKE2S_ADDR_CTRL, 0x1);
  while(blake2s_read_from_address(BLAKE2S_ADDR_STATUS) == 0);

  // finish
  blake2s_write_to_address(BLAKE2S_ADDR_BLOCK0, 0x61626300); // abc
  for(int i = 1; i < 16; i++)
      blake2s_write_to_address(BLAKE2S_ADDR_BLOCK0 + i, 0x0);
  blake2s_write_to_address(BLAKE2S_ADDR_BLOCKLEN, 0x3);
  blake2s_write_to_address(BLAKE2S_ADDR_CTRL, 0x4);
  while(blake2s_read_from_address(BLAKE2S_ADDR_STATUS) == 0);

  // read output
  printf("\r\n----------3. Test blake2s with message in RFC 7693\r\n");
  printf("Input:    616263\r\n");
  printf("Output:   ");
  for(int i = 0; i < 8; i++) {
      printf("%08x", blake2s_read_from_address(BLAKE2S_ADDR_DIGEST0 + i));
  }
  printf("\r\nExpected: 508c5e8c327c14e2e1a72ba34eeb452f37458b209ed63a294d999b4c86675982\r\n");
}
