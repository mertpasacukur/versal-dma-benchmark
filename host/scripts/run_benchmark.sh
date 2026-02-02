#!/bin/bash
#
# QDMA Benchmark Runner Script
# Runs comprehensive QDMA benchmarks on VPK120
#

set -e

# Configuration
BENCHMARK_APP="../qdma_test/qdma_benchmark"
OUTPUT_DIR="./results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_FILE="${OUTPUT_DIR}/qdma_benchmark_${TIMESTAMP}.csv"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

echo_section() {
    echo ""
    echo -e "${YELLOW}========================================${NC}"
    echo -e "${YELLOW}$1${NC}"
    echo -e "${YELLOW}========================================${NC}"
    echo ""
}

# Create output directory
mkdir -p $OUTPUT_DIR

# Check if benchmark app exists
if [ ! -x "$BENCHMARK_APP" ]; then
    echo "Building benchmark application..."
    cd ../qdma_test
    make
    cd -
fi

if [ ! -x "$BENCHMARK_APP" ]; then
    echo "ERROR: Benchmark application not found"
    exit 1
fi

# Detect device
PCIE_BDF=$(lspci -d 10ee: | head -1 | awk '{print $1}' | sed 's/://')
if [ -z "$PCIE_BDF" ]; then
    echo "ERROR: No Xilinx PCIe device found"
    exit 1
fi

BDF_HEX="0x${PCIE_BDF}"
echo_info "Using device at BDF: $BDF_HEX"

# Start CSV file with header
echo "test_name,transfer_size,iterations,h2c_gbps,c2h_gbps,latency_us,errors" > $OUTPUT_FILE

echo_section "QDMA Benchmark Suite"
echo "Output file: $OUTPUT_FILE"
echo "Device: $BDF_HEX"
echo ""

# Transfer size sweep test
echo_section "Transfer Size Sweep Test"

SIZES="64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608"

for SIZE in $SIZES; do
    echo_info "Testing size: $SIZE bytes"
    $BENCHMARK_APP -d $BDF_HEX -s $SIZE -i 100 -o $OUTPUT_FILE 2>/dev/null || {
        echo "  Warning: Test with size $SIZE failed"
    }
done

# Iteration count test (for statistical significance)
echo_section "Statistical Test (1MB, varying iterations)"

ITERATIONS="10 50 100 500 1000"
SIZE=1048576

for ITER in $ITERATIONS; do
    echo_info "Testing with $ITER iterations"
    $BENCHMARK_APP -d $BDF_HEX -s $SIZE -i $ITER -o $OUTPUT_FILE 2>/dev/null || true
done

# Data verification test
echo_section "Data Verification Test"

echo_info "Running with data verification enabled..."
$BENCHMARK_APP -d $BDF_HEX -s 1048576 -i 100 -v -o $OUTPUT_FILE 2>/dev/null || {
    echo "  Data verification test failed"
}

# Large transfer test
echo_section "Large Transfer Test"

LARGE_SIZES="16777216 33554432 67108864"

for SIZE in $LARGE_SIZES; do
    SIZE_MB=$((SIZE / 1048576))
    echo_info "Testing ${SIZE_MB}MB transfer"
    $BENCHMARK_APP -d $BDF_HEX -s $SIZE -i 10 -o $OUTPUT_FILE 2>/dev/null || {
        echo "  Warning: Large transfer test failed"
    }
done

# Summary
echo_section "Benchmark Complete"

echo "Results saved to: $OUTPUT_FILE"
echo ""
echo "Summary:"
echo "--------"

if [ -f "$OUTPUT_FILE" ]; then
    # Extract best results
    echo "Best H2C Throughput: $(tail -n +2 $OUTPUT_FILE | cut -d',' -f4 | sort -rn | head -1) GB/s"
    echo "Best C2H Throughput: $(tail -n +2 $OUTPUT_FILE | cut -d',' -f5 | sort -rn | head -1) GB/s"
    echo "Lowest Latency:      $(tail -n +2 $OUTPUT_FILE | cut -d',' -f6 | sort -n | head -1) us"
    echo ""
    echo "Total test runs: $(tail -n +2 $OUTPUT_FILE | wc -l)"
fi

echo ""
echo_info "Benchmark suite completed!"
