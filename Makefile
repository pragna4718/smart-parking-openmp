# ==============================================================================
# Makefile: Parallel Smart Parking Allocation System (OpenMP)
# ==============================================================================

CXX ?= clang++
CXXFLAGS := -O2 -std=c++17 -Wall -Wextra
UNAME_S := $(shell uname -s)

# Platform-specific OpenMP configuration
ifeq ($(UNAME_S),Darwin)
    # macOS with Homebrew libomp
    BREW_LIBOMP := $(shell brew --prefix libomp 2>/dev/null || echo "/opt/homebrew/opt/libomp")
    OMP_FLAGS := -Xpreprocessor -fopenmp -I$(BREW_LIBOMP)/include
    OMP_LIBS  := -L$(BREW_LIBOMP)/lib -lomp
else
    # Linux / WSL / GCC
    OMP_FLAGS := -fopenmp
    OMP_LIBS  := -fopenmp
endif

BIN_DIR := bin
SRC_DIR := src

.PHONY: all hello clean check-env

all: check-env $(BIN_DIR)/hello_openmp

check-env:
	@mkdir -p $(BIN_DIR)

$(BIN_DIR)/hello_openmp: $(SRC_DIR)/hello_openmp.cpp | check-env
	$(CXX) $(CXXFLAGS) $(OMP_FLAGS) $< -o $@ $(OMP_LIBS)

hello: $(BIN_DIR)/hello_openmp
	@echo "Executing OpenMP smoke test..."
	@./$(BIN_DIR)/hello_openmp

clean:
	rm -rf $(BIN_DIR)
