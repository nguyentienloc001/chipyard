# Kiến trúc phần cứng (Hardware Architecture)

## 1. Tổng quan

Hệ thống SoC được xây dựng trên nền tảng **Chipyard** (UC Berkeley), tùy biến để phục vụ nghiên cứu so sánh hiệu năng giữa các kiến trúc interconnect khác nhau trên hệ thống đa lõi RISC-V.

| Thành phần | Chi tiết |
|------------|----------|
| CPU | 4x Rocket Core RV64IMAFDC (BigCore) |
| L1 Cache | I$ 16KB + D$ 16KB mỗi tile |
| Interconnect | TLXbar (Crossbar) hoặc Constellation NoC |
| Crypto Accelerators | KLEIN-64, ChaCha20, SHA3-512 |
| Bộ nhớ | DDR3 1GB (qua TileLink) |
| Ngoại vi | UART (0x64000000), CLINT (0x02000000) |
| FPGA mục tiêu | Xilinx VC707 (Virtex-7) @ 50 MHz |

## 2. Các thay đổi so với Chipyard gốc

### 2.1 Thêm Crypto Accelerator

Tổng cộng **12 module mã hóa** được phát triển và tích hợp tại:
```
generators/chipyard/src/main/scala/cipher/
```

| Module | File | Mô tả |
|--------|------|-------|
| AES | `aes.scala` | Mã hóa AES-128/256, blackbox Verilog |
| SHA3-512 | `sha3.scala` | Hàm băm Keccak, hỗ trợ độ dài message tùy ý |
| ChaCha20 | `chacha.scala` | Stream cipher, 8/20 rounds, key 128/256-bit |
| KLEIN-64 | `klein.scala` | Block cipher nhẹ 64-bit |
| BLAKE2S | `blake2s.scala` | Hàm băm 256-bit |
| PRESENT | `present.scala` | Block cipher nhẹ 64-bit (S-box + P-box) |
| DMPresent | `dmpresent.scala` | PRESENT chống side-channel (dual-mask) |
| Prince | `prince.scala` | Ultra-lightweight cipher 64-bit |
| ASCON | `ascon.scala` | AEAD (Authenticated Encryption) |
| Poly1305 | `poly1305.scala` | MAC algorithm |
| MyTimer | `mytimer.scala` | Bộ đếm cycle tùy chỉnh |
| ROM | `rom.scala` | Bộ nhớ chỉ đọc 8x32-bit |

**Kiến trúc chung của các accelerator:**
- BlackBox Verilog → bọc bởi Chisel wrapper
- Giao tiếp qua **TileLink Register Node** (MMIO)
- Gắn vào **PBUS** (Peripheral Bus) qua `TLFragmenter`
- Giao diện thanh ghi chuẩn 4 offset:

| Offset | Tên | Chức năng |
|--------|-----|-----------|
| 0x00 | TRIGGER | Điều khiển: reset, write-enable, chip-select |
| 0x04 | DATA_A | Địa chỉ / tham số điều khiển |
| 0x08 | DATA_B | Dữ liệu ghi vào accelerator |
| 0x0C | DATA_C | Dữ liệu đọc ra (kết quả / trạng thái) |

### 2.2 Thêm cấu hình Constellation NoC

Sử dụng **Constellation** (packet-switched, wormhole-routed Network-on-Chip) thay thế TLXbar crossbar mặc định.

Các tham số NoC chung:
- **10 Virtual Channels** mỗi link, mỗi VC có **4 credits** (buffer depth)
- **NonblockingVirtualSubnetworksRouting**: 5 VC cho subnet A, 2 cho subnet B
- Hỗ trợ concurrent traffic không blocking

### 2.3 Cấu hình UART

Chipyard gốc sử dụng UART mặc định tại `0x10020000` (trong `AbstractConfig`). Dự án này thay đổi:
- `WithNoUART` → xóa UART mặc định
- `WithUART(address = 0x64000000)` → thêm UART tại địa chỉ thesis

Thứ tự trong config (đọc từ dưới lên): `AbstractConfig` → `WithNoUART` → `WithUART(0x64000000)`

### 2.4 Thêm FPGA target VC707

Thư mục `demoriscv/src/main/scala/vc707/` chứa:
- `Configs.scala` — cấu hình VC707
- `Harness.scala` — top-level wrapper cho FPGA
- `HarnessBinders.scala` — kết nối IO với FPGA pins

## 3. Ba cấu hình đối chứng

Định nghĩa tại `generators/chipyard/src/main/scala/config/CustomConfigs.scala`:

### 3.1 ThesisSoC — Crossbar Baseline

```
Interconnect: TLXbar (crossbar tập trung)
CPU:          4x Rocket BigCore RV64GC
Crypto:       SHA3 (0x10008000), ChaCha (0x10007000), KLEIN (0x10006000)
UART:         0x64000000
```

Đây là cấu hình **baseline** — sử dụng TLXbar mặc định của Chipyard. Mọi core kết nối qua 1 crossbar switch trung tâm, arbitration tuần tự.

### 3.2 QuadCoreRing — Ring NoC

```
Interconnect: Constellation NoC — BidirectionalTorus1D (6 nodes)
Topology:     Vòng 2 chiều, 6 router
Routing:      Shortest-path trên Torus1D
SoC base:     ThesisSoC (cùng CPU, crypto, UART)
```

**Node mapping:**
```
Node 0: Core 0    Node 1: Core 1
Node 2: Core 2    Node 3: Core 3
Node 4: Debug + PBUS
Node 5: Memory Controller (system[0])
```

Đặc điểm: latency phụ thuộc khoảng cách hop giữa các node. Max hop = 3 (nửa vòng ring).

### 3.3 QuadCoreMeshThesis — Mesh NoC

```
Interconnect: Constellation NoC — Mesh2D (3 cột × 2 hàng)
Topology:     Lưới 2D, 6 node
Routing:      Dimension-Ordered (XY routing)
SoC base:     ThesisSoC (cùng CPU, crypto, UART)
```

**Node layout (Mesh 3×2):**
```
[0: Core0] --- [1: Core1] --- [2: Core2]
    |               |               |
[3: Core3] --- [4: Memory] --- [5: Debug+PBUS]
```

Đặc điểm: max hop = 3 (góc-đối-góc), nhưng trung bình hop thấp hơn Ring cho nhiều cặp core.

## 4. Bản đồ địa chỉ (Memory Map)

| Địa chỉ | Thiết bị | Ghi chú |
|----------|----------|---------|
| `0x00000000` | Debug | Giao diện debug |
| `0x00010000` | BootROM | 64KB ROM khởi động |
| `0x02000000` | CLINT | Timer + Software Interrupt |
| `0x0C000000` | PLIC | Platform-Level Interrupt |
| `0x10006000` | KLEIN-64 | Crypto accelerator |
| `0x10007000` | ChaCha20 | Crypto accelerator |
| `0x10008000` | SHA3-512 | Crypto accelerator |
| `0x64000000` | UART | Serial I/O (kprintf) |
| `0x80000000` | DDR3 RAM | 1GB, firmware entry point |
