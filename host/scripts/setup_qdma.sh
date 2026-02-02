#!/bin/bash
#
# QDMA Driver Setup Script for VPK120
# This script loads the Xilinx QDMA driver and configures queues
#

set -e

# Configuration
QDMA_DRIVER_PATH="/opt/xilinx/qdma"
NUM_QUEUES=16
MODE="mm"  # mm or st

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

echo_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

echo_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo_error "Please run as root (sudo)"
    exit 1
fi

# Check if QDMA driver is installed
if [ ! -d "$QDMA_DRIVER_PATH" ]; then
    echo_error "QDMA driver not found at $QDMA_DRIVER_PATH"
    echo "Please install Xilinx QDMA driver from:"
    echo "  https://github.com/Xilinx/dma_ip_drivers"
    exit 1
fi

# Detect VPK120 PCIe device
echo_info "Detecting VPK120 PCIe device..."
PCIE_BDF=$(lspci -d 10ee: | head -1 | awk '{print $1}')

if [ -z "$PCIE_BDF" ]; then
    echo_error "No Xilinx PCIe device found"
    echo "Make sure:"
    echo "  1. VPK120 is powered on"
    echo "  2. PCIe cable is connected"
    echo "  3. Bitstream with CPM/QDMA is loaded"
    exit 1
fi

echo_info "Found Xilinx device at $PCIE_BDF"

# Convert BDF to full format
FULL_BDF=$(echo $PCIE_BDF | sed 's/://')
echo_info "Full BDF: $FULL_BDF"

# Unload existing driver if loaded
if lsmod | grep -q qdma; then
    echo_info "Unloading existing QDMA driver..."
    rmmod qdma_pf 2>/dev/null || true
    rmmod qdma_vf 2>/dev/null || true
fi

# Load QDMA driver
echo_info "Loading QDMA driver..."
cd $QDMA_DRIVER_PATH/linux-kernel/bin

if [ -f qdma_pf.ko ]; then
    insmod qdma_pf.ko
else
    echo_error "QDMA driver module not found. Please build the driver first."
    echo "  cd $QDMA_DRIVER_PATH/linux-kernel"
    echo "  make"
    exit 1
fi

# Wait for driver to initialize
sleep 2

# Check if driver loaded successfully
if ! lsmod | grep -q qdma_pf; then
    echo_error "Failed to load QDMA driver"
    exit 1
fi

echo_info "QDMA driver loaded successfully"

# Configure queues using dma-ctl
DMA_CTL="$QDMA_DRIVER_PATH/linux-kernel/tools/dma-ctl"

if [ ! -x "$DMA_CTL" ]; then
    echo_warn "dma-ctl not found, trying system path..."
    DMA_CTL=$(which dma-ctl 2>/dev/null || echo "")
    if [ -z "$DMA_CTL" ]; then
        echo_error "dma-ctl utility not found"
        exit 1
    fi
fi

# Get device name
DEVICE=$(ls /dev/qdma* 2>/dev/null | head -1 | sed 's/.*\(qdma[0-9]*\).*/\1/' || echo "")
if [ -z "$DEVICE" ]; then
    # Try to construct device name from BDF
    DEVICE="qdma${FULL_BDF}"
fi

echo_info "Configuring queues for $DEVICE..."

# Add queues
for ((q=0; q<$NUM_QUEUES; q++)); do
    echo_info "Adding queue $q..."
    $DMA_CTL $DEVICE q add idx $q mode $MODE dir bi || {
        echo_warn "Failed to add queue $q"
    }
done

# Start queues
for ((q=0; q<$NUM_QUEUES; q++)); do
    echo_info "Starting queue $q..."
    $DMA_CTL $DEVICE q start idx $q || {
        echo_warn "Failed to start queue $q"
    }
done

# Verify setup
echo ""
echo_info "QDMA Setup Complete!"
echo ""
echo "Device Information:"
echo "-------------------"
lspci -vvv -s $PCIE_BDF | head -20
echo ""

echo "Queue Status:"
echo "-------------"
$DMA_CTL $DEVICE q list 2>/dev/null || echo "Unable to list queues"
echo ""

echo "Device Nodes:"
echo "-------------"
ls -la /dev/qdma* 2>/dev/null || echo "No QDMA device nodes found"
echo ""

echo_info "You can now run the QDMA benchmark:"
echo "  ./qdma_benchmark -d 0x${FULL_BDF} -s 1048576 -i 100"
