# syntax=docker/dockerfile:1
##############################################################################
# Chipyard Docker Image — Multi-Stage, Multi-Arch (amd64 + arm64)
#
# Stage 1 (toolchain): Build RISC-V GNU toolchain from source
# Stage 2 (builder):   Build Verilator, firtool, Spike, pk, tests, DRAMSim2
# Stage 3 (final):     Minimal runtime with pre-built artifacts + Vivado
#
# Pre-requisite: run docker/install-vivado.sh to install Vivado to vivado-install/
#
# Build:
#   docker buildx build --platform linux/amd64 -t locnguyen96/chipyard-dev .
##############################################################################

# ═══════════════════════════════════════════════════════════════════════════
# Stage 1: RISC-V GNU Toolchain — build from source (cached separately)
# ═══════════════════════════════════════════════════════════════════════════
FROM ubuntu:22.04 AS toolchain

ENV DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-c"]

RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf automake autotools-dev libtool \
    build-essential gcc g++ \
    curl wget ca-certificates \
    git \
    python3 \
    libmpc-dev libmpfr-dev libgmp-dev \
    gawk bison flex texinfo gperf \
    bc unzip xz-utils \
    pkg-config libexpat1-dev zlib1g-dev \
    help2man perl make \
    && rm -rf /var/lib/apt/lists/*

ENV RISCV=/opt/riscv
ARG RISCV_TOOLCHAIN_TAG=2024.09.03
RUN cd /tmp && \
    git clone --depth 1 --branch ${RISCV_TOOLCHAIN_TAG} \
        https://github.com/riscv-collab/riscv-gnu-toolchain.git && \
    cd riscv-gnu-toolchain && \
    ./configure --prefix=$RISCV --with-arch=rv64gc --with-abi=lp64d && \
    make -j$(nproc) && \
    cd / && rm -rf /tmp/riscv-gnu-toolchain && \
    # Strip debug symbols to reduce size
    find $RISCV/bin -type f -executable -exec strip --strip-debug {} + 2>/dev/null || true && \
    find $RISCV/libexec -type f -executable -exec strip --strip-debug {} + 2>/dev/null || true && \
    find $RISCV/lib/gcc -name "*.a" -exec strip --strip-debug {} + 2>/dev/null || true && \
    rm -rf $RISCV/share/doc $RISCV/share/man $RISCV/share/info $RISCV/share/locale

# ═══════════════════════════════════════════════════════════════════════════
# Stage 2: Builder — Verilator, firtool, Spike, pk, tests, DRAMSim2
# ═══════════════════════════════════════════════════════════════════════════
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-c"]

RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf automake autotools-dev libtool \
    build-essential gcc g++ \
    curl wget ca-certificates \
    git \
    python3 \
    device-tree-compiler \
    libmpc-dev libmpfr-dev libgmp-dev \
    gawk bison flex texinfo gperf \
    bc unzip xz-utils \
    pkg-config libexpat1-dev zlib1g-dev libfl-dev \
    help2man perl make \
    && rm -rf /var/lib/apt/lists/*

# Copy toolchain from stage 1
ENV RISCV=/opt/riscv
COPY --from=toolchain /opt/riscv /opt/riscv
ENV PATH="$RISCV/bin:$PATH"

# ── 1. Verilator 5.022 ───────────────────────────────────────────────────
ARG VERILATOR_VERSION=5.022
RUN cd /tmp && \
    curl -fsSL "https://github.com/verilator/verilator/archive/v${VERILATOR_VERSION}.tar.gz" \
        | tar xz && \
    cd verilator-${VERILATOR_VERSION} && \
    autoconf && \
    ./configure --prefix=/opt/verilator && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/verilator-*

# ── 2. firtool / CIRCT ────────────────────────────────────────────────────
#   amd64: download pre-built binary
#   arm64: build from source (no pre-built arm64 release exists)
ARG FIRTOOL_VERSION=1.75.0
ARG TARGETARCH
RUN cd /tmp && \
    mkdir -p /opt/firtool/bin /opt/firtool/lib && \
    if [ "$TARGETARCH" = "amd64" ] || [ "$TARGETARCH" = "x86_64" ]; then \
        curl -fsSL "https://github.com/llvm/circt/releases/download/firtool-${FIRTOOL_VERSION}/circt-full-shared-linux-x64.tar.gz" \
            -o circt.tar.gz && \
        mkdir circt-extract && tar xzf circt.tar.gz -C circt-extract --strip-components=1 && \
        cp circt-extract/bin/firtool /opt/firtool/bin/ && \
        cp -a circt-extract/lib/*.so* /opt/firtool/lib/ 2>/dev/null || true && \
        rm -rf /tmp/circt* ; \
    else \
        apt-get update && apt-get install -y --no-install-recommends \
            cmake ninja-build clang lld python3-dev && \
        rm -rf /var/lib/apt/lists/* && \
        curl -fsSL "https://github.com/llvm/circt/releases/download/firtool-${FIRTOOL_VERSION}/circt-full-sources.tar.gz" \
            -o circt-src.tar.gz && \
        mkdir circt-src && tar xzf circt-src.tar.gz -C circt-src --strip-components=1 && \
        cd circt-src && \
        mkdir build && cd build && \
        cmake -G Ninja ../llvm/llvm \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_C_COMPILER=clang \
            -DCMAKE_CXX_COMPILER=clang++ \
            -DCMAKE_INSTALL_PREFIX=/opt/firtool \
            -DLLVM_ENABLE_PROJECTS=mlir \
            -DLLVM_EXTERNAL_PROJECTS=circt \
            -DLLVM_EXTERNAL_CIRCT_SOURCE_DIR=/tmp/circt-src \
            -DLLVM_TARGETS_TO_BUILD=host \
            -DLLVM_USE_LINKER=lld \
            -DLLVM_PARALLEL_LINK_JOBS=2 \
            -DBUILD_SHARED_LIBS=OFF \
            -DLLVM_BUILD_TOOLS=OFF \
            -DLLVM_BUILD_UTILS=OFF \
            -DCIRCT_RELEASE_TAG_ENABLED=ON \
            -DCIRCT_RELEASE_TAG=firtool-${FIRTOOL_VERSION} && \
        ninja -j$(nproc) firtool && \
        strip bin/firtool && \
        cp bin/firtool /opt/firtool/bin/ && \
        cd / && rm -rf /tmp/circt* ; \
    fi

# ── 3. Build Spike (riscv-isa-sim) ───────────────────────────────────────
ARG SPIKE_COMMIT=824ecdf6dc06ad0560001741ef1db861d4ed069f
RUN cd /tmp && \
    git clone --depth 1 https://github.com/riscv-software-src/riscv-isa-sim.git && \
    cd riscv-isa-sim && \
    git fetch --depth 1 origin $SPIKE_COMMIT && \
    git checkout $SPIKE_COMMIT && \
    mkdir build && cd build && \
    ../configure --prefix=$RISCV \
        --with-boost=no --with-boost-asio=no --with-boost-regex=no && \
    make -j$(nproc) && \
    make install && \
    make libfesvr.a && \
    cp libfesvr.a $RISCV/lib/ && \
    cd / && rm -rf /tmp/riscv-isa-sim

# ── 4. Build RISC-V proxy kernel (pk) ────────────────────────────────────
ARG PK_COMMIT=abadfdc507d5a75b6272dc360e70a80a510c758a
RUN cd /tmp && \
    git clone --depth 1 https://github.com/riscv-software-src/riscv-pk.git && \
    cd riscv-pk && \
    git fetch --depth 1 origin $PK_COMMIT && \
    git checkout $PK_COMMIT && \
    mkdir build && cd build && \
    ../configure --prefix=$RISCV --host=riscv64-unknown-elf --with-arch=rv64gc_zifencei && \
    make -j$(nproc) && \
    make install && \
    cd / && rm -rf /tmp/riscv-pk

# ── 5. Build riscv-tests ─────────────────────────────────────────────────
ARG TESTS_COMMIT=51de00886cd28a3cf9b85ee306fb2b5ee5ab550e
RUN cd /tmp && \
    git clone --depth 1 https://github.com/riscv-software-src/riscv-tests.git && \
    cd riscv-tests && \
    git fetch --depth 1 origin $TESTS_COMMIT && \
    git checkout $TESTS_COMMIT && \
    git submodule update --init --recursive && \
    autoconf && \
    mkdir build && cd build && \
    ../configure --prefix=$RISCV/riscv64-unknown-elf --with-xlen=64 && \
    (make -j$(nproc) || true) && \
    (make install || true) && \
    cd / && rm -rf /tmp/riscv-tests

# ── 6. Build libgloss-htif (HTIF linker script + library) ────────────────
RUN cd /tmp && \
    git clone --depth 1 https://github.com/ucb-bar/libgloss-htif.git && \
    cd libgloss-htif && \
    mkdir build && cd build && \
    ../configure --prefix=$RISCV/riscv64-unknown-elf --host=riscv64-unknown-elf && \
    make -j$(nproc) && \
    make install && \
    cd / && rm -rf /tmp/libgloss-htif

# ── 7. Build DRAMSim2 shared library ─────────────────────────────────────
RUN cd /tmp && \
    git clone --depth 1 https://github.com/dramninjasUMD/DRAMSim2.git && \
    cd DRAMSim2 && \
    make libdramsim.so && \
    cp libdramsim.so $RISCV/lib/ && \
    cd / && rm -rf /tmp/DRAMSim2

# Final cleanup: strip built binaries
RUN strip --strip-debug $RISCV/bin/spike 2>/dev/null || true && \
    strip --strip-debug $RISCV/lib/libriscv.so 2>/dev/null || true && \
    strip --strip-debug $RISCV/lib/libdramsim.so 2>/dev/null || true && \
    find $RISCV/lib -name "*.a" -exec strip --strip-debug {} + 2>/dev/null || true

# ═══════════════════════════════════════════════════════════════════════════
# Stage 3: Final — minimal runtime, no build tools
#
# Vivado pre-requisite: run docker/install-vivado.sh BEFORE building.
# That script installs Vivado to vivado-install/ on the host.  Only the
# ~15 GB installed result is COPYed here — the 72 GB installer tarball
# never enters the Docker build context.
# ═══════════════════════════════════════════════════════════════════════════
FROM ubuntu:22.04 AS final

ENV DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-c"]

# Runtime dependencies (includes Vivado runtime libs)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gcc g++ \
    curl ca-certificates gnupg \
    git \
    python3 python3-pip \
    device-tree-compiler \
    libmpc-dev libmpfr-dev libgmp-dev \
    zlib1g-dev libfl-dev libexpat1-dev \
    pkg-config \
    perl make \
    default-jdk \
    jq \
    locales libncurses5 libtinfo5 \
    libx11-6 libxext6 libxrender1 libxtst6 libxi6 \
    libfreetype6 fontconfig \
    && locale-gen en_US.UTF-8 \
    && rm -rf /var/lib/apt/lists/*

# sbt (Scala Build Tool)
RUN echo "deb https://repo.scala-sbt.org/scalasbt/debian all main" > /etc/apt/sources.list.d/sbt.list && \
    curl -sL "https://keyserver.ubuntu.com/pks/lookup?op=get&search=0x2EE0EA64E40A89B84B2DF73499E82A75642AC823" | apt-key add - && \
    apt-get update && apt-get install -y --no-install-recommends sbt && \
    rm -rf /var/lib/apt/lists/*

# Copy pre-built artifacts from builder
ENV RISCV=/opt/riscv
COPY --from=builder /opt/riscv /opt/riscv
COPY --from=builder /opt/verilator /opt/verilator
COPY --from=builder /opt/firtool /opt/firtool

# Copy pre-installed Vivado (run docker/install-vivado.sh first to populate vivado-install/)
COPY vivado-install/ /opt/Xilinx/

ENV XILINX_VIVADO=/opt/Xilinx/Vivado/2021.2
ENV PATH="$RISCV/bin:/opt/verilator/bin:/opt/firtool/bin:${XILINX_VIVADO}/bin:/usr/local/bin:$PATH"
ENV LD_LIBRARY_PATH="$RISCV/lib:/opt/firtool/lib"
# License: provide at runtime via -e XILINXD_LICENSE_FILE=<port>@<host>

# libudev stub — replaces Ubuntu 22.04 libudev.so.1 which crashes when called
# from Vivado 2021.2's FLEXlm license manager (realloc invalid pointer / SIGABRT).
# Must be done AFTER all apt-get installs since apt/curl depend on the real libudev.
COPY docker/libudev_stub.c /tmp/libudev_stub.c
RUN gcc -shared -fPIC -Wl,-soname,libudev.so.1 \
        -o /lib/x86_64-linux-gnu/libudev.so.1.7.2 /tmp/libudev_stub.c && \
    rm /tmp/libudev_stub.c

# Verify tools are working
RUN firtool --version && verilator --version && spike --help 2>&1 | head -1

# Environment setup script
RUN printf '%s\n' \
    '#!/bin/bash' \
    'export RISCV=/opt/riscv' \
    'export XILINX_VIVADO=/opt/Xilinx/Vivado/2021.2' \
    'export PATH=$RISCV/bin:/opt/verilator/bin:/opt/firtool/bin:${XILINX_VIVADO}/bin:/usr/local/bin:$PATH' \
    'export LD_LIBRARY_PATH=$RISCV/lib:/opt/firtool/lib:${LD_LIBRARY_PATH:-}' \
    'export JAVA_HEAP_SIZE=${JAVA_HEAP_SIZE:-8G}' \
    'export JAVA_TOOL_OPTIONS="-Xmx${JAVA_HEAP_SIZE} -Xss8M -Djava.io.tmpdir=/tmp"' \
    'export USE_CHISEL6=1' \
    '' \
    'command -v riscv64-unknown-elf-gcc >/dev/null && echo "[OK] riscv64-unknown-elf-gcc $(riscv64-unknown-elf-gcc --version | head -1)" || echo "[MISSING] riscv64-unknown-elf-gcc"' \
    'command -v verilator >/dev/null && echo "[OK] verilator $(verilator --version 2>&1 | head -1)" || echo "[MISSING] verilator"' \
    'command -v firtool >/dev/null && echo "[OK] firtool $(firtool --version 2>&1 | head -1)" || echo "[MISSING] firtool"' \
    'command -v sbt >/dev/null && echo "[OK] sbt" || echo "[MISSING] sbt"' \
    'command -v spike >/dev/null && echo "[OK] spike" || echo "[MISSING] spike"' \
    'command -v vivado >/dev/null && echo "[OK] vivado $(vivado -version 2>&1 | head -1)" || echo "[MISSING] vivado"' \
    'echo "[OK] RISCV=$RISCV"' \
    > /opt/chipyard-env.sh && \
    chmod +x /opt/chipyard-env.sh && \
    echo 'source /opt/chipyard-env.sh' >> /etc/bash.bashrc

# Labels
ARG BUILD_DATE
ARG VCS_REF
LABEL org.opencontainers.image.title="chipyard-dev" \
      org.opencontainers.image.description="Chipyard RISC-V SoC development environment with Vivado 2021.2" \
      org.opencontainers.image.created="${BUILD_DATE}" \
      org.opencontainers.image.revision="${VCS_REF}" \
      org.opencontainers.image.source="https://github.com/nguyentienloc001/chipyard"

WORKDIR /workspace

ENTRYPOINT ["/bin/bash", "-c", "source /opt/chipyard-env.sh && exec \"$@\"", "--"]
CMD ["/bin/bash"]
