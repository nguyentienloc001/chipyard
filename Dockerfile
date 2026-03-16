##############################################################################
# Chipyard Docker Image — Optimized Multi-Stage Build
#
# Workflow: generation (Chisel/FIRRTL) → firtool → Verilator sim → compile
#
# Forces linux/amd64 platform for full compatibility with RISC-V toolchain
# ecosystem (firtool, pre-built toolchains, etc.).
# On Apple Silicon: runs via Rosetta 2 in Docker Desktop.
#
# Usage:
#   docker build --platform linux/amd64 -t chipyard .
#   docker run --platform linux/amd64 -it --rm -v $(pwd):/workspace chipyard
#
# Inside container:
#   cd sims/verilator && make CONFIG=RocketConfig
##############################################################################

# ═══════════════════════════════════════════════════════════════════════════
# Stage 1: Builder — heavy build tools, compile everything, then discard
# ═══════════════════════════════════════════════════════════════════════════
FROM --platform=linux/amd64 ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-c"]

# All build + runtime deps together (this stage is thrown away)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential gcc g++ \
    autoconf automake autotools-dev libtool \
    curl wget ca-certificates gnupg \
    git \
    python3 \
    device-tree-compiler \
    libmpc-dev libmpfr-dev libgmp-dev \
    gawk bison flex texinfo gperf \
    bc unzip \
    pkg-config libexpat1-dev zlib1g-dev libfl-dev \
    help2man perl make \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

# ── 1. Verilator 5.022 ───────────────────────────────────────────────────
ARG VERILATOR_VERSION=5.022
RUN cd /tmp && \
    curl -fsSL "https://github.com/verilator/verilator/archive/refs/tags/v${VERILATOR_VERSION}.tar.gz" \
        | tar xz && \
    cd verilator-${VERILATOR_VERSION} && \
    autoconf && \
    ./configure --prefix=/opt/verilator && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/verilator-*

# ── 2. firtool / CIRCT — extract only firtool binary ─────────────────────
ARG FIRTOOL_VERSION=1.75.0
RUN cd /tmp && \
    curl -fsSL "https://github.com/llvm/circt/releases/download/firtool-${FIRTOOL_VERSION}/circt-full-shared-linux-x64.tar.gz" \
        -o circt.tar.gz && \
    mkdir circt-extract && tar xzf circt.tar.gz -C circt-extract --strip-components=1 && \
    mkdir -p /opt/firtool/bin /opt/firtool/lib && \
    cp circt-extract/bin/firtool /opt/firtool/bin/ && \
    cp -a circt-extract/lib/*.so* /opt/firtool/lib/ 2>/dev/null || true && \
    rm -rf /tmp/circt*

# ── 3. RISC-V toolchain (pre-built) ──────────────────────────────────────
ENV RISCV=/opt/riscv
ARG RISCV_TOOLCHAIN_TAG=2026.03.13
RUN mkdir -p $RISCV && \
    cd /tmp && \
    curl -fsSL "https://github.com/riscv-collab/riscv-gnu-toolchain/releases/download/${RISCV_TOOLCHAIN_TAG}/riscv64-elf-ubuntu-22.04-gcc.tar.xz" \
        -o riscv-toolchain.tar.xz && \
    tar xJf riscv-toolchain.tar.xz -C $RISCV --strip-components=1 && \
    rm -f /tmp/riscv-toolchain.tar.xz && \
    # Strip debug symbols from toolchain (saves ~500MB+)
    find $RISCV/bin -type f -executable -exec strip --strip-debug {} + 2>/dev/null || true && \
    find $RISCV/libexec -type f -executable -exec strip --strip-debug {} + 2>/dev/null || true && \
    find $RISCV/lib/gcc -name "*.a" -exec strip --strip-debug {} + 2>/dev/null || true && \
    # Remove docs, man pages, info
    rm -rf $RISCV/share/doc $RISCV/share/man $RISCV/share/info $RISCV/share/locale

ENV PATH="$RISCV/bin:$PATH"

# ── 4. Build Spike (riscv-isa-sim) ───────────────────────────────────────
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

# ── 5. Build RISC-V proxy kernel (pk) ────────────────────────────────────
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

# ── 6. Build riscv-tests ─────────────────────────────────────────────────
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

# ── 7. Build DRAMSim2 shared library ─────────────────────────────────────
RUN cd /tmp && \
    git clone --depth 1 https://github.com/dramninjasUMD/DRAMSim2.git && \
    cd DRAMSim2 && \
    make libdramsim.so && \
    cp libdramsim.so $RISCV/lib/ && \
    cd / && rm -rf /tmp/DRAMSim2

# Final cleanup in builder: strip built binaries
RUN strip --strip-debug $RISCV/bin/spike 2>/dev/null || true && \
    strip --strip-debug $RISCV/lib/libriscv.so 2>/dev/null || true && \
    strip --strip-debug $RISCV/lib/libdramsim.so 2>/dev/null || true && \
    find $RISCV/lib -name "*.a" -exec strip --strip-debug {} + 2>/dev/null || true

# ═══════════════════════════════════════════════════════════════════════════
# Stage 2: Final — minimal runtime image, no build tools
# ═══════════════════════════════════════════════════════════════════════════
FROM --platform=linux/amd64 ubuntu:22.04 AS final

ENV DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-c"]

# Only runtime dependencies — no autoconf, texinfo, gperf, help2man, etc.
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
    && rm -rf /var/lib/apt/lists/*

# sbt
RUN echo "deb https://repo.scala-sbt.org/scalasbt/debian all main" > /etc/apt/sources.list.d/sbt.list && \
    curl -sL "https://keyserver.ubuntu.com/pks/lookup?op=get&search=0x2EE0EA64E40A89B84B2DF73499E82A75642AC823" | apt-key add - && \
    apt-get update && apt-get install -y --no-install-recommends sbt && \
    rm -rf /var/lib/apt/lists/*

# Copy only what we need from builder (single layer, no duplication)
ENV RISCV=/opt/riscv
COPY --from=builder /opt/riscv /opt/riscv
COPY --from=builder /opt/verilator /opt/verilator
COPY --from=builder /opt/firtool /opt/firtool

ENV PATH="$RISCV/bin:/opt/verilator/bin:/opt/firtool/bin:/usr/local/bin:$PATH"
ENV LD_LIBRARY_PATH="$RISCV/lib:/opt/firtool/lib"

# Verify tools
RUN firtool --version && verilator --version && spike --help 2>&1 | head -1

# Environment setup script
RUN printf '%s\n' \
    '#!/bin/bash' \
    'export RISCV=/opt/riscv' \
    'export PATH=$RISCV/bin:/opt/verilator/bin:/opt/firtool/bin:/usr/local/bin:$PATH' \
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
    'echo "[OK] RISCV=$RISCV"' \
    > /opt/chipyard-env.sh && \
    chmod +x /opt/chipyard-env.sh && \
    echo 'source /opt/chipyard-env.sh' >> /etc/bash.bashrc

WORKDIR /workspace

ENTRYPOINT ["/bin/bash", "-c", "source /opt/chipyard-env.sh && exec \"$@\"", "--"]
CMD ["/bin/bash"]
