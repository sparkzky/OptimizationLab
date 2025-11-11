# Makefile for last_work project
# Generated from xmake.lua

# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -O3 -I./include -march=native
LDFLAGS :=

# Build directories
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj

# Targets
TARGETS := matrix_multiply filelines filelines_gen blocksize_benchmark simd_perf_test mt_perf_test

# All target
.PHONY: all
all: $(TARGETS)

# matrix_multiply target
MATRIX_MULTIPLY_SRCS := src/matrix_multiply/matrix_multiply.cpp
MATRIX_MULTIPLY_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(MATRIX_MULTIPLY_SRCS))
MATRIX_MULTIPLY_CXXFLAGS := $(CXXFLAGS) -msse -mavx
MATRIX_MULTIPLY_LDFLAGS := $(LDFLAGS) -lpthread

matrix_multiply: $(MATRIX_MULTIPLY_OBJS) | $(BUILD_DIR)
	$(CXX) $(MATRIX_MULTIPLY_OBJS) -o $(BUILD_DIR)/$@ $(MATRIX_MULTIPLY_LDFLAGS)

$(OBJ_DIR)/src/matrix_multiply/%.o: src/matrix_multiply/%.cpp | $(OBJ_DIR)/src/matrix_multiply
	$(CXX) $(MATRIX_MULTIPLY_CXXFLAGS) -c $< -o $@

# filelines target
FILELINES_SRCS := src/basic_benchmark/filelines.cpp \
                src/basic_benchmark/filelines_baseline.cpp \
                src/find_most_freq.cpp \
				src/simd_benchmark/filelines_mt.cpp
FILELINES_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(FILELINES_SRCS))
FILELINES_LDFLAGS := $(LDFLAGS) -lpthread

filelines: $(FILELINES_OBJS) | $(BUILD_DIR)
	$(CXX) $(FILELINES_OBJS) -o $(BUILD_DIR)/$@ $(FILELINES_LDFLAGS)

$(OBJ_DIR)/src/basic_benchmark/%.o: src/basic_benchmark/%.cpp | $(OBJ_DIR)/src/basic_benchmark
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/src/find_most_freq.o: src/find_most_freq.cpp | $(OBJ_DIR)/src
	$(CXX) $(CXXFLAGS) -c $< -o $@

# filelines_gen target
FILELINES_GEN_SRCS := src/filelines_gen.cpp
FILELINES_GEN_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(FILELINES_GEN_SRCS))

filelines_gen: $(FILELINES_GEN_OBJS) | $(BUILD_DIR)
	$(CXX) $(FILELINES_GEN_OBJS) -o $(BUILD_DIR)/$@ $(LDFLAGS)

$(OBJ_DIR)/src/filelines_gen.o: src/filelines_gen.cpp | $(OBJ_DIR)/src
	$(CXX) $(CXXFLAGS) -c $< -o $@

# blocksize_benchmark target
BLOCKSIZE_BENCHMARK_SRCS := src/blocksize_benchmark/blocksize_benchmark.cpp \
                            src/blocksize_benchmark/filelines_blocksize.cpp \
                            src/find_most_freq.cpp
BLOCKSIZE_BENCHMARK_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(BLOCKSIZE_BENCHMARK_SRCS))

blocksize_benchmark: $(BLOCKSIZE_BENCHMARK_OBJS) | $(BUILD_DIR)
	$(CXX) $(BLOCKSIZE_BENCHMARK_OBJS) -o $(BUILD_DIR)/$@ $(LDFLAGS)

$(OBJ_DIR)/src/blocksize_benchmark/%.o: src/blocksize_benchmark/%.cpp | $(OBJ_DIR)/src/blocksize_benchmark
	$(CXX) $(CXXFLAGS) -c $< -o $@

# simd_perf_test target
SIMD_PERF_TEST_SRCS := src/simd_benchmark/simd_perf_test.cpp \
                       src/basic_benchmark/filelines_baseline.cpp \
                       src/simd_benchmark/filelines_simd_opt.cpp \
                       src/find_most_freq.cpp
SIMD_PERF_TEST_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SIMD_PERF_TEST_SRCS))
SIMD_PERF_TEST_CXXFLAGS := $(CXXFLAGS) -mavx

simd_perf_test: $(SIMD_PERF_TEST_OBJS) | $(BUILD_DIR)
	$(CXX) $(SIMD_PERF_TEST_OBJS) -o $(BUILD_DIR)/$@ $(LDFLAGS)

$(OBJ_DIR)/src/simd_benchmark/%.o: src/simd_benchmark/%.cpp | $(OBJ_DIR)/src/simd_benchmark
	$(CXX) $(SIMD_PERF_TEST_CXXFLAGS) -c $< -o $@

# mt_perf_test target
MT_PERF_TEST_SRCS := src/simd_benchmark/mt_perf_test.cpp \
                     src/basic_benchmark/filelines_baseline.cpp \
                     src/simd_benchmark/filelines_simd_opt.cpp \
                     src/simd_benchmark/filelines_mt.cpp \
                     src/find_most_freq.cpp
MT_PERF_TEST_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(MT_PERF_TEST_SRCS))
MT_PERF_TEST_CXXFLAGS := $(CXXFLAGS) -mavx
MT_PERF_TEST_LDFLAGS := $(LDFLAGS) -lpthread

mt_perf_test: $(MT_PERF_TEST_OBJS) | $(BUILD_DIR)
	$(CXX) $(MT_PERF_TEST_OBJS) -o $(BUILD_DIR)/$@ $(MT_PERF_TEST_LDFLAGS)

# Special rule for simd_benchmark files in mt_perf_test (reuse simd objects)
# Note: This shares object files with simd_perf_test where possible

# Create necessary directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OBJ_DIR)/src:
	mkdir -p $(OBJ_DIR)/src

$(OBJ_DIR)/src/matrix_multiply:
	mkdir -p $(OBJ_DIR)/src/matrix_multiply

$(OBJ_DIR)/src/basic_benchmark:
	mkdir -p $(OBJ_DIR)/src/basic_benchmark

$(OBJ_DIR)/src/blocksize_benchmark:
	mkdir -p $(OBJ_DIR)/src/blocksize_benchmark

$(OBJ_DIR)/src/simd_benchmark:
	mkdir -p $(OBJ_DIR)/src/simd_benchmark

# Clean target
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# Individual clean targets
.PHONY: clean-matrix_multiply clean-filelines clean-filelines_gen clean-blocksize_benchmark clean-simd_perf_test clean-mt_perf_test
clean-matrix_multiply:
	rm -f $(BUILD_DIR)/matrix_multiply $(MATRIX_MULTIPLY_OBJS)

clean-filelines:
	rm -f $(BUILD_DIR)/filelines $(FILELINES_OBJS)

clean-filelines_gen:
	rm -f $(BUILD_DIR)/filelines_gen $(FILELINES_GEN_OBJS)

clean-blocksize_benchmark:
	rm -f $(BUILD_DIR)/blocksize_benchmark $(BLOCKSIZE_BENCHMARK_OBJS)

clean-simd_perf_test:
	rm -f $(BUILD_DIR)/simd_perf_test $(SIMD_PERF_TEST_OBJS)

clean-mt_perf_test:
	rm -f $(BUILD_DIR)/mt_perf_test $(MT_PERF_TEST_OBJS)

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all                    - Build all targets (default)"
	@echo "  matrix_multiply        - Build matrix multiplication program"
	@echo "  filelines              - Build file lines analysis program"
	@echo "  filelines_gen          - Build test file generator"
	@echo "  blocksize_benchmark    - Build block size performance test"
	@echo "  simd_perf_test         - Build SIMD performance test"
	@echo "  mt_perf_test           - Build multi-threaded SIMD performance test"
	@echo "  clean                  - Remove all build artifacts"
	@echo "  clean-<target>         - Remove specific target and its objects"
	@echo "  help                   - Show this help message"
