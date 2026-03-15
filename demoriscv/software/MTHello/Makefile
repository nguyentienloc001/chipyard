#################################
# RISCV Toolchain
#################################

ROOT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR := $(ROOT_DIR)build

CC=$(RISCV)/bin/riscv64-unknown-elf-gcc
OBJCOPY=$(RISCV)/bin/riscv64-unknown-elf-objcopy
OBJDUMP=$(RISCV)/bin/riscv64-unknown-elf-objdump

CFLAGS=-march=rv64gc -mcmodel=medany -O0 -std=gnu11 -Wall -nostartfiles
CFLAGS+= -fno-common -g -DENTROPY=0 -mabi=lp64d -DNONSMP_HART=0
CFLAGS+= -I $(ROOT_DIR)/include -I.

LFLAGS=-static -nostdlib -L $(ROOT_DIR)/linker -T hello.elf.lds

# Clock option passed in
PBUS_CLK ?= 1000000


#################################
# Build mt-hello by default
#################################
.PHONY: default
default: mt-hello.bin mt-hello.dump


#################################
# MT-HELLO build rules
#################################
MT_HELLO_ELF  := $(BUILD_DIR)/mt-hello.elf
MT_HELLO_BIN  := $(BUILD_DIR)/mt-hello.bin
MT_HELLO_DUMP := $(BUILD_DIR)/mt-hello.dump

$(MT_HELLO_ELF): mt-hello.c kprintf.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -DTL_CLK="$(PBUS_CLK)UL" $(LFLAGS) -o $@ crt0.S mt-hello.c kprintf.c

.PHONY: mt-hello.elf
mt-hello.elf: $(MT_HELLO_ELF)

$(MT_HELLO_BIN): $(MT_HELLO_ELF)
	mkdir -p $(BUILD_DIR)
	$(OBJCOPY) -O binary $< $@

.PHONY: mt-hello.bin
mt-hello.bin: $(MT_HELLO_BIN)

$(MT_HELLO_DUMP): $(MT_HELLO_ELF)
	$(OBJDUMP) -D -S $< > $@

.PHONY: mt-hello.dump
mt-hello.dump: $(MT_HELLO_DUMP)


#################################
# CLEAN
#################################
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
