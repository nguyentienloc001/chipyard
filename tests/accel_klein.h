#include <stdio.h>
#include "mmio.h"

#include "klein64.h"

#define KLEIN_TRIGGER   0x10006000
#define KLEIN_DATA_A    0x10006004
#define KLEIN_DATA_B    0x10006008
#define KLEIN_DATA_C    0x1000600C

#define KLEIN_ADDR_CTRL     0x00
#define KLEIN_ADDR_CONF     0x01
#define KLEIN_ADDR_STATUS   0x02

#define KLEIN_ADDR_KEY0     0x10
#define KLEIN_ADDR_KEY1     0x11

#define KLEIN_ADDR_BLOCK0   0x20
#define KLEIN_ADDR_BLOCK1   0x21

#define KLEIN_ADDR_RESULT0  0x30
#define KLEIN_ADDR_RESULT1  0x31

void klein_write_to_address(uint32_t address, uint32_t data)
{
  reg_write32(KLEIN_DATA_A, address);
  reg_write32(KLEIN_DATA_B, data);
  reg_write32(KLEIN_TRIGGER, 0x00000101);
  reg_write32(KLEIN_TRIGGER, 0x00000000);
}

uint32_t klein_read_to_address(uint32_t address)
{
  reg_write32(KLEIN_DATA_A, address);
  reg_write32(KLEIN_DATA_B, 0);
  reg_write32(KLEIN_TRIGGER, 0x00000001);
  uint32_t temp = reg_read32(KLEIN_DATA_C);
  reg_write32(KLEIN_TRIGGER, 0x00000000);
  return temp;
}

void klein_test(void)
{
  // reset
  reg_write32(KLEIN_TRIGGER, 0x00010000);
  reg_write32(KLEIN_TRIGGER, 0x00000000);
  // cipher
  klein_write_to_address(KLEIN_ADDR_BLOCK0, 0xdeadbeef); // inp
  klein_write_to_address(KLEIN_ADDR_BLOCK1, 0xf000000f);
  klein_write_to_address(KLEIN_ADDR_KEY0, 0x12345678); // key
  klein_write_to_address(KLEIN_ADDR_KEY1, 0x90abcdef);

  klein_write_to_address(KLEIN_ADDR_CONF, 0x01);
  klein_write_to_address(KLEIN_ADDR_CTRL, 0x02);

  while(!(klein_read_to_address(KLEIN_ADDR_STATUS) == 0x03));

  uint32_t block_1, block_2;
  block_1 = klein_read_to_address(KLEIN_ADDR_RESULT0);
  block_2 = klein_read_to_address(KLEIN_ADDR_RESULT1);

  printf("\r--------------------------------------\r\n");
  printf("\r# KLEIN-64 - Cipher and Decipher ============================================\r\n");
  printf("\rInput:    deadbeeff000000f\r\n");
  printf("\rOutput:   1234567890abcdef\r\n");
  printf("\rCipher:   %.8x%.8x\r\n", block_1, block_2);

  // decipher
  klein_write_to_address(KLEIN_ADDR_BLOCK0, block_1); // inp
  klein_write_to_address(KLEIN_ADDR_BLOCK1, block_2);
  klein_write_to_address(KLEIN_ADDR_KEY0, 0x12345678); // key
  klein_write_to_address(KLEIN_ADDR_KEY1, 0x90abcdef);

  klein_write_to_address(KLEIN_ADDR_CONF, 0x00);
  klein_write_to_address(KLEIN_ADDR_CTRL, 0x01);

  while(!(klein_read_to_address(KLEIN_ADDR_STATUS) == 0x01));
  klein_write_to_address(KLEIN_ADDR_CTRL, 0x02);
  while(!(klein_read_to_address(KLEIN_ADDR_STATUS) == 0x03));

  block_1 = klein_read_to_address(KLEIN_ADDR_RESULT0);
  block_2 = klein_read_to_address(KLEIN_ADDR_RESULT1);

  printf("\rDecipher: %.8x%.8x\r\n", block_1, block_2);
}

static void klein_test_encryption_elapsed() {
    unsigned int count_cycles = 0;

    // Reset the KLEIN cipher
    reg_write32(KLEIN_TRIGGER, 0x00010000);
    reg_write32(KLEIN_TRIGGER, 0x00000000);

    // Set input data (plaintext)
    klein_write_to_address(KLEIN_ADDR_BLOCK0, 0xdeadbeef);
    klein_write_to_address(KLEIN_ADDR_BLOCK1, 0xf000000f);

    // Set encryption key
    klein_write_to_address(KLEIN_ADDR_KEY0, 0x12345678);
    klein_write_to_address(KLEIN_ADDR_KEY1, 0x90abcdef);

    // Configure and start encryption
    klein_write_to_address(KLEIN_ADDR_CONF, 0x01);

    // Start measuring time
    mytimer_soft_reset();
    klein_write_to_address(KLEIN_ADDR_CTRL, 0x02);
    mytimer_start();

    // Wait for encryption to complete
    while (!(klein_read_to_address(KLEIN_ADDR_STATUS) == 0x03));

    // Stop the timer and get elapsed cycles
    count_cycles = mytimer_pause_and_return();

    // Read output (ciphertext)
    uint32_t block_1 = klein_read_to_address(KLEIN_ADDR_RESULT0);
    uint32_t block_2 = klein_read_to_address(KLEIN_ADDR_RESULT1);

    // Print results
    printf("\r\n# ELAPSED - KLEIN-64 Cipher Encryption ======================\r\n");
    printf("Elapsed cycles: %d\r\n", count_cycles);
    printf("Cipher text: %.8x%.8x\r\n", block_1, block_2);
}

static void klein_test_decryption_elapsed() {
    unsigned int count_cycles = 0;

    // Reset the KLEIN cipher
    reg_write32(KLEIN_TRIGGER, 0x00010000);
    reg_write32(KLEIN_TRIGGER, 0x00000000);

    // Set input data (ciphertext from previous encryption)
    uint32_t block_1 = 0x5a14d676; // Example ciphertext
    uint32_t block_2 = 0x98fa7e4c;

    klein_write_to_address(KLEIN_ADDR_BLOCK0, block_1);
    klein_write_to_address(KLEIN_ADDR_BLOCK1, block_2);

    // Set the same encryption key for decryption
    klein_write_to_address(KLEIN_ADDR_KEY0, 0x12345678);
    klein_write_to_address(KLEIN_ADDR_KEY1, 0x90abcdef);

    // Configure for decryption mode
    klein_write_to_address(KLEIN_ADDR_CONF, 0x00);

    // Start measuring time
    mytimer_soft_reset();
    klein_write_to_address(KLEIN_ADDR_CTRL, 0x01);
    mytimer_start();

    // Wait for keyschedule ready
    while (!(klein_read_to_address(KLEIN_ADDR_STATUS) == 0x01));
    
    // Trigger decryption
    klein_write_to_address(KLEIN_ADDR_CTRL, 0x02);
    
    // Wait for decryption to complete
    while (!(klein_read_to_address(KLEIN_ADDR_STATUS) == 0x03));

    // Stop the timer and get elapsed cycles
    count_cycles = mytimer_pause_and_return();

    // Read output (decrypted text)
    uint32_t decrypted_1 = klein_read_to_address(KLEIN_ADDR_RESULT0);
    uint32_t decrypted_2 = klein_read_to_address(KLEIN_ADDR_RESULT1);

    // Print results
    printf("\r\n# ELAPSED - KLEIN-64 Cipher Decryption ======================\r\n");
    printf("Elapsed cycles: %d\r\n", count_cycles);
    printf("Decrypted text: %.8x%.8x\r\n", decrypted_1, decrypted_2);
}

static void klein_test_software() {
    uint8_t key[8] = {0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef};
    uint8_t message[8] = {0xde, 0xad, 0xbe, 0xef, 0xf0, 0x00, 0x00, 0x0f};
    uint8_t cipher[8];
    uint8_t decrypted[8];
    unsigned int encryption_cycles, decryption_cycles;
    uint8_t cipher_text[8];
    uint8_t decipher_text[8];

    // Measure encryption time using mytimer
    mytimer_soft_reset();
    mytimer_start();
    klein64_encrypt(message, key, cipher);
    encryption_cycles = mytimer_pause_and_return();

    cipher_text[0] = cipher[0];
    cipher_text[1] = cipher[1];
    cipher_text[2] = cipher[2];
    cipher_text[3] = cipher[3];
    cipher_text[4] = cipher[4];
    cipher_text[5] = cipher[5];
    cipher_text[6] = cipher[6];
    cipher_text[7] = cipher[7];

    // Measure decryption time using mytimer
    mytimer_soft_reset();
    mytimer_start();
    klein64_decrypt(cipher, key, decrypted);
    decryption_cycles = mytimer_pause_and_return();

    decipher_text[0] = decrypted[0];
    decipher_text[1] = decrypted[1];
    decipher_text[2] = decrypted[2];
    decipher_text[3] = decrypted[3];
    decipher_text[4] = decrypted[4];
    decipher_text[5] = decrypted[5];
    decipher_text[6] = decrypted[6];
    decipher_text[7] = decrypted[7];

    // Print results
    printf("\r\n# ELAPSED - KLEIN-64 Software ======================\r\n");
    printf("Encryption Cycles: %u\r\n", encryption_cycles);
    printf("Cipher text: ");
    for (int i = 0; i < 8; i++) printf("%02X", cipher_text[i]);
    printf("\r\nDecryption Cycles: %u\r\n", decryption_cycles);
    printf("Decrypted text: ");
    for (int i = 0; i < 8; i++) printf("%02X", decipher_text[i]);
    printf("\r\n\r\n");
}