# Tài liệu tham chiếu: Thesis ↔ Source Code

> **Tác giả khóa luận:** Trịnh Huy Hoàng (MSSV: 21207156)
> **Tiêu đề:** Thiết kế hệ thống SoC dựa trên CPU RISC-V 64-bit đa lõi đồng nhất và Network-on-Chip tích hợp các lõi mật mã hóa nhẹ trên FPGA
> **Người hướng dẫn:** PGS.TS. Lê Đức Hùng – Trường ĐH KHTN TP.HCM
> **Năm:** 2025
> **Git commit chính:** `7eaed499` ("all changes by thhoanq")

---

## 1. Tổng quan hệ thống

### 1.1 Mô tả

Hệ thống SoC đa lõi đồng nhất RISC-V 64-bit được triển khai trên FPGA Xilinx VC707 (Virtex-7 XC7VX485T), gồm:

| Thành phần | Chi tiết |
|---|---|
| CPU | 4× Rocket Core RV64IMAFDC (RV64GC) |
| L1 Cache (mỗi tile) | I$ 16KB + D$ 16KB |
| Kết nối nội bộ | NoC Ring Topology (6 router) via Constellation |
| Lõi mã hóa | KLEIN-64, ChaCha20, SHA3-512 |
| Bộ nhớ | ROM 64KB + DDR3 RAM 1GB |
| Ngoại vi | UART, SPI (SD Card), GPIO (8-pin) |
| Tần số | 50 MHz |
| Công cụ tổng hợp | Xilinx Vivado 2024.2 |
| Framework thiết kế | Chipyard (Chisel/Scala → FIRRTL → Verilog) |

### 1.2 Luồng thiết kế

```
Scala Config (CustomConfigs.scala)
  → Chisel Elaboration (DigitalTop.scala)
  → FIRRTL IR → firtool (CIRCT)
  → Verilog (generators/chipyard/src/main/resources/vsrc/)
  → Vivado Synthesis → bitstream (MySystem.bit)
  → FPGA VC707
```

---

## 2. Cấu hình SoC

### 2.1 Config Scala chính

**File:** `generators/chipyard/src/main/scala/config/CustomConfigs.scala`

```scala
// Config thesis chính thức
class ThesisSoC extends Config(
  new WithoutTLMonitors ++
  new WithSHA3(address = 0x10008000) ++
  new WithChaCha(address = 0x10007000) ++
  new WithKLEIN(address = 0x10006000) ++
  new WithNoUART ++
  new WithNoScratchpads ++
  new WithNBigCores(4) ++        // 4 Rocket cores
  new AbstractConfig
)

// Thêm NoC Ring Topology
class QuadCoreRing extends Config(
  new WithSbusNoC(SimpleTLNoCParams(
    ...
    nocParams = NoCParams(
      topology = BidirectionalTorus1D(6),               // Ring 6 nodes
      channelParamGen = (a, b) => UserChannelParams(
        Seq.fill(10) { UserVirtualChannelParams(4) }),   // 10 VCs, 4 buffers each
      routingRelation = NonblockingVirtualSubnetworksRouting(
        BidirectionalTorus1DShortestRouting(), 5, 2))   // Minimal routing
  )) ++
  new ThesisSoC
)
```

**Các config khác có trong repo:**
- `CustomSoC` – dùng AES + SHA3 + ChaCha (KLEIN bị comment)
- `GCDTLBlackBoxRocketConfig` – 1 core, dùng cho test
- `QuadCoreMesh` – topology Mesh 2D 3×2 thay vì Ring

### 2.2 DigitalTop

**File:** `generators/chipyard/src/main/scala/DigitalTop.scala`

Sử dụng cake pattern mixin – mỗi cipher trait được mix vào DigitalTop để đăng ký peripheral trên PBUS.

---

## 3. Lõi mật mã hóa nhẹ

### 3.1 Kiến trúc tích hợp chung

Mọi lõi mã hóa đều tuân theo kiến trúc chuẩn:

```
Verilog RTL (vsrc/)
  → Verilog wrapper (chuẩn hóa interface)
  → Scala BlackBox wrapper (kết nối Chisel)
  → TileLink RegisterNode (TL-UL, trên PBUS)
  → CPU access qua MMIO (load/store)
```

**Cấu trúc register chung (offset từ base address):**

| Offset | Tên thanh ghi | Bit | Mô tả |
|---|---|---|---|
| `0x0000` | Control | [16]=iReset, [8]=iWe, [0]=iCs | Điều khiển reset/write-enable/chip-select |
| `0x0004` | Address | [7:0]=iAddress | Địa chỉ thanh ghi nội bộ của lõi |
| `0x0008` | Write_Data | [31:0] | Dữ liệu ghi vào lõi |
| `0x000C` | Read_Data | [31:0] | Đọc kết quả từ lõi |

### 3.2 KLEIN-64

**Thuật toán:** Block cipher, SPN, 12 vòng, key/plaintext/ciphertext 64-bit

| Mục | Chi tiết |
|---|---|
| Base address | `0x10006000` |
| Scala config | `generators/chipyard/src/main/scala/cipher/klein.scala` |
| Verilog RTL | `generators/chipyard/src/main/resources/vsrc/KLEIN/` |
| Firmware header | `demoriscv/software/Cipher/include/cipher/accel_klein.h` |
| Test header | `tests/accel_dmpresent.h`, `tests/klein64.h` |

**Địa chỉ thanh ghi nội bộ (iAddress):**

| iAddress | Tên | Mô tả |
|---|---|---|
| `0x00` | CTRL | Điều khiển lõi |
| `0x01` | CONF | Cấu hình (encrypt/decrypt) |
| `0x02` | STATUS | Trạng thái |
| `0x10` | KEY0 | Key [63:32] |
| `0x11` | KEY1 | Key [31:0] |
| `0x20` | BLOCK0 | Plaintext [63:32] |
| `0x21` | BLOCK1 | Plaintext [31:0] |
| `0x30` | RESULT0 | Ciphertext [63:32] |
| `0x31` | RESULT1 | Ciphertext [31:0] |

**Test vector (firmware):**
- Plaintext: `0xdeadbeeff000000f`
- Key: `0x1234567890abcdef`
- Ciphertext: `0x5a14d67698fa7e4c`

**Hiệu năng (hw vs sw):**
- HW mã hóa: 494 cycles vs SW: 11,477 cycles → **tăng tốc 23×**
- HW giải mã: 701 cycles vs SW: 14,225 cycles → **tăng tốc 20×**

**Verilog RTL files:**
```
klein.v, klein_core.v, klein_cipher.v, klein_decipher.v,
klein_keyschedule.v, klein_mixcolumn.v, klein_sbox.v
```

### 3.3 ChaCha20

**Thuật toán:** Stream cipher, ARX (Add-Rotate-XOR), 20 vòng, key 256-bit, nonce 64-bit, counter 64-bit

| Mục | Chi tiết |
|---|---|
| Base address | `0x10007000` |
| Scala config | `generators/chipyard/src/main/scala/cipher/chacha.scala` |
| Verilog RTL | `generators/chipyard/src/main/resources/vsrc/chacha/` |
| Firmware header | `demoriscv/software/Cipher/include/cipher/accel_chacha.h` |

**Địa chỉ thanh ghi nội bộ:**

| iAddress | Mô tả |
|---|---|
| `0x08` | CTRL |
| `0x09` | STATUS |
| `0x0a` | KEYLEN (0=128-bit, 1=256-bit) |
| `0x0b` | ROUNDS |
| `0x10–0x17` | KEY (8 × 32-bit words) |
| `0x20–0x21` | IV (nonce) |
| `0x40–0x4F` | DATA_IN (16 × 32-bit) |
| `0x80–0x8F` | DATA_OUT (16 × 32-bit) |

**Hiệu năng:**
- HW: 426 cycles vs SW: 21,091 cycles → **tăng tốc ~50×**

**Verilog RTL files:**
```
chacha.v, chacha_core.v, chacha_qr.v
```

### 3.4 SHA3-512

**Thuật toán:** Hash function, Sponge construction, Keccak-f[1600], rate=576-bit, output=512-bit

| Mục | Chi tiết |
|---|---|
| Base address | `0x10008000` |
| Scala config | `generators/chipyard/src/main/scala/cipher/sha3.scala` |
| Verilog RTL | `generators/chipyard/src/main/resources/vsrc/sha3/` |
| Firmware header | `demoriscv/software/Cipher/include/cipher/accel_sha3.h` |

**Địa chỉ thanh ghi nội bộ:**

| iAddress | Mô tả |
|---|---|
| `0x00` | RESET |
| `0x01` | DATA_IN (32-bit word, ghi từng word) |
| `0x02` | BYTE_NUM (số byte trong word cuối) |
| `0x03` | IN_LAST (báo hiệu word cuối cùng) |
| `0x0f` | STATUS |
| `0x10–0x1F` | DIGEST (16 × 32-bit = 512-bit) |

**Test case:**
- Input: `"The quick brown fox jumps over the lazy dog"`
- Output: SHA3-512 hash 512-bit (trùng với reference value)

**Hiệu năng:**
- HW: 6,311 cycles vs SW: 200,612 cycles → **tăng tốc ~32×**

**Verilog RTL files:**
```
keccak.v, keccak_wrapper.v, f_permutation.v, round.v,
padder.v, padder1.v, rconst.v
```

### 3.5 Các lõi khác có trong repo (không phải thesis chính)

| Lõi | Scala | Verilog | Trạng thái |
|---|---|---|---|
| AES | `cipher/aes.scala` | `vsrc/aes/` | Có trong `CustomSoC`, bị comment trong `ThesisSoC` |
| ASCON | `cipher/ascon.scala` | `vsrc/ascon/` | Chỉ có trong `GCDTLBlackBoxRocketConfig` |
| BLAKE2S | `cipher/blake2s.scala` | `vsrc/Blake2s/` | Commented out |
| PRESENT | `cipher/present.scala` | `vsrc/present_dmpresent/` | Commented out |
| DMPresent | `cipher/dmpresent.scala` | `vsrc/present_dmpresent/` | Commented out |
| PRINCE | `cipher/prince.scala` | `vsrc/prince/` | Commented out |
| POLY1305 | `cipher/poly1305.scala` | `vsrc/poly1305/` | Commented out |
| MyTimer | `cipher/mytimer.scala` | `vsrc/mytimer/` | Commented out |
| ROM | `cipher/rom.scala` | `vsrc/rom_8x32.v` | Utility |

---

## 4. Network-on-Chip (NoC)

**Tool:** Constellation (submodule trong Chipyard)

### 4.1 Tham số thiết kế

| Tham số | Giá trị |
|---|---|
| Topology | Ring (BidirectionalTorus1D, 6 nodes) |
| Thuật toán định tuyến | Minimal (BidirectionalTorus1DShortestRouting) |
| Chiến lược chuyển mạch | Wormhole switching |
| Điều khiển luồng | Credit-based |
| Kênh ảo/kênh vật lý | 10 VCs |
| Buffers/VC | 4 |
| Giao thức | TileLink (TL-UL cho PBus, TL-C cho còn lại) |

### 4.2 Node mapping

| Node | ID |
|---|---|
| Core 0 (Rocket Tile 0) | 0 |
| Core 1 (Rocket Tile 1) | 1 |
| Core 2 (Rocket Tile 2) | 2 |
| Core 3 (Rocket Tile 3) | 3 |
| debug[0] / PBus | 4 |
| Memory Bus (MBus) | 5 |

### 4.3 Tài nguyên NoC

| Thành phần | LUT | FF |
|---|---|---|
| NoC (Constellation) | ~15,000 | ~9,000 |
| MBus (standard bus) | ~2,100 | ~1,300 |
| PBus (standard bus) | ~900 | ~700 |

> NoC dùng nhiều hơn ~5× LUT và ~12× FF so với bus thông thường – đánh đổi để đạt low-latency.

### 4.4 Hiệu năng NoC (nhân ma trận)

| Kích thước | Đơn lõi (cycles) | 4 lõi (cycles) | Speedup |
|---|---|---|---|
| 8×8 | 39,474 | 12,918 | **3.06×** |
| 16×16 | 286,986 | 76,066 | **3.77×** |
| 32×32 | 2,215,466 | 562,785 | **3.94×** |
| 64×64 | 20,601,024 | 5,319,524 | **3.87×** |

---

## 5. Phần mềm / Firmware

### 5.1 Cấu trúc thư mục

```
demoriscv/
├── Makefile
└── software/
    ├── Cipher/          ← Kiểm tra lõi mã hóa
    │   ├── cipher.c     ← main()
    │   ├── kprintf.c/h  ← UART printf
    │   ├── include/
    │   │   ├── cipher/  ← accel_klein.h, accel_chacha.h, accel_sha3.h, ...
    │   │   ├── soft_cipher/ ← software-only implementations
    │   │   ├── devices/ ← clint.h, gpio.h, plic.h, spi.h, uart.h
    │   │   ├── mmio.h   ← reg_read32/reg_write32 macros
    │   │   └── platform.h ← base addresses
    │   └── build/       ← cipher.elf, cipher.bin, cipher.dump
    ├── MTHello/         ← Kiểm tra multi-hart boot
    │   ├── crt0.S       ← Startup assembly
    │   └── build/       ← mt-hello.elf
    └── MatMul/          ← Nhân ma trận song song
```

### 5.2 Bootloader / Startup (`crt0.S`)

**File:** `demoriscv/software/MTHello/crt0.S` (và tương tự cho các firmware khác)

Luồng boot:
1. **BootROM** → khởi tạo UART, SPI, GPIO
2. **SD Card** → tìm phân vùng GPT, đọc firmware
3. **Copy to RAM** → tại `0x80000000`
4. **Execute from RAM** → `_prog_start` → `main(hartid, argv)`

```asm
_prog_start:
    csrr  s0, mhartid
    beqz  s0, _setup      // Hart 0 khởi tạo UART
    li    t0, 0x8
    csrw  mie, t0          // Enable MSIE cho hart 1,2,3
    wfi                    // Ngủ chờ interrupt
    j     _entry
```

**Tham số stack:**
- `STACK_SIZE = 0x800` (2KB mỗi hart)
- `STACK_SHIFT = 11`
- Stack hart_id: `SCRATCH_TOP - (hartid+1) * STACK_SIZE`

### 5.3 Memory Layout

| Địa chỉ | Vùng nhớ |
|---|---|
| `0x80000000` | `.text` (code) |
| tiếp theo | `.tdata`, `.tbss` (Thread Local Storage) |
| tiếp theo | `.rodata`, `.preinit_array`, `.init_array`, `.fini_array` |
| tiếp theo | `.htif` (Berkeley HTIF syscalls) |
| tiếp theo | `.data`, `.sdata` |
| tiếp theo | `.bss` |
| tiếp theo | Heap (128 KB, align 4KB) |
| tiếp theo | Stack Hart 0 (boot hart, tối thiểu 24KB + TCB) |
| tiếp theo | Stack Hart 1, 2, 3 |

### 5.4 MMIO helper macros

**File:** `demoriscv/software/Cipher/include/mmio.h`

```c
#define reg_write32(addr, data)  (*(volatile uint32_t *)(addr) = (data))
#define reg_read32(addr)         (*(volatile uint32_t *)(addr))
```

### 5.5 Firmware Cipher (`demoriscv/software/Cipher/cipher.c`)

Main gọi:
- `klein_test()` – test encrypt/decrypt với test vector
- `chacha_test_cases()` – test 8-round 128-bit và 20-round 256-bit
- `sha3_test_cases()` – test hash "The quick brown fox..."

Commented out (performance measurement):
- `klein_test_encryption_elapsed()` / `klein_test_decryption_elapsed()`
- `chacha_test_elapsed()`
- `sha3_test_elapsed()`

### 5.6 Chương trình nhân ma trận song song

**Chiến lược:** Row-wise partitioning

```
4 hart cùng chạy:
  Hart 0: MatMul(C[0..p/4-1],    A[0..p/4-1],    B)
  Hart 1: MatMul(C[p/4..p/2-1],  A[p/4..p/2-1],  B)
  Hart 2: MatMul(C[p/2..3p/4-1], A[p/2..3p/4-1], B)
  Hart 3: MatMul(C[3p/4..p-1],   A[3p/4..p-1],   B)
```

Không có race condition vì:
- A và B chỉ đọc (read-only)
- Mỗi hart ghi vào vùng C riêng biệt

---

## 6. Sơ đồ bộ nhớ ngoại vi (Memory Map)

| Địa chỉ cơ sở | Thiết bị |
|---|---|
| `0x02000000` | CLINT (MSIP hart 0–3 tại `+0x0`, `+0x4`, `+0x8`, `+0xC`) |
| `0x10006000` | KLEIN-64 accelerator |
| `0x10007000` | ChaCha accelerator (trong `ThesisSoC`); SHA3 trong `CustomSoC` |
| `0x10008000` | SHA3-512 accelerator (trong `ThesisSoC`); AES trong `CustomSoC` |
| `0x1000E000` | MyTimer (trong một số config) |
| `0x80000000` | DDR3 RAM (1GB) – nơi load firmware |

---

## 7. Tài nguyên FPGA (VC707)

### 7.1 Tổng thể

| Tài nguyên | Sử dụng |
|---|---|
| LUT | 147,996 |
| FF | 83,858 |
| BRAM | 192 |
| DSP | 96 |
| Công suất tổng | 3.414 W |

### 7.2 Breakdown theo thành phần

| Thành phần | LUT | FF | BRAM | DSP | Công suất |
|---|---|---|---|---|---|
| Multicore System | 131,518 | 68,733 | 192 | 96 | 0.643 W |
| Rocket Tile (×1) | 25,035 | 12,536 | 48 | 24 | 0.133 W |
| – CPU | 5,211 | 2,011 | 0 | 13 | |
| – FPU | 12,234 | 3,760 | 0 | 11 | |
| – I$ Cache | 4,048 | 3,919 | 12 | 0 | |
| – D$ Cache | 2,753 | 2,280 | 36 | 0 | |
| Interconnect (NoC+Bus) | 21,126 | 12,047 | 0 | 0 | 0.051 W |
| KLEIN-64 | 887 | 835 | 0 | 0 | 0.009 W |
| ChaCha | 2,825 | 2,005 | 0 | 0 | 0.007 W |
| SHA3-512 | 3,524 | 2,335 | 0 | 0 | 0.027 W |
| Memory IP | 16,473 | 14,970 | 0 | 0 | 2.381 W |

> Memory IP chiếm 70% công suất – phần còn lại rất tiết kiệm năng lượng.

---

## 8. Cấu trúc source code quan trọng

### 8.1 Scala/Chisel

```
generators/chipyard/src/main/scala/
├── DigitalTop.scala                    ← SoC top-level (mixin tất cả cipher traits)
├── config/
│   ├── AbstractConfig.scala            ← Base config
│   ├── CustomConfigs.scala             ← ThesisSoC, QuadCoreRing, QuadCoreMesh
│   └── ...
├── cipher/
│   ├── klein.scala    ← KLEIN BlackBox + TileLink wrapper
│   ├── chacha.scala   ← ChaCha BlackBox + TileLink wrapper
│   ├── sha3.scala     ← SHA3 BlackBox + TileLink wrapper
│   ├── aes.scala, ascon.scala, blake2s.scala, ...
│   └── mytimer.scala
├── harness/
│   └── HarnessBinders.scala            ← FPGA IO binders
└── iobinders/
    └── IOBinders.scala
```

### 8.2 Verilog RTL

```
generators/chipyard/src/main/resources/vsrc/
├── KLEIN/          ← klein.v, klein_core.v, klein_cipher.v, ...
├── chacha/         ← chacha.v, chacha_core.v, chacha_qr.v
├── sha3/           ← keccak.v, keccak_wrapper.v, f_permutation.v, ...
├── aes/
├── ascon/
├── Blake2s/
├── poly1305/
├── present_dmpresent/
├── prince/
└── mytimer/
```

### 8.3 Tests (Chipyard standard tests)

```
tests/
├── accel_aes.h, accel_blake2s.h, accel_chacha.h
├── accel_dmpresent.h, accel_klein.h, accel_present.h
├── accel_prince.h, accel_sha3.h
├── customsystem.c    ← SoC system test
├── elapsedsystem.c   ← Performance measurement
├── test.c, dataset.h ← Generic test
├── mt-matmul.c       ← Matrix multiply test
├── mytimer.h, util.h
└── klein64.h, kleinSbox.h
```

---

## 9. Các điểm nối thesis ↔ source code quan trọng

| Nội dung Thesis | File/Location trong code |
|---|---|
| §3.2 Kiến trúc hệ thống (Hình 3.2) | `CustomConfigs.scala:ThesisSoC`, `QuadCoreRing` |
| §3.3 Rocket Core RV64GC | Config `WithNBigCores(4)` → `rocket-chip/` |
| §3.4.2 CLINT/MSIP (Bảng 3.1) | `include/devices/clint.h`, `crt0.S` (csrw mie) |
| §3.4.2 PLIC | `include/devices/plic.h` |
| §3.5.1 KLEIN-64 | `cipher/klein.scala`, `vsrc/KLEIN/`, `accel_klein.h` |
| §3.5.2 ChaCha | `cipher/chacha.scala`, `vsrc/chacha/`, `accel_chacha.h` |
| §3.5.3 SHA3-512 | `cipher/sha3.scala`, `vsrc/sha3/`, `accel_sha3.h` |
| §3.6 NoC Ring Topology | `QuadCoreRing` config, `BidirectionalTorus1D(6)` |
| §3.7 Matrix Multiplication | `demoriscv/software/MatMul/`, `tests/mt-matmul.c` |
| §3.8 Memory Layout | `demoriscv/software/Cipher/include/platform.h` |
| §4.1 Chipyard structure (Hình 4.1) | `DigitalTop.scala`, `HarnessBinders.scala`, `IOBinders.scala` |
| §4.2 TileLink MMIO integration | `cipher/*.scala` – pattern: `TLRegisterNode` + `node.regmap()` |
| §4.3 Constellation NoC | `CustomConfigs.scala:QuadCoreRing` – `WithSbusNoC(...)` |
| §4.4.1 Bootloader (Hình 4.4) | `demoriscv/software/MTHello/crt0.S`, `demoriscv/Makefile` |
| §4.4.2 SoC system test | `demoriscv/software/MTHello/` |
| §4.4.3 Cipher test | `demoriscv/software/Cipher/cipher.c` |
| §4.4.4 Matrix multiply | `demoriscv/software/MatMul/` |
| §5.1 Test vectors KLEIN | `accel_klein.h:klein_test()` (deadbeef/1234567890abcdef) |
| §5.1 Test vectors ChaCha | `accel_chacha.h:chacha_test_cases()` |
| §5.1 Test vectors SHA3 | `accel_sha3.h:sha3_test_cases()` (quick brown fox) |
| §5.4 Bảng thông số (Bảng 5.4) | `CustomConfigs.scala:QuadCoreRing` tổng hợp |

---

## 10. Tóm tắt kết quả nghiên cứu

| Chỉ tiêu | Kết quả |
|---|---|
| Tần số hoạt động | 50 MHz trên VC707 |
| Tăng tốc mã hóa KLEIN | 23× (hw vs sw) |
| Tăng tốc ChaCha20 | ~50× (hw vs sw) |
| Tăng tốc SHA3-512 | ~32× (hw vs sw) |
| Tăng tốc nhân ma trận (64×64) | 3.87× (4 core vs 1 core) |
| Tổng LUT | 147,996 / 485,760 (~30%) |
| Tổng công suất | 3.414 W (70% từ Memory IP) |

---

## 11. Hướng phát triển (từ thesis §6.2)

- Thêm CPU dị nhất (BOOM, CVA6) vào SoC
- Mở rộng many-core (>4 lõi)
- Private L1 cache per core qua RoCC interface
- Shared coherent L2 cache
- Đánh giá NoC độc lập (có/không có NoC)
- Tối ưu năng lượng cho IoT
- Triển khai ASIC (hiện chỉ FPGA)
