# Changes vs `origin/main`

Branch: `make_docker_as_muti_stage_build`

---

## 1. `docker/` — New directory (entirely new vs main)

### `docker/Dockerfile` *(new)*

The old repo had `dockerfiles/Dockerfile` — a single-stage ~65-line image with no Vivado support.
This branch replaces it with a **3-stage multi-arch build** (~293 lines).

| Stage | Base | What it builds |
|-------|------|----------------|
| `toolchain` | `ubuntu:22.04` | RISC-V GNU toolchain from source (`riscv-gnu-toolchain@2024.09.03`) |
| `builder` | `toolchain` | Verilator 5.022, firtool 1.75.0 (CIRCT), Spike 1.1.1, pk, riscv-tests, DRAMSim2 |
| `final` | `ubuntu:22.04` | Copies only compiled artifacts + Vivado; minimal runtime |

Key design decisions:
- **Multi-stage** — build dependencies (autoconf, cmake, flex, etc.) not present in final image
- **`COPY vivado-install/ /opt/Xilinx/`** — Vivado baked into image at build time (requires pre-running `install-vivado.sh`)
- **`/opt/chipyard-env.sh`** — environment script auto-sourced in container; sets `RISCV`, `XILINX_VIVADO`, `PATH`, `LD_LIBRARY_PATH`, `JAVA_TOOL_OPTIONS`
- **Strip debug symbols** on toolchain binaries to reduce image size
- Final image: **20.5 GB** uncompressed (old single-stage Vivado image was ~114 GB)

### `docker/install-vivado.sh` *(new)*

Script to install Vivado 2021.2 on the **host** into `vivado-install/` before the Docker build.
Accepts a `.tar.gz` tarball or pre-extracted directory.

Post-install pruning removes ~52 GB of unnecessary data, leaving ~12 GB for Virtex-7/VC707 only:

| Directory removed | Size | Reason |
|-------------------|------|--------|
| `data/parts/xilinx/devint/vault/versal` | ~39 GB | Versal AI timing data (not needed for Virtex-7) |
| `data/xsim` | ~3.5 GB | Vivado simulator |
| `data/secureip` | ~1.9 GB | Encrypted simulation models |
| `data/deca` | ~1.6 GB | ML timing prediction |
| `data/resource_est` | ~1.1 GB | Resource estimator ML models |
| `data/simmodels` | ~424 MB | Simulation primitives |
| `ids_lite` | ~2.6 GB | ISE Design Suite leftovers |
| `gnu/microblaze` | ~1.4 GB | MicroBlaze toolchain |

Also checks free disk space before proceeding (~75 GB needed on `/tmp` for extraction, ~25 GB on project root for install result).

### `docker/install_config.txt` *(new)*

Vivado batch install configuration. Key settings:

```
Product=Vivado
Modules=..., Virtex-7 Vivado Devices:1, Virtex-7:1, 7 Series:1, (all others :0)
EnableDiskUsageOptimization=1
```

All device families **except Virtex-7** are disabled. Vitis, HLS, DocNav, Alveo, Zynq, UltraScale all set to `0`.

### `docker/build.sh` *(new)*

Wrapper around `docker buildx build` with flags for:
- `--platform` (default: current arch; `--push` defaults to `linux/amd64,linux/arm64`)
- `--push` — build multi-arch and push to Docker Hub
- `--no-cache`, `--tag`, `--repo`

### `docker/docker-run.sh` *(new)*

Convenience script to launch the container with recommended volume mounts:
- `-v $(pwd):/workspace`
- `-v ~/.Xilinx/Xilinx.lic:/root/.Xilinx/Xilinx.lic:ro`
- `--mac-address` for node-locked license

---

## 2. `.dockerignore` *(new)*

Prevents large directories from being sent to the Docker build context:

```
vivado-install/          # copied explicitly via COPY
.git/
sims/
fpga/generated-src/
*.tar.gz
```

Without this, `docker build` would send the entire chipyard repo (~several GB) as build context.

---

## 3. `fpga/src/main/scala/vc707/Configs.scala` *(modified)*

### Removed broken configs

Four config classes were referencing types not present in this Chipyard version and caused compile errors:

```scala
// REMOVED — caused "not found" compile errors:
// class CustomVC707Config    — referenced chipyard.CustomConfig (not present)
// class TestVC707Config      — referenced chipyard.TestConfig (not present)
// class InternshipConfig     — referenced WithNCustomCores (not in rocket package)
// class QuadCoreVC707Config  — referenced chipyard.QuadCoreRing (not present)
```

### `BoomVC707Config` — BOOM variant changed

```scala
// Before (origin/main):
new chipyard.MegaBoomV3Config

// After:
new chipyard.MediumBoomV3Config
```

`MegaBoomV3Config` does not exist in this Chipyard version; `MediumBoomV3Config` is the correct type.

### Minor: import cleanup + indentation

Removed blank lines between imports; standardised 4-space indentation in config class bodies.

---

## 4. Vivado License / libudev fix *(runtime, not committed)*

Not committed to the repo but required to run the container on Ubuntu 22.04:

**Problem:** Vivado 2021.2's FLEXlm license manager (`libXil_lmgr11.so`) calls
`dlopen("libudev.so.1")` → `udev_enumerate_scan_devices()`. Ubuntu 22.04's
`libudev.so.1` crashes with `realloc(): invalid pointer` (SIGABRT) when called
from the old license manager.

**Fix:** A custom C stub (`/mnt/data/libudev_stub.so`) re-implements the udev API
by reading `/sys/class/net` directly, bypassing the crash entirely.

Deployed by mounting it over the versioned system library in `docker run`:

```bash
-v /mnt/data/libudev_stub.so:/lib/x86_64-linux-gnu/libudev.so.1.7.2:ro
```

The symlink chain `libudev.so.1 → libudev.so.1.7.2` means `dlopen("libudev.so.1")`
resolves to the stub. `LD_PRELOAD` does **not** work here because the library is
loaded via `dlopen`, not at link time.

---

## 5. CI — `.github/workflows/ci.yml` *(new)*

Added GitHub Actions workflow with change-detection: only runs FPGA/simulation
jobs when relevant files change. Key jobs removed from earlier iterations:
`build-simulator` and `run-simulation` (too slow/flaky for CI).

---

## Summary

| File | Change |
|------|--------|
| `dockerfiles/Dockerfile` (65 lines) | **Replaced** by `docker/Dockerfile` (293 lines, 3-stage) |
| `docker/install-vivado.sh` | **New** — host Vivado installer + 52 GB post-install pruning |
| `docker/install_config.txt` | **New** — Vivado batch config, Virtex-7 only |
| `docker/build.sh` | **New** — buildx wrapper |
| `docker/docker-run.sh` | **New** — convenience run script |
| `.dockerignore` | **New** — excludes large dirs from build context |
| `fpga/.../vc707/Configs.scala` | **Modified** — removed 4 broken configs, fixed BoomVC707Config |
| Docker image size | **114 GB → 20.5 GB** (82% reduction) |
| Vivado install size | **63 GB → 12 GB** (81% reduction, Virtex-7 only) |
