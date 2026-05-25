# Hướng dẫn phần mềm (Software Build Guide)

## 1. Tổng quan các chương trình

Tất cả firmware nằm trong `demoriscv/software/`, chạy bare-metal trên Rocket core (M-mode, không OS).

| Chương trình | Thư mục | Mục đích | Số core |
|-------------|---------|----------|---------|
| MTHello | `MTHello/` | Kiểm tra SMP boot — mỗi hart in "Hello" | 4 |
| MatMul | `MatMul/` | Nhân ma trận song song, verify kết quả | 4 |
| Cipher | `Cipher/` | Test crypto accelerator (KLEIN, ChaCha, SHA3) | 1 |
| Benchmark | `Benchmark/` | Đo hiệu năng interconnect (4 bài test) | 1-4 |

## 2. Ánh xạ phần mềm ↔ phần cứng

### 2.1 Cơ chế boot đa lõi

```
crt0.S (libgloss)
  ├── Hart 0 → _start_main → main()        // Master core
  └── Hart 1-3 → _start_secondary → __main()  // Secondary cores
```

- `__main()` trong mỗi chương trình: enable MSI interrupt → vào vòng `wfi` (Wait For Interrupt)
- Hart 0 đánh thức các hart khác bằng cách ghi vào **CLINT MSIP** register:
  ```c
  CLINT_REG(CLINT_MSIP_BASE + hart_id * 4) = 1;  // Set MSIP pending
  ```
- Secondary hart nhận interrupt → trap handler gọi `handle_msi()` → thực thi công việc

### 2.2 UART Output (kprintf)

```
kprintf() → kputc() → amoor.w vào UART TX FIFO (0x64000000)
```

- **SiFive UART**: thanh ghi TX tại offset 0x00
  - Bit [7:0]: ký tự cần gửi
  - Bit 31: cờ FIFO đầy (1 = đầy, chờ)
- `amoor.w` (atomic OR word): ghi ký tự vào TX FIFO, đọc lại trạng thái trong 1 thao tác atomic
- **Quan trọng**: cần gọi `uart_init()` trước khi dùng kprintf — bật TX enable (`txctrl.txen = 1`)

### 2.3 Ánh xạ Cipher Software ↔ Hardware

| Hàm SW (trong `cipher.c`) | Accelerator HW | Địa chỉ base | Giao diện |
|----------------------------|----------------|---------------|-----------|
| `test_klein()` | KLEIN-64 | 0x10006000 | MMIO reg (trigger/data_a/data_b/data_c) |
| `test_chacha()` | ChaCha20 | 0x10007000 | MMIO reg |
| `test_sha3()` | SHA3-512 | 0x10008000 | MMIO reg |

Quy trình điển hình:
1. Ghi key/data vào `DATA_A`, `DATA_B`
2. Set `TRIGGER` (cs=1, we=1)
3. Đợi bit done trong `DATA_C`
4. Đọc kết quả từ `DATA_C`

Cipher project còn có **pure C implementation** (`soft_cipher/`) để so sánh kết quả HW vs SW.

### 2.4 Ánh xạ Benchmark Software ↔ Hardware

| Benchmark | Đo cái gì | HW sử dụng |
|-----------|-----------|-------------|
| `bench_mem_latency` | Latency truy cập memory (cycles/access) | mcycle CSR, CLINT MSIP, DDR3 |
| `bench_mem_bandwidth` | Throughput đọc/ghi tuần tự (bytes/cycle) | mcycle CSR, CLINT MSIP, DDR3 |
| `bench_matmul_scale` | Speedup khi tăng số core | mcycle CSR, CLINT MSIP, barrier |
| `bench_pingpong` | Latency giao tiếp core-to-core | mcycle CSR, CLINT MSIP, shared memory |

**Phương pháp đo:**
- Sử dụng CSR `mcycle` (hardware cycle counter built-in của Rocket core)
- `fence` instruction trước/sau đọc mcycle để đảm bảo memory ordering
- Kết quả in qua UART bằng `kprintf()`

## 3. Cách build firmware

### 3.1 Yêu cầu

- RISC-V GNU Toolchain (`riscv64-unknown-elf-gcc`)
- `libgloss_htif` (trong `toolchains/libgloss/`)
- Hoặc dùng Docker image: `locnguyen96/chipyard-dev:latest`

### 3.2 Build libgloss (lần đầu)

```bash
git submodule update --init --depth=1 toolchains/libgloss
source /opt/chipyard-env.sh
cd toolchains/libgloss
mkdir -p build && cd build
../configure --prefix=$RISCV/riscv64-unknown-elf --host=riscv64-unknown-elf
make -j$(nproc)
```

### 3.3 Build từng chương trình

```bash
source /opt/chipyard-env.sh

# MatMul
make -C demoriscv/software/MatMul

# Cipher
make -C demoriscv/software/Cipher

# MTHello
make -C demoriscv/software/MTHello

# Benchmark (4 ELF files)
make -C demoriscv/software/Benchmark
```

### 3.4 Sử dụng Docker

```bash
# Build tất cả benchmark
docker run --rm -v "$(pwd)":/workspace \
  -w /workspace/demoriscv/software/Benchmark \
  -e RISCV=/opt/riscv \
  locnguyen96/chipyard-dev:latest \
  make clean all
```

### 3.5 Output

Mỗi chương trình tạo ra trong thư mục `build/`:

| File | Mô tả |
|------|-------|
| `*.elf` | File thực thi RISC-V (input cho simulator/FPGA) |
| `*.bin` | Binary thuần (cho bootloader nạp vào RAM) |
| `*.dump` | Disassembly (debug) |

### 3.6 Tham số biên dịch

```makefile
CFLAGS  = -march=rv64gc    # ISA: RV64 + General + Compressed
          -mabi=lp64d       # ABI: LP64 double-float
          -O0               # Không tối ưu (đo chính xác cycle)
          -mcmodel=medany   # Addressing model cho bare-metal
          -nostdlib         # Không dùng libc chuẩn
          -nostartfiles     # Dùng crt0 tự viết (libgloss)
          -static           # Liên kết tĩnh
```

Linker script: `htif.ld` (đặt code tại 0x80000000 — DDR3 entry point)

## 4. Khác biệt giữa Verilator và FPGA

### 4.1 Verilator (Mô phỏng RTL)

| Đặc điểm | Giá trị |
|-----------|---------|
| Loại | Mô phỏng cycle-accurate trên PC |
| Tốc độ | ~1-10 KHz (rất chậm so với HW thật) |
| Nạp firmware | Qua dòng lệnh: `+binary=file.elf` |
| UART output | In trực tiếp ra stdout (qua UARTAdapter) |
| Debug | Có thể dump waveform (FST/VCD) |
| Timeout | `+max-cycles=N` (mặc định 10M) |
| Độ tin cậy | **Cycle-accurate** — kết quả mcycle đáng tin |

**Lệnh chạy:**
```bash
cd sims/verilator
make SUB_PROJECT=chipyard CONFIG=ThesisSoC \
  BINARY=path/to/firmware.elf \
  TIMEOUT_CYCLES=100000000 \
  run-binary
```

### 4.2 FPGA (Phần cứng thật)

| Đặc điểm | Giá trị |
|-----------|---------|
| Board | Xilinx VC707 (Virtex-7 XC7VX485T) |
| Tần số | 50 MHz |
| Nạp firmware | Qua SD card hoặc JTAG |
| UART output | Qua cổng serial vật lý (USB-UART) |
| Debug | JTAG hoặc LED/GPIO |
| Timeout | Không có — chạy real-time |

**Build bitstream:**
```bash
cd demoriscv
make SUB_PROJECT=vc707 bitstream  # Cần Vivado 2021.2
```

### 4.3 So sánh chi tiết

| Khía cạnh | Verilator | FPGA |
|-----------|-----------|------|
| **Firmware** | Cùng file `.elf` | Cùng file `.elf` → convert `.bin` |
| **UART địa chỉ** | 0x64000000 (giống nhau) | 0x64000000 (giống nhau) |
| **kprintf** | Output → stdout terminal | Output → serial port (minicom/screen) |
| **mcycle** | Đếm chính xác từng cycle | Đếm chính xác từng cycle |
| **Harness** | `TestHarness` (Chisel) | `VC707Harness` (Chisel → Vivado) |
| **DRAM** | DRAMSim2 (mô phỏng) | DDR3 SDRAM thật (MIG controller) |
| **Boot** | HTIF load trực tiếp vào RAM | BootROM → SD card → RAM |
| **Thời gian build** | ~30 phút (Verilator compile) | ~2-4 giờ (Vivado synthesis + P&R) |
| **Thời gian chạy** | Rất chậm (phút → giờ) | Real-time (mili-giây) |

### 4.4 Điểm chung (không cần thay đổi code)

- Cùng **source code firmware** (`.c` files)
- Cùng **địa chỉ UART** (0x64000000)
- Cùng **kprintf/kputc** cho output
- Cùng **mcycle CSR** cho đo thời gian
- Cùng **CLINT MSIP** cho inter-core communication
- Kết quả benchmark **có thể so sánh trực tiếp** giữa Verilator và FPGA (cùng cycle count)
