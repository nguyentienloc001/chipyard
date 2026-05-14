# NoC vs. Shared Bus vs. Crossbar: Interconnect Comparison for 4-Core RISC-V SoC

> **Status:** Skeleton — fill in measured values after simulation and FPGA runs complete.

---

## 1. Introduction

Modern multi-core SoCs require scalable on-chip interconnects. This report compares three interconnect families on a 4-core Rocket RISC-V SoC implemented on Xilinx VC707 (Virtex-7) FPGA:

- **Shared Bus** — broadcast coherence, all requests serialized (lower bound)
- **Crossbar** — point-to-point TileLink crossbar (Chipyard default)
- **Ring NoC** — BidirectionalTorus1D(6 routers) via Constellation
- **Mesh NoC** — Mesh2D(3×2, 6 nodes) via Constellation
- **Tree NoC** — BidirectionalTree(height=2, dAry=2, 7 nodes) via Constellation

All configurations use the same 4×Rocket RV64GC core, 16KB L1 I$/D$ per tile, and DDR3 memory on VC707 @ 50 MHz.

---

## 2. Experimental Setup

### SoC Configurations

| Config | Class | Interconnect | Notes |
|--------|-------|--------------|-------|
| SharedBusSoC | `SharedBusSoC` | Broadcast bus | Worst-case baseline |
| ThesisSoC | `ThesisSoC` | TileLink crossbar | Default Chipyard |
| QuadCoreRing | `QuadCoreRing` | Ring (6 routers) | 1-3 hops |
| QuadCoreMesh | `QuadCoreMesh` | Mesh 3×2 | ≤3 hops |
| QuadCoreTree | `QuadCoreTree` | Tree (7 nodes) | ≤2 hops root→leaf |

### Benchmarks

| # | Name | What it measures | Key metric |
|---|------|-----------------|------------|
| 1 | bench_mem_latency | L1-miss round-trip latency | cycles/access |
| 2 | bench_mem_bandwidth | Sequential read bandwidth | bytes/cycle |
| 3 | bench_matmul_scale | Parallel compute scaling | cycles (4-core vs 1-core) |
| 4 | bench_pingpong | Core-to-core communication | cycles/round-trip |
| 5 | bench_alltoall | N×(N-1) cross-traffic | throughput + fairness |
| 6 | bench_hotspot | Atomic contention under load | fairness variance |
| 7 | bench_streaming | Producer→3 consumers | consumer spread |

### Platform

- FPGA: Xilinx VC707 (Virtex-7 XC7VX485T)
- Clock: 50 MHz
- Memory: DDR3 1GB
- Toolchain: Vivado 2021.2, RISC-V GCC (rv64gc)

---

## 3. Simulation Results

*Run `./scripts/run_sim_matrix.sh` to generate. See `outputs/sim/` for raw logs.*

### 3.1 Ping-Pong Latency (bench_pingpong)

| Config | Core0↔Core1 | Core0↔Core2 | Core0↔Core3 |
|--------|------------|------------|------------|
| SharedBusSoC | TBD | TBD | TBD |
| ThesisSoC | TBD | TBD | TBD |
| QuadCoreRing | TBD | TBD | TBD |
| QuadCoreMesh | TBD | TBD | TBD |
| QuadCoreTree | TBD | TBD | TBD |

![Latency comparison](charts/latency_comparison.png)

### 3.2 All-to-All Traffic (bench_alltoall)

| Config | Throughput (bytes/cycle) | Fairness (max/min) |
|--------|------------------------|--------------------|
| SharedBusSoC | TBD | TBD |
| ThesisSoC | TBD | TBD |
| QuadCoreRing | TBD | TBD |
| QuadCoreMesh | TBD | TBD |
| QuadCoreTree | TBD | TBD |

![All-to-all throughput](charts/alltoall_throughput.png)

### 3.3 Hot-Spot Contention (bench_hotspot)

| Config | Avg cycles | Spread (%) | Max/Min |
|--------|-----------|-----------|---------|
| SharedBusSoC | TBD | TBD | TBD |
| ThesisSoC | TBD | TBD | TBD |
| QuadCoreRing | TBD | TBD | TBD |
| QuadCoreMesh | TBD | TBD | TBD |
| QuadCoreTree | TBD | TBD | TBD |

![Fairness comparison](charts/fairness_comparison.png)

---

## 4. FPGA Results

*Run `./scripts/run_fpga_synthesis.sh` then flash each bitstream to VC707.*

### 4.1 Synthesis Metrics

| Config | LUTs | FFs | BRAM | WNS (ns) | Power (W) |
|--------|------|-----|------|----------|----------|
| SharedBusVC707 | TBD | TBD | TBD | TBD | TBD |
| CrossbarVC707 | TBD | TBD | TBD | TBD | TBD |
| RingVC707 | TBD | TBD | TBD | TBD | TBD |
| MeshVC707 | TBD | TBD | TBD | TBD | TBD |
| TreeVC707 | TBD | TBD | TBD | TBD | TBD |

![Area overhead](charts/area_overhead.png)

### 4.2 FPGA Benchmark Results

*Flash bitstreams, run benchmarks via UART/SD card. Log outputs to `outputs/fpga/<config>/`.*

---

## 5. Analysis

### 5.1 Latency

*Fill after data collection.*

Expected: SharedBus worst, Tree best for uniform traffic due to ≤2 hops.

### 5.2 Throughput & Fairness

*Fill after data collection.*

Expected: NoC topologies show better fairness under all-to-all due to parallel paths.

### 5.3 Area Cost

*Fill after data collection.*

Expected: Ring ≈ Mesh < Tree in LUT usage. Tree has more routers (7 vs 6).

---

## 6. Conclusions

*Fill after data collection.*

Preliminary expectations:
- SharedBus worst on all traffic-intensive benchmarks
- Tree NoC best latency for memory-bound workloads (cores at leaves, memory at root)
- Mesh/Ring competitive on bandwidth-intensive workloads (multiple parallel paths)
- Area overhead of NoC: ~10-20% LUT increase over crossbar (estimated)

---

## Appendix: Running the Experiments

```bash
# 1. Build firmware
cd demoriscv/software/Benchmark && make && cd -

# 2. Run Verilator simulation matrix (parallel, see script for per-config commands)
./scripts/run_sim_matrix.sh

# 3. Parse simulation results
./scripts/parse_bench_results.sh

# 4. Run Vivado synthesis (parallel, license-limited)
./scripts/run_fpga_synthesis.sh

# 5. Extract synthesis metrics
./scripts/extract_vivado_metrics.sh

# 6. Generate charts
python3 scripts/generate_charts.py

# 7. FPGA demo (interactive)
./scripts/fpga_demo.sh
```
