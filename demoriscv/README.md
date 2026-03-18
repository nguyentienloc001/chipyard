# DemoRISCV — Quad-Core RISC-V SoC with Crypto Accelerators

Custom SoC design based on Chipyard, implementing a 4-core Rocket (RV64GC) system with NoC interconnect and lightweight cryptographic accelerators on Xilinx FPGA.

## Architecture

| Component | Detail |
|-----------|--------|
| CPU | 4x Rocket Core RV64IMAFDC |
| L1 Cache | I$ 16KB + D$ 16KB per tile |
| Interconnect | NoC Ring Topology (Constellation, 6 routers) |
| Crypto cores | KLEIN-64, ChaCha20, SHA3-512 |
| Memory | ROM 64KB + DDR3 1GB |
| Peripherals | UART, SPI (SD), GPIO (8-pin) |
| Target FPGA | Xilinx VC707 (Virtex-7) @ 50 MHz |

## Directory Structure

```
demoriscv/
├── Makefile                        # FPGA build (Vivado bitstream)
├── src/main/scala/
│   ├── arty100t/                   # Arty A7-100T FPGA configs
│   │   ├── Configs.scala
│   │   ├── Harness.scala
│   │   ├── HarnessBinder.scala
│   │   └── MyShell.scala
│   ├── vc707/                      # VC707 FPGA configs (thesis target)
│   │   ├── Configs.scala
│   │   ├── Harness.scala
│   │   └── HarnessBinders.scala
│   └── resources/bootROM/          # Bootloader sources
│       ├── basic/                  # Single-core boot
│       └── MTBoot/                 # Multi-core boot (SMP)
└── software/                       # Bare-metal firmware
    ├── Cipher/                     # Crypto accelerator tests
    │   ├── cipher.c                # Main: KLEIN, ChaCha, SHA3 tests
    │   └── include/cipher/         # MMIO driver headers
    ├── MTHello/                    # Multi-hart hello world
    │   ├── crt0.S                  # SMP startup assembly
    │   └── mt-hello.c
    └── MatMul/                     # Parallel matrix multiplication
        └── src/matmul.c            # Row-wise partitioned across 4 harts
```

## SoC Configs

Defined in `generators/chipyard/src/main/scala/config/CustomConfigs.scala`:

- **`QuadCoreRing`** — 4 Rocket cores + KLEIN + ChaCha + SHA3 + NoC Ring (thesis config)
- **`QuadCoreMesh`** — Same but with Mesh 2D (3x2) topology
- **`ThesisSoC`** — Base config without NoC
- **`CustomSoC`** — Alternative with AES + SHA3 + ChaCha

## Memory Map

| Address | Device |
|---------|--------|
| `0x02000000` | CLINT (inter-core interrupts) |
| `0x10006000` | KLEIN-64 accelerator |
| `0x10007000` | ChaCha20 accelerator |
| `0x10008000` | SHA3-512 accelerator |
| `0x80000000` | DDR3 RAM (firmware entry) |

## Build & Run

```bash
# Build firmware
cd demoriscv/software/MTHello && make

# Generate Verilog (from repo root)
make generate CONFIG=QuadCoreRingConfig

# Build & run Verilator simulation
make verilator CONFIG=RocketConfig
make sim CONFIG=RocketConfig BINARY=demoriscv/software/MTHello/build/mt-hello.elf

# FPGA bitstream (requires Vivado)
cd demoriscv && make SUB_PROJECT=vc707 bitstream
```
