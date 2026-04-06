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
- Spin-wait sử dụng `fence` instruction để giảm coherence traffic trên NoC
- Single-pair: Warmup 10, đo 40 iterations
- Concurrent: Warmup 5, đo 15 iterations (giảm vì 4 cores spin-wait rất chậm trên Verilator)

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
| Core 0 ↔ Core 1 (1 hop) | 208.3 |
| Core 0 ↔ Core 2 (2 hops) | 203.5 |
| Core 0 ↔ Core 3 (3 hops) | 203.7 |
| Concurrent: Pair 0↔1 | 203.8 |
| Concurrent: Pair 2↔3 | 206.0 |

**Nhận xét:**
- Crossbar cho latency **đồng đều** (~203-208 cycles) cho mọi cặp core — đúng lý thuyết vì crossbar không có khái niệm "khoảng cách"
- Concurrent 2 pairs: latency gần như không đổi (~204-206 cycles) → crossbar xử lý tốt 2 luồng đồng thời ở mức tải này

## 5. Kết quả: QuadCoreRing (Ring NoC)

> Chạy trên Verilator, Docker image `locnguyen96/chipyard-dev:latest`

### 5.1 Memory Latency (cycles/access)

| Phase | Core 0 | Core 1 | Core 2 | Core 3 |
|-------|--------|--------|--------|--------|
| A (1 core, private) | 17 | — | — | — |
| B (4 cores, private) | 17 | 17 | 17 | 17 |
| C (4 cores, shared) | 17 | 17 | 17 | 17 |

**Nhận xét:** Ring NoC cho latency tương đương Crossbar (~17 cycles). Pointer-chase pattern chủ yếu hit L1 cache (16KB), nên interconnect chưa phải bottleneck. Router pipeline overhead không thể hiện ở mức tải này.

### 5.2 Memory Bandwidth

| Phase | Kết quả |
|-------|---------|
| A — 1 core, Read 64KB | 558,535 cycles |
| A — 1 core, Write 64KB | 484,416 cycles |
| B — 4 cores, private 16KB each | Core 0: 142,146 / Core 1: 142,356 / Core 2: 142,969 / Core 3: 142,963 cycles |
| B — Aggregate | 65,536 bytes trong 142,969 cycles |
| C — 4 cores, shared 64KB | Core 0: 560,838 / Core 1: 584,166 / Core 2: 584,104 / Core 3: 584,042 cycles |

**Nhận xét:**
- Phase A: Read chậm hơn Crossbar ~2.4% (558K vs 545K), Write chậm hơn ~2.5% (484K vs 472K) — overhead do router pipeline trên đường đến memory controller.
- Phase B: ~142K cycles/core vs Crossbar ~135-138K → chậm hơn ~3-5%. NoC routing overhead nhỏ nhưng nhất quán.
- Phase C: 560-584K vs Crossbar 547-548K → chậm hơn ~2-6%. Khoảng cách nhỏ cho thấy Ring xử lý shared access tương đương Crossbar ở mức 4 cores.

### 5.3 MatMul Scaling (32×32)

| Số core | Cycles | Speedup |
|---------|--------|---------|
| 1 | 1,725,152 | 1.0x |
| 2 | 874,138 | **1.9x** |
| 4 | 439,915 | **3.9x** |

**Nhận xét:** Ring NoC đạt gần linear scaling tương đương Crossbar (3.9x với 4 cores). MatMul compute-bound nên interconnect topology ảnh hưởng rất ít.

### 5.4 Ping-Pong Latency (cycles/round-trip)

| Test | Latency |
|------|---------|
| Core 0 ↔ Core 1 (1 hop) | 361.6 |
| Core 0 ↔ Core 2 (2 hops) | 397.0 |
| Core 0 ↔ Core 3 (3 hops) | 357.2 |
| Concurrent (0↔1 + 2↔3) | *Không thể đo trên Verilator* (xem ghi chú bên dưới) |

**Nhận xét:**
- Latency Ring **cao hơn đáng kể** so với Crossbar (357-397 vs 203-208 cycles, ~1.75x). Nguyên nhân: mỗi hop trong Ring NoC thêm router pipeline delay (route compute + VC allocation + switch traversal + link traversal).
- Pair 0↔2 (2 hops) có latency cao nhất (397.0), trong khi 0↔3 (3 hops) lại thấp hơn (357.2). Điều này do BidirectionalTorus topology cho phép đi cả 2 chiều → shortest path 0→3 có thể là 3 hops (forward) hoặc 3 hops (backward), nhưng traffic routing khác nhau ảnh hưởng latency.
- **Concurrent test không khả thi trên Verilator**: khi 4 cores đồng thời spin-wait qua NoC, mỗi poll tạo coherence traffic qua router pipeline. Verilator mô phỏng từng cycle → số lượng transactions quá lớn khiến simulation không hoàn thành trong thời gian hợp lý. Test này cần chạy trên **FPGA** (real-time) để đo được.

## 6. Kết quả: QuadCoreMeshThesis (Mesh NoC)

> Chạy trên Verilator, Docker image `locnguyen96/chipyard-dev:latest`

### 6.1 Memory Latency (cycles/access)

| Phase | Core 0 | Core 1 | Core 2 | Core 3 |
|-------|--------|--------|--------|--------|
| A (1 core, private) | 17 | — | — | — |
| B (4 cores, private) | 17 | 17 | 17 | 17 |
| C (4 cores, shared) | 17 | 17 | 17 | 17 |

**Nhận xét:** Mesh NoC cho latency tương đương Crossbar và Ring (~17 cycles). Tất cả 3 kiến trúc đều cho kết quả giống nhau ở benchmark này do pointer-chase pattern hit L1 cache.

### 6.2 Memory Bandwidth

| Phase | Kết quả |
|-------|---------|
| A — 1 core, Read 64KB | 566,728 cycles |
| A — 1 core, Write 64KB | 492,937 cycles |
| B — 4 cores, private 16KB each | Core 0: 143,446 / Core 1: 139,739 / Core 2: 141,049 / Core 3: 139,808 cycles |
| B — Aggregate | 65,536 bytes trong 143,446 cycles |
| C — 4 cores, shared 64KB | Core 0: 568,549 / Core 1: 584,649 / Core 2: 584,603 / Core 3: 584,527 cycles |

**Nhận xét:**
- Phase A: Read chậm hơn Crossbar ~3.9% (566K vs 545K), Write chậm hơn ~4.3% (492K vs 472K). Mesh overhead cao hơn Ring do XY routing qua nhiều router hơn cho một số path.
- Phase B: ~140-143K cycles/core. Đáng chú ý Core 1 và Core 3 nhanh hơn Core 0 và Core 2 — cho thấy ảnh hưởng của vị trí node trong Mesh (Core 1,3 gần Memory Controller hơn trong layout 3×2).
- Phase C: Tương đương Ring, chênh lệch không đáng kể.

### 6.3 MatMul Scaling (32×32)

| Số core | Cycles | Speedup |
|---------|--------|---------|
| 1 | 1,725,248 | 1.0x |
| 2 | 873,654 | **1.9x** |
| 4 | 438,904 | **3.9x** |

**Nhận xét:** Mesh NoC đạt scaling tương đương Ring và Crossbar. Compute-bound workload không phân biệt được 3 topology ở kích thước 4 cores.

### 6.4 Ping-Pong Latency (cycles/round-trip)

| Test | Latency |
|------|---------|
| Core 0 ↔ Core 1 (1 hop) | 350.5 |
| Core 0 ↔ Core 2 (2 hops) | 397.3 |
| Core 0 ↔ Core 3 (3 hops) | 354.2 |
| Concurrent (0↔1 + 2↔3) | *Không thể đo trên Verilator* |

**Nhận xét:**
- Mesh NoC cho latency **tương đương Ring** (350-397 vs 357-397 cycles). Ở 4 cores, Mesh 3×2 và Ring 6-node có cùng số node nên hop count trung bình tương đương.
- Pair 0↔2 có latency cao nhất (397.3) — trong Mesh layout `[0]-[1]-[2]` trên hàng trên, Core 0 đến Core 2 cần 2 hops ngang.
- So với Crossbar: NoC (cả Ring và Mesh) chậm hơn ~1.75x cho ping-pong. Đây là trade-off cho scalability.
- Concurrent test cũng không khả thi trên Verilator (cùng lý do như Ring).

## 7. Tổng hợp so sánh

| Metric | Crossbar | Ring NoC | Mesh NoC | Ghi chú |
|--------|----------|----------|----------|---------|
| Latency 1 core | 17 cyc | 17 cyc | 17 cyc | Ngang nhau (L1 cache hit) |
| Latency 4 cores shared | 17 cyc | 17 cyc | 17 cyc | Ngang nhau (L1 cache hit) |
| BW Read 1 core | 545K cyc | 558K cyc | 566K cyc | Crossbar nhanh nhất (~2-4%) |
| BW Write 1 core | 472K cyc | 484K cyc | 492K cyc | Crossbar nhanh nhất (~2-4%) |
| BW 4 cores private | 138K cyc | 142K cyc | 143K cyc | Crossbar nhanh nhất (~3%) |
| BW 4 cores shared | 548K cyc | 560-584K cyc | 569-585K cyc | Crossbar nhanh nhất (~2-6%) |
| MatMul 4-core speedup | 3.9x | 3.9x | 3.9x | **Ngang nhau** |
| Ping-pong avg | 205 cyc | 372 cyc | 367 cyc | Crossbar nhanh hơn ~1.75x |
| Ping-pong concurrent | 205 cyc | *N/A (Verilator)* | *N/A (Verilator)* | Cần FPGA để đo NoC concurrent |
| Scalability > 4 cores | Kém (O(N²) wires) | Tốt | Rất tốt | **NoC thắng** |

## 8. Phân tích và kết luận

### 8.1 Tại sao Crossbar nhanh hơn NoC ở 4 cores?

Ở quy mô **4 cores**, Crossbar có lợi thế vì:
1. **Khoảng cách vật lý ngắn**: Mọi core đều cách memory controller chỉ 1 hop qua switch trung tâm
2. **Không có router pipeline**: Crossbar dùng arbitration đơn giản, không cần route computation, VC allocation, switch allocation như NoC
3. **Tải thấp**: Với chỉ 4 cores, crossbar chưa bị bão hòa

### 8.2 Khi nào NoC vượt trội?

NoC (Ring/Mesh) được thiết kế cho **scalability**:
- **> 8 cores**: Crossbar wiring tăng O(N²), trở nên không khả thi về mặt timing closure và diện tích chip
- **Ring**: Wiring tăng O(N), phù hợp đến ~16 cores
- **Mesh**: Wiring tăng O(√N), phù hợp đến 64+ cores
- **High contention**: Khi nhiều nguồn traffic đồng thời, NoC phân tán áp lực qua nhiều router thay vì tập trung tại 1 arbiter

### 8.3 Ping-pong: Crossbar vs NoC

Kết quả ping-pong cho thấy sự khác biệt rõ nhất:
- **Crossbar ~205 cycles**: Không phụ thuộc cặp core nào → đồng đều, concurrent cũng không tăng
- **Ring ~357-397 cycles**: Phụ thuộc topology distance, trung bình gấp ~1.75x crossbar
- **Mesh ~350-397 cycles**: Tương đương Ring ở 4 cores (cùng hop count trung bình)
- **Concurrent test trên NoC**: Không khả thi trên Verilator do 4 cores spin-wait tạo lượng coherence traffic quá lớn qua router pipeline. Cần **FPGA** để đo concurrent latency trên NoC.

Đây là trade-off cốt lõi: NoC đánh đổi **latency per-hop** để đạt được **scalability** khi hệ thống mở rộng.

### 8.4 Kết luận

| Tiêu chí | Khuyến nghị |
|----------|-------------|
| Hệ thống ≤ 4 cores, latency-sensitive | **Crossbar** (TLXbar) |
| Hệ thống 8-16 cores | **Ring NoC** (BidirectionalTorus1D) |
| Hệ thống 16+ cores, many-core | **Mesh NoC** (Mesh2D) |
| Compute-bound workloads (MatMul) | Cả 3 tương đương |
| Communication-heavy workloads | Crossbar tốt hơn ở ≤4 cores, NoC tốt hơn khi scale |

Kết quả benchmark xác nhận rằng ở quy mô 4 cores, cả 3 kiến trúc đều cho hiệu năng compute tương đương (MatMul 3.9x speedup). Sự khác biệt chỉ thể hiện ở communication latency (ping-pong) và memory bandwidth overhead (~2-6%). NoC là lựa chọn đúng đắn khi hướng đến thiết kế many-core SoC có khả năng mở rộng.

## 9. Cách chạy lại benchmark

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
    TIMEOUT_CYCLES=500000000 run-binary'
```

### Chạy trên CI

Push lên bất kỳ branch nào → GitHub Actions tự động:
1. Build libgloss + firmware (4 ELF files)
2. Generate Verilog + build Verilator simulator cho 3 configs (song song)
3. Chạy 4 benchmarks × 3 configs = 12 test runs (song song, mỗi job có 6h timeout riêng)
4. Upload kết quả (artifacts, lưu 30 ngày)

CI workflow: `.github/workflows/ci.yml`
Kết quả download từ: **GitHub Actions → Artifacts → `result-{config}-{benchmark}-{sha}`**

### Lưu ý khi chạy Verilator

- **Ping-pong benchmark** rất chậm trên NoC configs (Ring/Mesh) do spin-wait tạo nhiều coherence traffic
- Concurrent ping-pong (4 cores) trên NoC **không khả thi trên Verilator** — cần chạy trên FPGA
- Khuyến nghị `TIMEOUT_CYCLES=500000000` (500M cycles) cho tất cả benchmark
- Không nên chạy nhiều Verilator simulation song song trên cùng máy — chia sẻ CPU sẽ làm chậm đáng kể
