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
CFLAGS += -mcmodel=medany -I$(ROOT_DIR)/include -I$(SRCDIR)

LIBGLOSS_BUILD := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))../../../toolchains/libgloss/build
SPECS_FLAGS := -nostdlib -nostartfiles -static \
               -L$(RISCV)/riscv64-unknown-elf/lib \
               -T$(RISCV)/riscv64-unknown-elf/lib/htif.ld \
               $(LIBGLOSS_BUILD)/misc/crt0.o \
               -lgloss_htif -lgcc

PBUS_CLK ?= 1000000

#################################
# Benchmark targets
#################################
BENCHMARKS := bench_mem_latency bench_mem_bandwidth bench_matmul_scale bench_pingpong bench_alltoall bench_hotspot bench_streaming bench_bus_util_smoke

# Support sources (linked with every benchmark)
SUPPORT_SRCS := $(SRCDIR)/kprintf.c $(SRCDIR)/crt_stubs.c $(SRCDIR)/traps.c

.PHONY: default all clean
default: all
all: $(foreach b,$(BENCHMARKS),$(b).elf $(b).dump)

# Pattern rules
define BENCH_RULES
$(BUILD_DIR)/$(1).elf: $(SRCDIR)/$(1).c $(SUPPORT_SRCS)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SPECS_FLAGS) -DTL_CLK="$(PBUS_CLK)UL" -o $$@ $$^

.PHONY: $(1).elf
$(1).elf: $(BUILD_DIR)/$(1).elf

$(BUILD_DIR)/$(1).bin: $(BUILD_DIR)/$(1).elf
	$(OBJCOPY) -O binary $$< $$@

.PHONY: $(1).bin
$(1).bin: $(BUILD_DIR)/$(1).bin

$(BUILD_DIR)/$(1).dump: $(BUILD_DIR)/$(1).elf
	$(OBJDUMP) -D -S $$< > $$@

.PHONY: $(1).dump
$(1).dump: $(BUILD_DIR)/$(1).dump
endef

$(foreach b,$(BENCHMARKS),$(eval $(call BENCH_RULES,$(b))))

clean:
	rm -rf $(BUILD_DIR)
