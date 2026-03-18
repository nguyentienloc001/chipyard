#############################################################################
# Chipyard Top-Level Makefile
#
# Convenience targets wrapping Docker and simulation workflows.
# For the full chipyard build system, see common.mk and variables.mk.
#############################################################################

DOCKER_DIR := docker
IMAGE      := locnguyen96/chipyard-dev:latest
CONFIG      ?= RocketConfig
SW_PROGRAMS ?= MTHello Cipher MatMul

.PHONY: help docker-build docker-shell \
        build-sw generate verilator sim clean

help:
	@echo "Chipyard Makefile targets:"
	@echo ""
	@echo "  Docker:"
	@echo "    docker-build   Build Docker image (multi-arch)"
	@echo "    docker-shell   Interactive shell via docker run"
	@echo ""
	@echo "  Build (runs inside Docker):"
	@echo "    build-sw       Build all software (Cipher, MatMul, MTHello)"
	@echo "    generate       Generate Verilog (CONFIG=RocketConfig)"
	@echo "    verilator      Build Verilator simulator"
	@echo "    sim BINARY=x   Run simulation with a binary"
	@echo ""
	@echo "  Misc:"
	@echo "    clean          Remove all generated outputs (HW + SW)"
	@echo ""
	@echo "  Override CONFIG:  make generate CONFIG=MyConfig"

# ── Docker ────────────────────────────────────────────────────────────────

docker-build:
	$(DOCKER_DIR)/build.sh

docker-shell:
	$(DOCKER_DIR)/docker-run.sh shell

# ── Build & Simulation (inside Docker) ────────────────────────────────────

build-sw:
	docker run --rm -v "$$(pwd):/workspace" -w /workspace $(IMAGE) \
		bash -c 'source /opt/chipyard-env.sh 2>/dev/null && \
		for d in $(SW_PROGRAMS); do \
			echo "=== $$d ===" && make -C demoriscv/software/$$d clean && \
			make -C demoriscv/software/$$d || exit 1; done'

generate:
	$(DOCKER_DIR)/docker-run.sh generate $(CONFIG)

verilator:
	$(DOCKER_DIR)/docker-run.sh verilator $(CONFIG)

sim:
	$(DOCKER_DIR)/docker-run.sh sim $(CONFIG) $(BINARY)

# ── Cleanup ───────────────────────────────────────────────────────────────

clean:
	rm -rf sims/verilator/generated-src sims/verilator/output
	rm -f sims/verilator/simulator-*
	for d in Cipher MatMul MTHello; do rm -rf demoriscv/software/$$d/build; done
