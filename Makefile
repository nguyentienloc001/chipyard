#############################################################################
# Chipyard Top-Level Makefile
#
# Convenience targets wrapping Docker and simulation workflows.
# For the full chipyard build system, see common.mk and variables.mk.
#############################################################################

DOCKER_DIR     := docker
DOCKER_COMPOSE := docker compose -f $(DOCKER_DIR)/docker-compose.yml
IMAGE          := locnguyen96/chipyard-dev:latest
CONFIG         ?= RocketConfig

.PHONY: help docker-build docker-shell docker-up docker-down \
        generate verilator sim clean-sim

help:
	@echo "Chipyard Makefile targets:"
	@echo ""
	@echo "  Docker:"
	@echo "    docker-build   Build Docker image (multi-arch)"
	@echo "    docker-shell   Interactive shell via docker run"
	@echo "    docker-up      Start docker-compose services"
	@echo "    docker-down    Stop docker-compose services"
	@echo ""
	@echo "  Simulation (runs inside Docker):"
	@echo "    generate       Generate Verilog (CONFIG=RocketConfig)"
	@echo "    verilator      Build Verilator simulator"
	@echo "    sim BINARY=x   Run simulation with a binary"
	@echo ""
	@echo "  Misc:"
	@echo "    clean-sim      Remove generated simulation outputs"
	@echo ""
	@echo "  Override CONFIG:  make generate CONFIG=MyConfig"

# ── Docker ────────────────────────────────────────────────────────────────

docker-build:
	$(DOCKER_DIR)/build.sh

docker-shell:
	$(DOCKER_DIR)/docker-run.sh shell

docker-up:
	$(DOCKER_COMPOSE) up -d

docker-down:
	$(DOCKER_COMPOSE) down

# ── Simulation (inside Docker) ────────────────────────────────────────────

generate:
	$(DOCKER_DIR)/docker-run.sh generate $(CONFIG)

verilator:
	$(DOCKER_DIR)/docker-run.sh verilator $(CONFIG)

sim:
	$(DOCKER_DIR)/docker-run.sh sim $(CONFIG) $(BINARY)

# ── Cleanup ───────────────────────────────────────────────────────────────

clean-sim:
	rm -rf sims/verilator/generated-src sims/verilator/output
	rm -f sims/verilator/simulator-*
