#################################
# RISCV Toolchain
#################################

ROOT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR := $(ROOT_DIR)build
SRCDIR = $(ROOT_DIR)src

CC=$(RISCV)/bin/riscv64-unknown-elf-gcc
OBJCOPY=$(RISCV)/bin/riscv64-unknown-elf-objcopy
OBJDUMP=$(RISCV)/bin/riscv64-unknown-elf-objdump

CFLAGS := -march=rv64gc -mabi=lp64d -O0 -std=gnu11 -Wall -fno-common -g
CFLAGS += -mcmodel=medany -I$(ROOT_DIR)/include -I.c

# Using linker and startup file in libgloss
SPECS_FLAGS := --specs=htif_nano.specs

# Clock option passed in
PBUS_CLK ?= 1000000

#################################
# Build cipher by default
#################################
.PHONY: default
default: cipher.bin cipher.dump

#################################
# CIPHER build rules
#################################
CIPHER_ELF  := $(BUILD_DIR)/cipher.elf
CIPHER_BIN  := $(BUILD_DIR)/cipher.bin
CIPHER_DUMP := $(BUILD_DIR)/cipher.dump

$(CIPHER_ELF): cipher.c kprintf.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SPECS_FLAGS) -DTL_CLK="$(PBUS_CLK)UL" $(LFLAGS) -o $@ cipher.c kprintf.c

.PHONY: cipher.elf
cipher.elf: $(CIPHER_ELF)

$(CIPHER_BIN): $(CIPHER_ELF)
	mkdir -p $(BUILD_DIR)
	$(OBJCOPY) -O binary $< $@

.PHONY: cipher.bin
cipher.bin: $(CIPHER_BIN)

$(CIPHER_DUMP): $(CIPHER_ELF)
	$(OBJDUMP) -D -S $< > $@

.PHONY: cipher.dump
cipher.dump: $(CIPHER_DUMP)

#################################
# CLEAN
#################################
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
