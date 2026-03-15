
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

  kprintf("\r# KLEIN-64 - Cipher and Decipher ====================================\r\n");
  kprintf("\rBlock:    deadbeeff000000f\r\n");
  kprintf("\rKey:      1234567890abcdef\r\n");
  kprintf("\rCipher:   %w%w\r\n", block_1, block_2);

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

  kprintf("\rDecipher: %w%w\r\n\r\n", block_1, block_2);
}

void klein_test_encryption_elapsed() {
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
    klein_write_to_address(KLEIN_ADDR_CTRL, 0x02);
    unsigned long _encryption_cycles = -read_csr(mcycle);

    // Wait for encryption to complete
    while (!(klein_read_to_address(KLEIN_ADDR_STATUS) == 0x03));

    // Stop the timer and get elapsed cycles
    _encryption_cycles += read_csr(mcycle);

    // Get result
//    uint32_t cipher_1, cipher_2;
//    cipher_1 = klein_read_to_address(KLEIN_ADDR_RESULT0);
//    cipher_2 = klein_read_to_address(KLEIN_ADDR_RESULT1);

    // Print results
//    kprintf("\r\n# ELAPSED - KLEIN-64 Cipher Encryption ======================\r\n");
    kprintf("Executed cycles for encryption: %d\r\n", _encryption_cycles);
//    kprintf("Cipher text:    %w%w\r\n", cipher_1, cipher_2);
}

void klein_test_decryption_elapsed() {
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
    klein_write_to_address(KLEIN_ADDR_CTRL, 0x01);
    unsigned long _decryption_cycles = -read_csr(mcycle);

    // Wait for keyschedule ready
    while (!(klein_read_to_address(KLEIN_ADDR_STATUS) == 0x01));

    // Trigger decryption
    klein_write_to_address(KLEIN_ADDR_CTRL, 0x02);

    // Wait for decryption to complete
    while (!(klein_read_to_address(KLEIN_ADDR_STATUS) == 0x03));

    // Stop the timer and get elapsed cycles
    _decryption_cycles += read_csr(mcycle);

    // Get result
//    uint32_t decipher_1, decipher_2;
//    decipher_1 = klein_read_to_address(KLEIN_ADDR_RESULT0);
//    decipher_2 = klein_read_to_address(KLEIN_ADDR_RESULT1);

    // Print results
//    kprintf("\r\n# ELAPSED - KLEIN-64 Cipher Decryption ======================\r\n");
    kprintf("Executed cycles for decryption: %d\r\n", _decryption_cycles);
//    kprintf("Decipher text:  %w%w\r\n", decipher_1, decipher_2);
}
