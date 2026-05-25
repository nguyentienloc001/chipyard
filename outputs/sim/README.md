# Simulation Logs

Run simulations with (from `sims/verilator/` in each config worktree):

```bash
make SUB_PROJECT=chipyard CONFIG=<cfg>
make SUB_PROJECT=chipyard CONFIG=<cfg> \
  BINARY=../../demoriscv/software/Benchmark/build/<bench>.elf \
  TIMEOUT_CYCLES=500000000 run-binary
```

## Configs
- SharedBusSoC
- ThesisSoC
- QuadCoreRing
- QuadCoreMesh
- QuadCoreTree

## Benchmarks (7 total)
- bench_mem_latency
- bench_mem_bandwidth
- bench_matmul_scale
- bench_pingpong
- bench_alltoall *(added Phase 2)*
- bench_hotspot *(added Phase 2)*
- bench_streaming *(added Phase 2)*

## Parallel execution
See plan `docs/superpowers/plans/2026-05-13-noc-fpga-demo.md` Task 10 for worktree-based parallel simulation setup.
