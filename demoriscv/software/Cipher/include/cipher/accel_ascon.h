

#define ASCON_TRIGGER		0x10006000
#define ASCON_DATA_A		0x10006004 // data_in
#define ASCON_DATA_B		0x10006008 // data_out
#define ASCON_STATUS		0x1000600C

static uint32_t test_vector[50] = {
0x30000010, 0x00010203, 0x04050607, 0x08090A0B, 0x0C0D0E0F, 0x00000000, 0x40000010, 0x00010203, 0x04050607, 0x08090A0B,
0x0C0D0E0F, 0x50000008, 0x12458000, 0x00000000, 0x61000020, 0x00010203, 0x12521512, 0x11212315, 0x01155441, 0x21441124,
0x49643546, 0x44844231, 0x86498080, 0x10000000, 0x40000010, 0x00010203, 0x04050607, 0x08090A0B, 0x0C0D0E0F, 0x50000008,
0x12458000, 0x00000000, 0x71000020, 0x6305EAFF, 0x9D7BE529, 0x922D92AC, 0x54E66004, 0x631FB2FF, 0x5BDE3477, 0x79DD56BC,
0xC455ACA1, 0x81000010, 0xDD68B0C1, 0x9C108776, 0xC9E8BF15, 0xEE322315, 0x20000000, 0x51000008, 0x12458000, 0x00000000};

void ascon_test() {
	int i;

	for(i = 0; i < 11; i++) {
		if(!(reg_read32(ASCON_STATUS) & 0x1)) {
			reg_write32(ASCON_DATA_A, test_vector[i]);
			reg_write32(ASCON_TRIGGER, 0x00000002); // write_en
		}
	}

	reg_write32(ASCON_TRIGGER, 0x00000001);

	for(i = 11; i < 50; i++) {
		if(!(reg_read32(ASCON_STATUS) & 0x1)) {
			reg_write32(ASCON_DATA_A, test_vector[i]);
			reg_write32(ASCON_TRIGGER, 0x00000002); // write_en
		}
	}

	reg_write32(ASCON_TRIGGER, 0x00000008);

  kprintf("# Ascon - AEAD128 =================================================================\r\n");
  kprintf("Key Data:        0x000102030405060708090a0b0c0d0e0f\r\n");
  kprintf("Nonce Data:      0x000102030405060708090a0b0c0d0e0f\r\n");
  kprintf("Associated Data: 0x1245800000000000\r\n");
  kprintf("Plaintext:       0x0001020312521512112123150115544121441124496435464484423186498080\r\n");
  kprintf("--------------------- Encrypt ---------------------\r\n");
  kprintf("Ciphertext:      0x");

  i = 0;
	while(!((reg_read32(ASCON_STATUS) >> 1) & 0x1) && i < 8) {
		reg_write32(ASCON_TRIGGER, 0x00000004);
		kprintf("%x", reg_read32(ASCON_DATA_B));
		i++;
	}

	kprintf("\r\nTag:             0x");
  while(!((reg_read32(ASCON_STATUS) >> 1) & 0x1) && i < 12) {
    reg_write32(ASCON_TRIGGER, 0x00000004);
    kprintf("%x", reg_read32(ASCON_DATA_B));
    i++;
  }

  kprintf("\r\n--------------------- Decrypt ---------------------\r\n");
  kprintf("Plaintext:       0x");
  while(!((reg_read32(ASCON_STATUS) >> 1) & 0x1) && i < 20) {
    reg_write32(ASCON_TRIGGER, 0x00000004);
    kprintf("%x", reg_read32(ASCON_DATA_B));
    i++;
  }

  kprintf("\r\nTag:             0x");
  while(!((reg_read32(ASCON_STATUS) >> 1) & 0x1) && i < 24) {
    reg_write32(ASCON_TRIGGER, 0x00000004);
    kprintf("%x", reg_read32(ASCON_DATA_B));
    i++;
  }
}