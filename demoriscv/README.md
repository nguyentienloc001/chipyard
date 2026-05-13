# DemoRISCV — Quad-Core RISC-V SoC with NoC Interconnect

Custom SoC design based on Chipyard, implementing a 4-core Rocket (RV64GC) system with configurable interconnect topologies on Xilinx FPGA.

## Architecture

| Component | Detail |
|-----------|--------|
| CPU | 4x Rocket Core RV64IMAFDC |
| L1 Cache | I$ 16KB + D$ 16KB per tile |
| Interconnect | Configurable: Shared Bus / Crossbar / Ring / Mesh / Tree |
| Memory | ROM 64KB + DDR3 1GB |
| Peripherals | UART, SPI (SD), GPIO (8-pin) |
| Target FPGA | Xilinx VC707 (Virtex-7) @ 50 MHz |

## Directory Structure

```
demoriscv/
├── Makefile                        # FPGA build (Vivado bitstream)
├── src/main/scala/
│   ├── arty100t/                   # Arty A7-100T FPGA configs
│   ├── vc707/                      # VC707 FPGA configs (thesis target)
│   └── resources/bootROM/          # Bootloader sources
└── software/                       # Bare-metal firmware
    ├── Benchmark/                  # Interconnect benchmarks (7 tests)
    ├── MTHello/                    # Multi-hart hello world
    └── MatMul/                     # Parallel matrix multiplication
```

## SoC Configs

Defined in `generators/chipyard/src/main/scala/config/CustomConfigs.scala`:

- **`SharedBusSoC`** — Broadcast bus baseline (worst-case interconnect)
- **`ThesisSoC`** — Base 4-core config, TileLink crossbar (no crypto)
- **`QuadCoreRing`** — 4 cores + Ring NoC topology (BidirectionalTorus1D)
- **`QuadCoreMesh`** — 4 cores + Mesh 2D (3x2) topology
- **`QuadCoreTree`** — 4 cores + Tree NoC topology (BidirectionalTree)

## Memory Map

| Address | Device |
|---------|--------|
| `0x02000000` | CLINT (inter-core interrupts) |
| `0x64000000` | UART |
| `0x80000000` | DDR3 RAM (firmware entry) |

## Build & Run

```bash
# Build benchmark firmware
cd demoriscv/software/Benchmark && make

# Build & run Verilator simulation
cd sims/verilator
make SUB_PROJECT=chipyard CONFIG=ThesisSoC
make SUB_PROJECT=chipyard CONFIG=ThesisSoC \
  BINARY=../../demoriscv/software/Benchmark/build/bench_pingpong.elf run-binary

# FPGA bitstream (requires Vivado)
cd demoriscv && make SUB_PROJECT=vc707 CONFIG=QuadCoreXBarVC707Config bitstream
```
