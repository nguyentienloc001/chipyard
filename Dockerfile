##############################################################################
# Chipyard Docker Image
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

FROM --platform=linux/amd64 ubuntu:22.04 AS base

ENV DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-c"]

# ── 1. System dependencies ──────────────────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gcc g++ \
    autoconf automake autotools-dev libtool \
    curl wget ca-certificates gnupg \
    git \
    python3 python3-pip \
    device-tree-compiler \
    libmpc-dev libmpfr-dev libgmp-dev \
    gawk bison flex texinfo gperf \
    bc unzip \
    pkg-config libexpat1-dev zlib1g-dev libfl-dev \
    help2man perl make \
    default-jdk \
    jq \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

# ── 2. sbt (Scala Build Tool) ───────────────────────────────────────────────
RUN echo "deb https://repo.scala-sbt.org/scalasbt/debian all main" > /etc/apt/sources.list.d/sbt.list && \
    curl -sL "https://keyserver.ubuntu.com/pks/lookup?op=get&search=0x2EE0EA64E40A89B84B2DF73499E82A75642AC823" | apt-key add - && \
    apt-get update && apt-get install -y --no-install-recommends sbt && \
    rm -rf /var/lib/apt/lists/*

# ── 3. Verilator 5.022 (build from source — exact version required) ─────────
ARG VERILATOR_VERSION=5.022
RUN cd /tmp && \
    curl -fsSL "https://github.com/verilator/verilator/archive/refs/tags/v${VERILATOR_VERSION}.tar.gz" \
        | tar xz && \
    cd verilator-${VERILATOR_VERSION} && \
    autoconf && \
    ./configure --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    cd / && rm -rf /tmp/verilator-*

# ── 4. firtool / CIRCT (prebuilt shared binary, native x86_64) ──────────────
ARG FIRTOOL_VERSION=1.75.0
RUN cd /tmp && \
    curl -fsSL "https://github.com/llvm/circt/releases/download/firtool-${FIRTOOL_VERSION}/circt-full-shared-linux-x64.tar.gz" \
        -o circt.tar.gz && \
    mkdir circt-extract && tar xzf circt.tar.gz -C circt-extract --strip-components=1 && \
    cp -a circt-extract/bin/* /usr/local/bin/ && \
    cp -a circt-extract/lib/* /usr/local/lib/ 2>/dev/null || true && \
    ldconfig && \
    rm -rf /tmp/circt* && \
    firtool --version

# ── 5. RISC-V bare-metal toolchain (pre-built from riscv-collab) ────────────
# Complete toolchain with newlib, needed for pk/tests/libgloss builds
ENV RISCV=/opt/riscv
ARG RISCV_TOOLCHAIN_TAG=2026.03.13
RUN mkdir -p $RISCV && \
    cd /tmp && \
    curl -fsSL "https://github.com/riscv-collab/riscv-gnu-toolchain/releases/download/${RISCV_TOOLCHAIN_TAG}/riscv64-elf-ubuntu-22.04-gcc.tar.xz" \
        -o riscv-toolchain.tar.xz && \
    tar xJf riscv-toolchain.tar.xz -C $RISCV --strip-components=1 && \
    rm -f /tmp/riscv-toolchain.tar.xz && \
    $RISCV/bin/riscv64-unknown-elf-gcc --version | head -1

ENV PATH="$RISCV/bin:$PATH"

# ── 6. Build Spike (riscv-isa-sim) → provides libriscv, libfesvr, spike ─────
FROM base AS spike-builder

ENV RISCV=/opt/riscv
ENV PATH="$RISCV/bin:$PATH"

ARG SPIKE_COMMIT=824ecdf6dc06ad0560001741ef1db861d4ed069f
RUN cd /tmp && \
    git clone https://github.com/riscv-software-src/riscv-isa-sim.git && \
    cd riscv-isa-sim && \
    git checkout $SPIKE_COMMIT && \
    mkdir build && cd build && \
    ../configure --prefix=$RISCV \
        --with-boost=no --with-boost-asio=no --with-boost-regex=no && \
    make -j$(nproc) && \
    make install && \
    make libfesvr.a && \
    cp libfesvr.a $RISCV/lib/ && \
    cd / && rm -rf /tmp/riscv-isa-sim

# ── 7. Build RISC-V proxy kernel (pk) ───────────────────────────────────────
ARG PK_COMMIT=abadfdc507d5a75b6272dc360e70a80a510c758a
RUN cd /tmp && \
    git clone https://github.com/riscv-software-src/riscv-pk.git && \
    cd riscv-pk && \
    git checkout $PK_COMMIT && \
    mkdir build && cd build && \
    ../configure --prefix=$RISCV --host=riscv64-unknown-elf --with-arch=rv64gc_zifencei && \
    make -j$(nproc) && \
    make install && \
    cd / && rm -rf /tmp/riscv-pk

# ── 8. Build riscv-tests ────────────────────────────────────────────────────
ARG TESTS_COMMIT=51de00886cd28a3cf9b85ee306fb2b5ee5ab550e
RUN cd /tmp && \
    git clone https://github.com/riscv-software-src/riscv-tests.git && \
    cd riscv-tests && \
    git checkout $TESTS_COMMIT && \
    git submodule update --init --recursive && \
    autoconf && \
    mkdir build && cd build && \
    ../configure --prefix=$RISCV/riscv64-unknown-elf --with-xlen=64 && \
    (make -j$(nproc) || true) && \
    (make install || true) && \
    cd / && rm -rf /tmp/riscv-tests

# ── 9. Build DRAMSim2 shared library ────────────────────────────────────────
RUN cd /tmp && \
    git clone https://github.com/dramninjasUMD/DRAMSim2.git && \
    cd DRAMSim2 && \
    make libdramsim.so && \
    cp libdramsim.so $RISCV/lib/ && \
    cd / && rm -rf /tmp/DRAMSim2

# ── 10. Final image ─────────────────────────────────────────────────────────
FROM base AS final

ENV RISCV=/opt/riscv
ENV PATH="$RISCV/bin:/usr/local/bin:$PATH"

# Copy all built RISC-V tools from builder stage (spike, pk, tests, dramsim)
COPY --from=spike-builder /opt/riscv /opt/riscv

# ── 11. Environment setup script ────────────────────────────────────────────
RUN printf '%s\n' \
    '#!/bin/bash' \
    'export RISCV=/opt/riscv' \
    'export PATH=$RISCV/bin:/usr/local/bin:$PATH' \
    'export LD_LIBRARY_PATH=$RISCV/lib:/usr/local/lib:${LD_LIBRARY_PATH:-}' \
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
