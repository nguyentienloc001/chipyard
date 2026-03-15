#################################
# RISCV Toolchain
#################################

ROOT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR := $(ROOT_DIR)build
SRCDIR = $(ROOT_DIR)src

CC := $(RISCV)/bin/riscv64-unknown-elf-gcc
OBJCOPY := $(RISCV)/bin/riscv64-unknown-elf-objcopy
OBJDUMP := $(RISCV)/bin/riscv64-unknown-elf-objdump

CFLAGS := -march=rv64gc -mabi=lp64d -O0 -std=gnu11 -Wall -fno-common -g
CFLAGS += -mcmodel=medany -I$(ROOT_DIR)/include -I.c

# Using linker and startup file in libgloss
SPECS_FLAGS := --specs=htif_nano.specs

# Clock option passed in
PBUS_CLK ?= 1000000

#################################
# Build matmul by default
#################################
.PHONY: default
default: matmul.bin matmul.dump

#################################
# MATMUL build rules
#################################
MATMUL_ELF  := $(BUILD_DIR)/matmul.elf
MATMUL_BIN  := $(BUILD_DIR)/matmul.bin
MATMUL_DUMP := $(BUILD_DIR)/matmul.dump

# Source file
CSOURCES = $(wildcard $(SRCDIR)/*.c)

$(MATMUL_ELF): $(CSOURCES)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SPECS_FLAGS) -DTL_CLK="$(PBUS_CLK)UL" -o $@ $^

.PHONY: matmul.elf
matmul.elf: $(MATMUL_ELF)

$(MATMUL_BIN): $(MATMUL_ELF)
	mkdir -p $(BUILD_DIR)
	$(OBJCOPY) -O binary $< $@

.PHONY: matmul.bin
matmul.bin: $(MATMUL_BIN)

$(MATMUL_DUMP): $(MATMUL_ELF)
	$(OBJDUMP) -D -S $< > $@

.PHONY: matmul.dump
matmul.dump: $(MATMUL_DUMP)

#################################
# CLEAN
#################################
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
