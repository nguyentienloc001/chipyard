
#define POLY_TRIGGER   0x10008000
#define POLY_DATA_A    0x10008004
#define POLY_DATA_B    0x10008008
#define POLY_DATA_C    0x1000800C

#define POLY_ADDR_CTRL          0x08
#define POLY_INIT_BIT           0x00
#define POLY_NEXT_BIT           0x01
#define POLY_FINSH_BIT          0x02

#define POLY_ADDR_STATUS        0x109

#define POLY_ADDR_BLOCKLEN      0x10a

#define POLY_ADDR_KEY_BASE      0x10
#define POLY_ADDR_BLOCK_BASE    0x20
#define POLY_ADDR_MAC_BASE      0x30

static void poly_write_to_address(unsigned int address, unsigned int data) {
    reg_write32(POLY_DATA_A,  address);
    reg_write32(POLY_DATA_B, data);
    reg_write32(POLY_TRIGGER, 0x03);
    reg_write32(POLY_TRIGGER, 0x00);
}

static unsigned int poly_read_from_address(unsigned int address) {
    reg_write32(POLY_DATA_A,  address);
    reg_write32(POLY_DATA_B, 0x00);

    reg_write32(POLY_TRIGGER, 0x01);
    unsigned int temp = reg_read32(POLY_DATA_C);
    reg_write32(POLY_TRIGGER, 0x00);
    return temp;
}

void poly_test_rfc8439(void) {
  unsigned int poly1305_key[8] = {0x85d6be78, 0x57556d33, 0x7f4452fe, 0x42d506a8, 0x0103808a, 0xfb0db2fd, 0x4abff6af, 0x4149f51b};

  unsigned int poly1305_block1[4] = {0x0, 0x0, 0x0, 0x0};
  unsigned int poly1305_block2[4] = {0x43727970, 0x746f6772, 0x61706869, 0x6320466f};
  unsigned int poly1305_block3[4] = {0x72756d20, 0x52657365, 0x61726368, 0x2047726f};
  unsigned int poly1305_block4[4] = {0x75700000, 0x0, 0x0, 0x0};

  unsigned int result_mac[4] = {0x0, 0x0, 0x0, 0x0};
  unsigned int mac_expected[4] = {0xa8061dc1, 0x305136c6, 0xc22b8baf, 0x0c0127a9};

  // write key
  for(int i = 0; i < 8; i++)
    poly_write_to_address(POLY_ADDR_KEY_BASE + i, poly1305_key[i]);

  // write block 1
  for(int i = 0; i < 4; i++)
    poly_write_to_address(POLY_ADDR_BLOCK_BASE + i, poly1305_block1[i]);

  //test_rfc8439: Running init() with the RFC key.
  poly_write_to_address(POLY_ADDR_CTRL, 0x1 << POLY_INIT_BIT);
  while(!(poly_read_from_address(POLY_ADDR_STATUS) == 0x01));
  //test_rfc8439: init() should be completed.

  //test_rfc8439: Loading the first 16 bytes of message and running next().
  // write block 2
  for(int i = 0; i < 4; i++)
    poly_write_to_address(POLY_ADDR_BLOCK_BASE + i, poly1305_block2[i]);
  poly_write_to_address(POLY_ADDR_BLOCKLEN, 0x10);
  poly_write_to_address(POLY_ADDR_CTRL, 0x1 << POLY_NEXT_BIT);
  while(!(poly_read_from_address(POLY_ADDR_STATUS) == 0x01));
  //test_rfc8439: next() should be completed.

  //test_rfc8439: Loading the second 16 bytes and running next().
  // write block 3
  for(int i = 0; i < 4; i++)
    poly_write_to_address(POLY_ADDR_BLOCK_BASE + i, poly1305_block3[i]);
  poly_write_to_address(POLY_ADDR_BLOCKLEN, 0x10);
  poly_write_to_address(POLY_ADDR_CTRL, 0x1 << POLY_NEXT_BIT);
  while(!(poly_read_from_address(POLY_ADDR_STATUS) == 0x01));
  //test_rfc8439: next() should be completed.

  //test_rfc8439: Loading the third 2 bytes and running next().
  // write block 4
  for(int i = 0; i < 4; i++)
    poly_write_to_address(POLY_ADDR_BLOCK_BASE + i, poly1305_block4[i]);
  poly_write_to_address(POLY_ADDR_BLOCKLEN, 0x2);
  poly_write_to_address(POLY_ADDR_CTRL, 0x1 << POLY_NEXT_BIT);
  while(!(poly_read_from_address(POLY_ADDR_STATUS) == 0x01));
  //test_rfc8439: next() should be completed.

  //test_rfc8439: running finish() to get the MAC.
  poly_write_to_address(POLY_ADDR_CTRL, 0x1 << POLY_FINSH_BIT);
  while(!(poly_read_from_address(POLY_ADDR_STATUS) == 0x01));
  //test_rfc8439: finish() should be completed.

  //test_rfc8439: Checking the generated MAC.
  for (int i = 0; i < 4; ++i)
  {
     result_mac[i] = poly_read_from_address(POLY_ADDR_MAC_BASE + i);
  }

  kprintf("# Test_rfc8439   ============================================\r\n");
  kprintf("# 256-bit key    ============================================\r\n");
  kprintf("Key:             ");
  for(int i = 0; i < 8; i++)
    kprintf("%w", poly1305_key[i]);
  kprintf("\r\n");

  kprintf("Expected result: ");
  for(int i = 0; i < 4; i++)
      kprintf("%w", mac_expected[i]);
  kprintf("\r\n");

  kprintf("Result mac:      ");
  for(int i = 0; i < 4; i++)
    kprintf("%w", result_mac[i]);
  kprintf("\r\n");
}