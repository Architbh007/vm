# Compiler and flags
CXX = mpic++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -I./include

# Directories
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include

# Source files
SEQ_SRC = $(SRC_DIR)/sequential_dijkstra.cpp
DIST_SRC = $(SRC_DIR)/distributed_dijkstra.cpp
GEN_SRC = $(SRC_DIR)/graph_generator.cpp

# Executables
SEQ_BIN = $(BUILD_DIR)/sequential
DIST_BIN = $(BUILD_DIR)/distributed
GEN_BIN = $(BUILD_DIR)/generator

# Targets
all: $(SEQ_BIN) $(DIST_BIN) $(GEN_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(SEQ_BIN): $(SEQ_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SEQ_SRC) -o $(SEQ_BIN) -lm

$(DIST_BIN): $(DIST_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(DIST_SRC) -o $(DIST_BIN) -lm

$(GEN_BIN): $(GEN_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(GEN_SRC) -o $(GEN_BIN) -lm

clean:
	rm -rf $(BUILD_DIR)/*

.PHONY: all clean
