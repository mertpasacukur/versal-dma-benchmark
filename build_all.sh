#!/bin/bash
################################################################################
# Versal DMA Benchmark - Complete Build Script
# Target: VPK120 (Versal Premium VP1202)
################################################################################

set -e

echo "============================================================"
echo "       VERSAL DMA BENCHMARK - COMPLETE BUILD"
echo "============================================================"
echo ""

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${SCRIPT_DIR}"

# Find Vivado/Vitis installation
find_xilinx_tools() {
    local tool=$1
    local var_name=$2

    # Check environment variable first
    if [ -n "${!var_name}" ]; then
        echo "${!var_name}"
        return
    fi

    # Search common locations
    for version in 2024.1 2023.2 2023.1 2022.2; do
        for base in /opt/Xilinx /tools/Xilinx ~/Xilinx /c/Xilinx "C:/Xilinx"; do
            if [ -d "${base}/${tool}/${version}" ]; then
                echo "${base}/${tool}/${version}"
                return
            fi
        done
    done

    echo ""
}

VIVADO_PATH=$(find_xilinx_tools "Vivado" "XILINX_VIVADO")
VITIS_PATH=$(find_xilinx_tools "Vitis" "XILINX_VITIS")

if [ -z "$VIVADO_PATH" ]; then
    echo "ERROR: Vivado installation not found!"
    echo "Please set XILINX_VIVADO environment variable"
    exit 1
fi

if [ -z "$VITIS_PATH" ]; then
    echo "ERROR: Vitis installation not found!"
    echo "Please set XILINX_VITIS environment variable"
    exit 1
fi

VIVADO_CMD="${VIVADO_PATH}/bin/vivado"
XSCT_CMD="${VITIS_PATH}/bin/xsct"

echo "Vivado:  ${VIVADO_PATH}"
echo "Vitis:   ${VITIS_PATH}"
echo "Project: ${PROJECT_ROOT}"
echo ""

################################################################################
# Step 1: Vivado Build
################################################################################

echo "============================================================"
echo "  STEP 1: VIVADO BUILD"
echo "============================================================"
echo ""

VIVADO_SCRIPT="${PROJECT_ROOT}/vivado/scripts/build_project.tcl"

if [ ! -f "$VIVADO_SCRIPT" ]; then
    echo "ERROR: Vivado build script not found: $VIVADO_SCRIPT"
    exit 1
fi

echo "Running Vivado build script..."
echo "This may take 30-60 minutes depending on your system."
echo ""

"$VIVADO_CMD" -mode batch -source "$VIVADO_SCRIPT" -notrace

echo ""
echo "Vivado build completed successfully!"
echo ""

# Verify XSA was generated
XSA_FILE="${PROJECT_ROOT}/vivado/output/xsa/dma_benchmark.xsa"
if [ ! -f "$XSA_FILE" ]; then
    echo "ERROR: XSA file not found: $XSA_FILE"
    exit 1
fi

echo "XSA exported: $XSA_FILE"
echo ""

################################################################################
# Step 2: Vitis Platform Creation
################################################################################

echo "============================================================"
echo "  STEP 2: VITIS PLATFORM CREATION"
echo "============================================================"
echo ""

PLATFORM_SCRIPT="${PROJECT_ROOT}/vitis/scripts/create_platform.tcl"

if [ ! -f "$PLATFORM_SCRIPT" ]; then
    echo "ERROR: Platform script not found: $PLATFORM_SCRIPT"
    exit 1
fi

echo "Creating Vitis platform from XSA..."
echo ""

"$XSCT_CMD" "$PLATFORM_SCRIPT"

echo ""
echo "Platform created successfully!"
echo ""

################################################################################
# Step 3: Vitis Application Build
################################################################################

echo "============================================================"
echo "  STEP 3: VITIS APPLICATION BUILD"
echo "============================================================"
echo ""

APP_SCRIPT="${PROJECT_ROOT}/vitis/scripts/create_application.tcl"

if [ ! -f "$APP_SCRIPT" ]; then
    echo "ERROR: Application script not found: $APP_SCRIPT"
    exit 1
fi

echo "Creating and building Vitis application..."
echo ""

"$XSCT_CMD" "$APP_SCRIPT"

echo ""

################################################################################
# Build Complete
################################################################################

echo "============================================================"
echo "       BUILD COMPLETE"
echo "============================================================"
echo ""
echo "Output Files:"
echo "  XSA:      ${PROJECT_ROOT}/vivado/output/xsa/dma_benchmark.xsa"
echo "  PDI:      ${PROJECT_ROOT}/vivado/output/xsa/dma_benchmark.pdi"
echo "  ELF:      ${PROJECT_ROOT}/vitis/workspace/dma_benchmark_app/Debug/dma_benchmark_app.elf"
echo "  Reports:  ${PROJECT_ROOT}/vivado/output/versal_dma_benchmark/reports/"
echo ""
echo "To run the benchmark:"
echo "  1. Connect VPK120 to your PC via JTAG"
echo "  2. Open Vitis IDE"
echo "  3. Import workspace: ${PROJECT_ROOT}/vitis/workspace"
echo "  4. Right-click application -> Run As -> Launch on Hardware"
echo ""
echo "Or use command line:"
echo "  xsct"
echo "  connect"
echo "  target -set -filter {name =~ \"*A72*0\"}"
echo "  rst -system"
echo "  device program ${PROJECT_ROOT}/vivado/output/xsa/dma_benchmark.pdi"
echo "  dow ${PROJECT_ROOT}/vitis/workspace/dma_benchmark_app/Debug/dma_benchmark_app.elf"
echo "  con"
echo ""
echo "============================================================"
