# Kết quả benchmark (Benchmark Results)

## 1. Mục tiêu

So sánh hiệu năng **3 kiến trúc interconnect** trên cùng hệ thống 4-core Rocket RV64GC:

| Cấu hình | Interconnect | Topology |
|-----------|-------------|----------|
| **ThesisSoC** | TLXbar Crossbar | Crossbar tập trung (baseline) |
| **QuadCoreRing** | Constellation NoC | BidirectionalTorus1D (vòng 6 node) |
| **QuadCoreMeshThesis** | Constellation NoC | Mesh2D 3×2 (lưới 6 node) |

Tất cả config đều dùng: 4 Rocket BigCore, cùng crypto (KLEIN + ChaCha + SHA3), cùng UART (0x64000000), cùng DDR3.

## 2. Phương pháp đo

- **Công cụ**: Verilator (cycle-accurate RTL simulation)
- **Cycle counter**: CSR `mcycle` — hardware counter built-in của Rocket core, đếm chính xác từng clock cycle
- **Memory ordering**: `fence` instruction trước/sau mỗi lần đọc mcycle
- **Firmware**: cùng binary `.elf` chạy trên cả 3 config (chỉ khác HW)
- **Compiler**: `riscv64-unknown-elf-gcc -O0` (không tối ưu, đảm bảo đo đúng behavior)

## 3. Bài test

### 3.1 Benchmark 1: Memory Latency

**Mục tiêu:** Đo latency truy cập memory (cycles/access) dưới các mức contention khác nhau.

**Phương pháp:** Pointer chasing — mỗi load phụ thuộc kết quả load trước (`p = p->next`), tránh prefetch.
- Node size: 64 bytes (1 cacheline), 256 node/chain
- Warmup: 100 iterations, đo: 1000 iterations

| Phase | Mô tả |
|-------|-------|
| A | 1 core, vùng nhớ riêng — baseline latency |
| B | 4 cores, mỗi core truy cập vùng riêng — đo contention trên interconnect |
| C | 4 cores, cùng truy cập 1 vùng — worst-case contention + coherence |

### 3.2 Benchmark 2: Memory Bandwidth

**Mục tiêu:** Đo throughput đọc/ghi tuần tự (bytes/cycle).

**Phương pháp:** Đọc/ghi tuần tự mảng `int32_t`, dùng `volatile` để tránh compiler optimize.

| Phase | Mô tả |
|-------|-------|
| A | 1 core đọc + ghi 64KB |
| B | 4 cores, mỗi core đọc 16KB riêng — aggregate bandwidth |
| C | 4 cores, cùng đọc 64KB — sharing overhead |

### 3.3 Benchmark 3: MatMul Scaling

**Mục tiêu:** Đo speedup thực tế khi tăng số core.

**Phương pháp:** Nhân ma trận 32×32 int32, chia block rows cho mỗi core.
- Core 0 dispatch via CLINT MSIP, đo tổng thời gian bằng mcycle
- Barrier đồng bộ hoàn thành

| Run | Mô tả |
|-----|-------|
| 1 core | Core 0 tính toàn bộ → T1 |
| 2 cores | Mỗi core 16 rows → T2, Speedup = T1/T2 |
| 4 cores | Mỗi core 8 rows → T4, Speedup = T1/T4 |

### 3.4 Benchmark 4: Inter-core Ping-Pong

**Mục tiêu:** Đo latency giao tiếp core-to-core qua shared memory.

**Phương pháp:** Core A ghi flag=1 (ping), Core B đợi flag==1 rồi ghi flag=2 (pong), Core A đợi flag==2. Đo round-trip time.
- Mỗi flag nằm trên cacheline riêng (tránh false sharing)
- Warmup: 50 iterations, đo: 150 iterations

| Test | Mô tả |
|------|-------|
| Pair A | Core 0 ↔ Core 1 (1 hop trong Ring) |
| Pair B | Core 0 ↔ Core 2 (2 hops trong Ring) |
| Pair C | Core 0 ↔ Core 3 (3 hops trong Ring) |
| Concurrent | (0↔1) + (2↔3) đồng thời — đo interference |

## 4. Kết quả: ThesisSoC (Crossbar Baseline)

> Chạy trên Verilator, Docker image `locnguyen96/chipyard-dev:latest` (arm64)

### 4.1 Memory Latency (cycles/access)

| Phase | Core 0 | Core 1 | Core 2 | Core 3 |
|-------|--------|--------|--------|--------|
| A (1 core, private) | 17 | — | — | — |
| B (4 cores, private) | 17 | 17 | 17 | 17 |
| C (4 cores, shared) | 17 | 17 | 17 | 17 |

**Nhận xét:** Crossbar cho latency đồng đều ~17 cycles/access bất kể số core hay contention. Điều này do pointer-chase pattern chủ yếu hit L1 cache (256 node × 64B = 16KB, vừa D$ 16KB).

### 4.2 Memory Bandwidth

| Phase | Kết quả |
|-------|---------|
| A — 1 core, Read 64KB | 545,556 cycles |
| A — 1 core, Write 64KB | 472,766 cycles |
| B — 4 cores, private 16KB each | Core 0: 138,270 / Core 1: 135,463 / Core 2: 135,497 / Core 3: 135,467 cycles |
| B — Aggregate | 65,536 bytes trong 138,270 cycles |
| C — 4 cores, shared 64KB | Core 0: 547,943 / Core 1: 547,908 / Core 2: 547,908 / Core 3: 547,892 cycles |

**Nhận xét:**
- Phase B: 4 cores đọc private 16KB mỗi core chỉ mất ~135K cycles (vs 545K cho 1 core đọc 64KB). Aggregate throughput tăng ~4x.
- Phase C: 4 cores cùng đọc shared 64KB → mỗi core mất ~548K cycles, gần bằng 1 core đọc 64KB (545K). Contention không tăng latency đáng kể trên crossbar.

### 4.3 MatMul Scaling (32×32)

| Số core | Cycles | Speedup |
|---------|--------|---------|
| 1 | 1,725,320 | 1.0x |
| 2 | 868,469 | **1.9x** |
| 4 | 435,497 | **3.9x** |

**Nhận xét:** Crossbar đạt gần linear scaling (3.9x với 4 cores). MatMul là compute-bound (tính toán nhiều hơn memory access), nên interconnect chưa phải bottleneck ở kích thước ma trận 32×32.

### 4.4 Ping-Pong Latency (cycles/round-trip)

| Test | Latency |
|------|---------|
| Core 0 ↔ Core 1 (1 hop) | 162.5 |
| Core 0 ↔ Core 2 (2 hops) | 160.8 |
| Core 0 ↔ Core 3 (3 hops) | 160.8 |
| Concurrent: Pair 0↔1 | 162.4 |
| Concurrent: Pair 2↔3 | 162.4 |

**Nhận xét:**
- Crossbar cho latency **đồng đều** (~160-162 cycles) cho mọi cặp core — đúng lý thuyết vì crossbar không có khái niệm "khoảng cách"
- Concurrent 2 pairs: latency gần như không đổi → crossbar xử lý tốt 2 luồng đồng thời ở mức tải này

## 5. Kết quả: QuadCoreRing (Ring NoC)

> *Đang chạy trên CI — kết quả sẽ được cập nhật*

### Dự kiến theo lý thuyết

| Test | So với Crossbar |
|------|----------------|
| Mem Latency (1 core) | ≈ Crossbar (thêm ~1-3 cycles do router pipeline) |
| Mem Latency (4 cores) | Tốt hơn khi contention cao (phân tán traffic) |
| Bandwidth (aggregate) | Tốt hơn (parallel links vs shared bus) |
| MatMul speedup | ≈ 3.5-3.9x |
| Ping-pong | **Phụ thuộc hop count**: Pair A < Pair B < Pair C |
| Ping-pong concurrent | Gần không đổi so với single pair (non-blocking) |

## 6. Kết quả: QuadCoreMeshThesis (Mesh NoC)

> *Đang chạy trên CI — kết quả sẽ được cập nhật*

### Dự kiến theo lý thuyết

| Test | So với Ring |
|------|------------|
| Mem Latency | Tương đương hoặc tốt hơn (avg hop thấp hơn) |
| Bandwidth | Tương đương (cùng số link) |
| MatMul speedup | ≈ Ring |
| Ping-pong distant pairs | **Tốt hơn Ring** (max 3 hops nhưng avg thấp hơn) |

## 7. Tổng hợp so sánh

| Metric | Crossbar | Ring NoC | Mesh NoC | Ai thắng? |
|--------|----------|----------|----------|-----------|
| Latency 1 core | 17 cyc | *TBD* | *TBD* | ≈ Ngang nhau |
| Latency 4 cores shared | 17 cyc | *TBD* | *TBD* | NoC khi contention cao |
| BW aggregate (4 cores) | 138K cyc | *TBD* | *TBD* | NoC (parallel links) |
| MatMul 4-core speedup | 3.9x | *TBD* | *TBD* | ≈ Ngang nhau |
| Ping-pong uniform | 160-162 cyc | *TBD* | *TBD* | Crossbar (đồng đều) |
| Ping-pong concurrent | 162 cyc | *TBD* | *TBD* | NoC khi nhiều pairs |
| Scalability > 4 cores | Kém | Tốt | Rất tốt | **NoC** |

## 8. Cách chạy lại benchmark

### Chạy local (Docker)

```bash
# Build firmware
docker run --rm -v "$(pwd)":/workspace \
  -w /workspace/demoriscv/software/Benchmark \
  -e RISCV=/opt/riscv \
  locnguyen96/chipyard-dev:latest make clean all

# Build simulator + chạy benchmark
docker run --rm -v "$(pwd)":/workspace -w /workspace \
  locnguyen96/chipyard-dev:latest bash -c '
  source /opt/chipyard-env.sh
  cd sims/verilator
  make SUB_PROJECT=chipyard CONFIG=ThesisSoC \
    BINARY=/workspace/demoriscv/software/Benchmark/build/bench_matmul_scale.elf \
    TIMEOUT_CYCLES=100000000 run-binary'
```

### Chạy trên CI

Push lên branch `benchmark/noc-comparison` → GitHub Actions tự động:
1. Build libgloss
2. Build firmware (4 ELF)
3. Generate Verilog cho 3 configs
4. Build Verilator simulator
5. Chạy 4 benchmarks × 3 configs = 12 test runs
6. Upload kết quả (artifacts)

Kết quả download từ: **GitHub Actions → Artifacts → `results-{config}-{sha}`**
