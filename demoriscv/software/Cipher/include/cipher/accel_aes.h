#include "mmio.h"

#define AES_TRIGGER         0x10008000
#define AES_DATA_A          0x10008004
#define AES_DATA_B          0x10008008
#define AES_DATA_C          0x1000800C

#define AES_ADDR_NAME0      0x00
#define AES_ADDR_NAME1      0x01
#define AES_ADDR_VERSION    0x02

#define AES_ADDR_CTRL       0x08
#define AES_ADDR_STATUS   	0x09
#define AES_ADDR_CONFIG   	0x0A

#define AES_ADDR_KEY0     	0x10
#define AES_ADDR_KEY1     	0x11
#define AES_ADDR_KEY2     	0x12
#define AES_ADDR_KEY3     	0x13
#define AES_ADDR_KEY4     	0x14
#define AES_ADDR_KEY5     	0x15
#define AES_ADDR_KEY6     	0x16
#define AES_ADDR_KEY7     	0x17

#define AES_ADDR_BLOCK0   	0x20
#define AES_ADDR_BLOCK1   	0x21
#define AES_ADDR_BLOCK2   	0x22
#define AES_ADDR_BLOCK3   	0x23

#define AES_ADDR_RESULT0  	0x30
#define AES_ADDR_RESULT1  	0x31
#define AES_ADDR_RESULT2  	0x32
#define AES_ADDR_RESULT3  	0x33

#define AES_CONFIG_128_KEY  0x00    // Config value for key expansion 128
#define AES_CONFIG_256_KEY  0x02    // Config value for key expansion 256

#define AES_CONFIG_128_EN  	0x01    // Config value for encryption process (key 128-bit)
#define AES_CONFIG_128_DE  	0x00    // Config value for decryption process (key 128-bit)

#define AES_CONFIG_256_EN  	0x03    // Config value for encryption process (key 256-bit)
#define AES_CONFIG_256_DE  	0x02    // Config value for decryption process (key 256-bit)

#define AES_CTRL_INIT_KEY 	0x01    // Control value for starting key expansion
#define AES_CTRL_START 		0x02    // Control value for starting cipher process

#define AES_OP_EN          	0x01    // General "bit value convention" for encryption and decryption aes128
#define AES_OP_DE           0x00

static void aes_write_to_address(unsigned int address, unsigned int data) {
  reg_write32(AES_DATA_A, address);
  reg_write32(AES_DATA_B, data);
  reg_write32(AES_TRIGGER, 0x03);
  reg_write32(AES_TRIGGER, 0x00);
}

static unsigned int aes_read_from_address(unsigned int address) {
  reg_write32(AES_DATA_A, address);
  reg_write32(AES_DATA_B, 0);
  reg_write32(AES_TRIGGER, 0x01);
  unsigned int temp = reg_read32(AES_DATA_C);
  reg_write32(AES_TRIGGER, 0x00);
  return temp;
}

static void aes_128_cipher(unsigned char operation, unsigned int *key, unsigned int *block, unsigned int *res) {
    aes_write_to_address(AES_ADDR_KEY0, key[0]);
    aes_write_to_address(AES_ADDR_KEY1, key[1]);
    aes_write_to_address(AES_ADDR_KEY2, key[2]);
    aes_write_to_address(AES_ADDR_KEY3, key[3]);
    aes_write_to_address(AES_ADDR_KEY4, 0x0);
    aes_write_to_address(AES_ADDR_KEY5, 0x0);
    aes_write_to_address(AES_ADDR_KEY6, 0x0);
    aes_write_to_address(AES_ADDR_KEY7, 0x0);

    aes_write_to_address(AES_ADDR_CONFIG, AES_CONFIG_128_KEY);
    aes_write_to_address(AES_ADDR_CTRL, AES_CTRL_INIT_KEY);

    while(aes_read_from_address(AES_ADDR_STATUS) == 0);
    //kprintf("!Key expansion finished!\r\n");

    aes_write_to_address(AES_ADDR_BLOCK0, block[0]);
    aes_write_to_address(AES_ADDR_BLOCK1, block[1]);
    aes_write_to_address(AES_ADDR_BLOCK2, block[2]);
    aes_write_to_address(AES_ADDR_BLOCK3, block[3]);

    unsigned char AES_CONFIG = 0x00;
	if (operation == 0x01)
		AES_CONFIG = AES_CONFIG_128_EN;
	else
		AES_CONFIG = AES_CONFIG_128_DE;

	aes_write_to_address(AES_ADDR_CONFIG, AES_CONFIG);
    aes_write_to_address(AES_ADDR_CTRL, AES_CTRL_START);

    while(aes_read_from_address(AES_ADDR_STATUS) == 0);
    //kprintf("!Result!\r\n");

    res[0] = aes_read_from_address(AES_ADDR_RESULT0);
    res[1] = aes_read_from_address(AES_ADDR_RESULT1);
    res[2] = aes_read_from_address(AES_ADDR_RESULT2);
    res[3] = aes_read_from_address(AES_ADDR_RESULT3);
}


static void aes_256_cipher(unsigned char operation, unsigned int *key, unsigned int *block, unsigned int *res) {
    aes_write_to_address(AES_ADDR_KEY0, key[0]);
    aes_write_to_address(AES_ADDR_KEY1, key[1]);
    aes_write_to_address(AES_ADDR_KEY2, key[2]);
    aes_write_to_address(AES_ADDR_KEY3, key[3]);
    aes_write_to_address(AES_ADDR_KEY4, key[4]);
    aes_write_to_address(AES_ADDR_KEY5, key[5]);
    aes_write_to_address(AES_ADDR_KEY6, key[6]);
    aes_write_to_address(AES_ADDR_KEY7, key[7]);

    aes_write_to_address(AES_ADDR_CONFIG, AES_CONFIG_256_KEY);
    aes_write_to_address(AES_ADDR_CTRL, AES_CTRL_INIT_KEY);

    while(aes_read_from_address(AES_ADDR_STATUS) == 0);
    //kprintf("!Key expansion finished!\r\n");

    aes_write_to_address(AES_ADDR_BLOCK0, block[0]);
    aes_write_to_address(AES_ADDR_BLOCK1, block[1]);
    aes_write_to_address(AES_ADDR_BLOCK2, block[2]);
    aes_write_to_address(AES_ADDR_BLOCK3, block[3]);

    unsigned char AES_CONFIG = 0x00;
	if (operation == 0x01)
		AES_CONFIG = AES_CONFIG_256_EN;
	else
		AES_CONFIG = AES_CONFIG_256_DE;

	aes_write_to_address(AES_ADDR_CONFIG, AES_CONFIG);
    aes_write_to_address(AES_ADDR_CTRL, AES_CTRL_START);

    while(aes_read_from_address(AES_ADDR_STATUS) == 0);
    //kprintf("!Result!\r\n");

    res[0] = aes_read_from_address(AES_ADDR_RESULT0);
    res[1] = aes_read_from_address(AES_ADDR_RESULT1);
    res[2] = aes_read_from_address(AES_ADDR_RESULT2);
    res[3] = aes_read_from_address(AES_ADDR_RESULT3);
}


static void aes_test_cases(void)
{
//  char str[80] = "";

  // =================================================================
  unsigned int key_128[4] = {0x0, 0x0, 0x0, 0x0};
  unsigned int block[4] 	= {0x0, 0x0, 0x0, 0x0};
  unsigned int exp_res[4] = {0x0, 0x0, 0x0, 0x0};

  unsigned int res[4] 	= {0x0, 0x0, 0x0, 0x0};

  // ============================= TEST 1 - EN ============================
  key_128[0] = 0x39383736;
	key_128[1] = 0x35343332;
	key_128[2] = 0x31303938;
	key_128[3] = 0x37363534;

	block[0] = 0x31323334;
	block[1] = 0x35363738;
	block[2] = 0x39303132;
	block[3] = 0x33343536;

	exp_res[0] = 0x6f2f5312;
	exp_res[1] = 0x53e5f4da;
	exp_res[2] = 0xd07781b2;
	exp_res[3] = 0xa1e33d0b;

	aes_128_cipher(AES_OP_EN, key_128, block, res);

	kprintf("# AES128 - EN ===============================================================\r\n");
	kprintf("Expected result: ");
	kprintf("%w_%w_%w_%w\r\n", exp_res[0], exp_res[1], exp_res[2], exp_res[3]);
    kprintf("Result dump:     ");
	kprintf("%w_%w_%w_%w\r\n\r\n", res[0], res[1], res[2], res[3]);



	// ============================= TEST 2 - DE ============================
	exp_res[0] = 0x6465736c;
	exp_res[1] = 0x61627665;
	exp_res[2] = 0x78726973;
	exp_res[3] = 0x6376213f;

	key_128[0] = 0x31323334;
	key_128[1] = 0x35363738;
	key_128[2] = 0x39303132;
	key_128[3] = 0x33343536;

	block[0] = 0x6e6f9733;
	block[1] = 0x6428318a;
	block[2] = 0xbd2fb855;
	block[3] = 0xfd1ee6b4;

  // reset start
  reg_write32(AES_TRIGGER, 0x04);
  reg_write32(AES_TRIGGER, 0x00);
  // reset end
	aes_128_cipher(AES_OP_DE, key_128, block, res);

	kprintf("# AES128 - DE ===============================================================\r\n");
	kprintf("Expected result: ");
	kprintf("%w_%w_%w_%w\r\n", exp_res[0], exp_res[1], exp_res[2], exp_res[3]);
    kprintf("Result dump:     ");
	kprintf("%w_%w_%w_%w\r\n\r\n", res[0], res[1], res[2], res[3]);


 //    // ============================= TEST 3 - EN ============================
	// block[0] = 0x6bc1bee2;
	// block[1] = 0x2e409f96;
	// block[2] = 0xe93d7e11;
	// block[3] = 0x7393172a;

	// key_128[0] = 0x2b7e1516;
	// key_128[1] = 0x28aed2a6;
	// key_128[2] = 0xabf71588;
	// key_128[3] = 0x09cf4f3c;

	// exp_res[0] = 0x3ad77bb4;
	// exp_res[1] = 0x0d7a3660;
	// exp_res[2] = 0xa89ecaf3;
	// exp_res[3] = 0x2466ef97;

	// aes_128_cipher(AES_OP_EN, key_128, block, res);

	// kprintf("# AES128 - EN ===============================================================\r\n");
	// kprintf("Expected result: ");
	// sprintf(str, "%w_%w_%w_%w\r\n", exp_res[0], exp_res[1], exp_res[2], exp_res[3]);
	// kprintf(str);
 //    kprintf("Result dump:     ");
	// sprintf(str, "%w_%w_%w_%w\r\n\r\n", res[0], res[1], res[2], res[3]);
	// kprintf(str);


	// // ============================= TEST 4 - EN ============================
 //    block[0] = 0x6d616b68;
 //    block[1] = 0x61696d69;
 //    block[2] = 0x6e687669;
 //    block[3] = 0x7070726f;

 //    key_128[0] = 0x73616d73;
 //    key_128[1] = 0x756e6767;
 //    key_128[2] = 0x616c6178;
 //    key_128[3] = 0x79733231;

 //    exp_res[0] = 0x48aabecf;
 //    exp_res[1] = 0xeb074f5a;
 //    exp_res[2] = 0xf8f85a2c;
 //    exp_res[3] = 0x096918ed;

	// aes_128_cipher(AES_OP_EN, key_128, block, res);

	// printf("# AES128 - EN ===============================================================\r\n");
	// printf("Expected result: ");
	// sprintf(str, "%w_%w_%w_%w\r\n", exp_res[0], exp_res[1], exp_res[2], exp_res[3]);
	// printf(str);
 //    printf("Result dump:     ");
	// sprintf(str, "%w_%w_%w_%w\r\n\r\n", res[0], res[1], res[2], res[3]);
	// printf(str);



 //    // ============================= TEST 5 - DE ============================
 //    exp_res[0] = 0x6d616b68;
 //    exp_res[1] = 0x61696d69;
 //    exp_res[2] = 0x6e687669;
 //    exp_res[3] = 0x7070726f;

 //    key_128[0] = 0x73616d73;
 //    key_128[1] = 0x756e6767;
 //    key_128[2] = 0x616c6178;
 //    key_128[3] = 0x79733231;

 //    block[0] = 0x48aabecf;
 //    block[1] = 0xeb074f5a;
 //    block[2] = 0xf8f85a2c;
 //    block[3] = 0x096918ed;

	// aes_128_cipher(AES_OP_DE, key_128, block, res);

	// printf("# AES128 - DE ===============================================================\r\n");
	// printf("Expected result: ");
	// sprintf(str, "%w_%w_%w_%w\r\n", exp_res[0], exp_res[1], exp_res[2], exp_res[3]);
	// kprintf(str);
 //    kprintf("Result dump:     ");
	// sprintf(str, "%w_%w_%w_%w\r\n\r\n", res[0], res[1], res[2], res[3]);
	// kprintf(str);





	// ============================= TEST 6 - EN256 ============================
	//==========================================================================
    unsigned int key_256[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    key_256[0] = 0x603deb10;
    key_256[1] = 0x15ca71be;
    key_256[2] = 0x2b73aef0;
    key_256[3] = 0x857d7781;
    key_256[4] = 0x1f352c07;
    key_256[5] = 0x3b6108d7;
    key_256[6] = 0x2d9810a3;
    key_256[7] = 0x0914dff4;

    block[0] = 0x6bc1bee2;
    block[1] = 0x2e409f96;
    block[2] = 0xe93d7e11;
    block[3] = 0x7393172a;

    exp_res[0] = 0xf3eed1bd;
    exp_res[1] = 0xb5d2a03c;
    exp_res[2] = 0x064b5a7e;
    exp_res[3] = 0x3db181f8;

  // reset start
  reg_write32(AES_TRIGGER, 0x04);
  reg_write32(AES_TRIGGER, 0x00);
  // reset end
	aes_256_cipher(AES_OP_EN, key_256, block, res);

	kprintf("# AES256 - EN ===============================================================\r\n");
	kprintf("Expected result: ");
	kprintf("%w_%w_%w_%w\r\n", exp_res[0], exp_res[1], exp_res[2], exp_res[3]);
    kprintf("Result dump:     ");
	kprintf("%w_%w_%w_%w\r\n\r\n", res[0], res[1], res[2], res[3]);


	// ============================= TEST 7 - DE256 ============================
	//==========================================================================
    key_256[0] = 0x603deb10;
    key_256[1] = 0x15ca71be;
    key_256[2] = 0x2b73aef0;
    key_256[3] = 0x857d7781;
    key_256[4] = 0x1f352c07;
    key_256[5] = 0x3b6108d7;
    key_256[6] = 0x2d9810a3;
    key_256[7] = 0x0914dff4;

    exp_res[0] = 0x6bc1bee2;
    exp_res[1] = 0x2e409f96;
    exp_res[2] = 0xe93d7e11;
    exp_res[3] = 0x7393172a;

    block[0] = 0xf3eed1bd;
    block[1] = 0xb5d2a03c;
    block[2] = 0x064b5a7e;
    block[3] = 0x3db181f8;

	aes_256_cipher(AES_OP_DE, key_256, block, res);

	kprintf("# AES256 - DE ===============================================================\r\n");
	kprintf("Expected result: ");
	kprintf("%w_%w_%w_%w\r\n", exp_res[0], exp_res[1], exp_res[2], exp_res[3]);
    kprintf("Result dump:     ");
	kprintf("%w_%w_%w_%w\r\n\r\n", res[0], res[1], res[2], res[3]);
}