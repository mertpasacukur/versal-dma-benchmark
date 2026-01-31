# Vivado Design Outputs

This folder contains the Vivado build script and output files for the Versal DMA Benchmark design.

## Files

| File | Description |
|------|-------------|
| `build_all_dma_v3.tcl` | Vivado TCL script to build the design (recommended) |
| `design_all_dma_v3.xsa` | Hardware platform export (use with Vitis) |
| `design_all_dma_v3.pdi` | Programmable Device Image (for JTAG programming) |

## v3 Design Improvements

1. **LPD DMA (ADMA) NoC Path**: Added M_AXI_LPD interface to allow ADMA access to LPDDR4 via NoC
2. **AXI DMA 26-bit Length**: Increased buffer length from 14-bit (16KB max) to 26-bit (64MB max)
3. **AXI CDMA Fix**: Reduced burst size from 256 to 64 for better NoC compatibility

## Building the Design

1. Open terminal with Vivado in PATH
2. Navigate to the Vivado scripts directory:
   ```bash
   cd vivado/scripts
   ```
3. Run Vivado in batch mode:
   ```bash
   vivado -mode batch -source build_all_dma_v2.tcl
   ```
4. Build takes approximately 30-45 minutes
5. Outputs will be in `vivado/export/` directory

## Design Components

- **CIPS**: Versal PS with LPD DMA enabled, M_AXI_FPD and M_AXI_LPD interfaces
- **AXI NoC**: Network-on-Chip with LPDDR4 memory controller
- **AXI CDMA**: Memory-to-memory DMA (Scatter-Gather mode)
- **AXI DMA**: Stream DMA with MM2S/S2MM loopback (26-bit length)
- **AXI MCDMA**: Multi-channel DMA with 4 channels

## Target Board

- **Board**: VPK120 (Versal Premium VP1202)
- **Memory**: 2GB LPDDR4 on-board
- **Tools**: Vivado 2023.2
