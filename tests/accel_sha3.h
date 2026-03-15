#include <stdio.h>
#include <riscv-pk/encoding.h>

#include "mmio.h"

#define SHA3_control  0x1000C000     // Bit high to low: reset-we-cs
#define SHA3_addr     0x1000C004
#define SHA3_write    0x1000C008
#define SHA3_read     0x1000C00C

#define CTRL_SHA3_RESET   0x04
#define CTRL_SHA3_WRITE   0x03
#define CTRL_SHA3_READ    0x01
#define CTRL_SHA3_IDLE    0x00

#define	SHA3_ADDR_RESET   	0x00
#define	SHA3_ADDR_IN 			  0x01
#define SHA3_ADDR_BYTE_NUM  0x02
#define SHA3_ADDR_IN_LAST		0x03
#define SHA3_ADDR_STATUS		0x0f

#define SHA3_ADDR_DIGEST_BASE		0x10

static void sha3_soft_reset() {
  reg_write32(SHA3_control, CTRL_SHA3_RESET);
  reg_write32(SHA3_control, CTRL_SHA3_IDLE);
}

static void sha3_write_to_address(unsigned int address, unsigned int data) {
    reg_write32(SHA3_addr,  address);
    reg_write32(SHA3_write, data);
    reg_write32(SHA3_control, CTRL_SHA3_WRITE);
    reg_write32(SHA3_control, CTRL_SHA3_IDLE);
}

static unsigned int sha3_read_from_address(unsigned int address) {
    reg_write32(SHA3_addr,  address);
    reg_write32(SHA3_write, 0x00);

    reg_write32(SHA3_control, CTRL_SHA3_READ);
    unsigned int temp = reg_read32(SHA3_read);
    reg_write32(SHA3_control, CTRL_SHA3_IDLE);
    return temp;
}


static void sha3_512_hash(unsigned int *message, unsigned int msg_len_word, unsigned int last_byte_num, unsigned int *digest) {
	sha3_write_to_address(SHA3_ADDR_RESET, 0x00);

	unsigned char i = 0;
	for(i = 0; i < (msg_len_word - 1); i++)
		sha3_write_to_address(SHA3_ADDR_IN, message[i]);

	sha3_write_to_address(SHA3_ADDR_BYTE_NUM, last_byte_num);
	sha3_write_to_address(SHA3_ADDR_IN_LAST, message[msg_len_word - 1]);

	while(sha3_read_from_address(SHA3_ADDR_STATUS) == 0);
  printf("STATUS: %08x\n", sha3_read_from_address(SHA3_ADDR_STATUS));

	for(i = 0; i < 16; i++)
		digest[i] = sha3_read_from_address(SHA3_ADDR_DIGEST_BASE + i);

}


static void sha3_test_cases() {

	unsigned int message_1[11] = {
							0x54686520, 0x71756963, 0x6b206272, 0x6f776e20,
							0x666f7820, 0x6a756d70, 0x73206f76, 0x65722074,
							0x6865206c, 0x617a7920, 0x646f6700};

	unsigned int message_2[12] = {
							0x54686520, 0x71756963, 0x6b206272, 0x6f776e20,
							0x666f7820, 0x6a756d70, 0x73206f76, 0x65722074,
							0x6865206c, 0x617a7920, 0x646f672e, 0x00000000};

	// unsigned int message_3[8] = {
	// 						0x53484133, 0x20746573 ,0x74206672, 0x6F6D2044,
	// 						0x45534C61, 0x62207669, 0x7070726F, 0x00000000};

	unsigned int message_4[5] = {
							0x48434D55, 0x53204645, 0x54454C20, 0x4445534C,
							0x41420000};

	unsigned int digest[16] = {
							0x00, 0x00, 0x00, 0x00,
							0x00, 0x00, 0x00, 0x00,
							0x00, 0x00, 0x00, 0x00,
							0x00, 0x00, 0x00, 0x00};


  printf("STATUS: %08x\n", sha3_read_from_address(SHA3_ADDR_STATUS));

	// ============================= TEST 1 =============================
	// "The quick brown fox jumps over the lazy dog "
//	sha3_soft_reset();
	sha3_512_hash(message_1, 11, 3, digest);

	printf("# SHA3-512 ===============================================================\n");
	printf("Message:  \"The quick brown fox jumps over the lazy dog \"\n");
	printf("Expected: 01dedd5de4ef14642445ba5f5b97c15e47b9ad931326e4b0727cd94cefc44fff23f07bf543139939b49128caf436dc1bdee54fcb24023a08d9403f9b4bf0d450\n");
	printf("Digest:   ");
	for(int i = 0; i < 16; i++)
		printf("%08x", digest[i]);
  printf("\n\n");





	// ============================= TEST 2 =============================
	// "The quick brown fox jumps over the lazy dog."
//	sha3_soft_reset();
	sha3_512_hash(message_2, 12, 0, digest);

	printf("# SHA3-512 ===============================================================\n");
	printf("Message:  \"The quick brown fox jumps over the lazy dog.\"\n");
	printf("Expected: 18f4f4bd419603f95538837003d9d254c26c23765565162247483f65c50303597bc9ce4d289f21d1c2f1f458828e33dc442100331b35e7eb031b5d38ba6460f8\n");
	printf("Digest:   ");
	for(int i = 0; i < 16; i++)
		printf("%08x", digest[i]);


	printf("\n\n");
}
