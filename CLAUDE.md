# Versal DMA Benchmark Project - Claude Code Configuration

## Xilinx Tools Configuration

### Vivado 2023.2
- **Path:** `C:/Xilinx_2023_2/Vivado/2023.2`
- **Executable:** `C:/Xilinx_2023_2/Vivado/2023.2/bin/vivado.bat`
- **Settings:** `C:/Xilinx_2023_2/Vivado/2023.2/settings64.bat`

### Vitis 2023.2
- **Path:** `C:/Xilinx_2023_2/Vitis/2023.2`
- **Executable:** `C:/Xilinx_2023_2/Vitis/2023.2/bin/vitis.bat`
- **XSCT:** `C:/Xilinx_2023_2/Vitis/2023.2/bin/xsct.bat`
- **Settings:** `C:/Xilinx_2023_2/Vitis/2023.2/settings64.bat`

### Vitis HLS 2023.2
- **Path:** `C:/Xilinx_2023_2/Vitis_HLS/2023.2`

## Project Structure

```
versal_dma/
├── vivado/                    # Vivado projects
│   ├── versal_all_dma/       # All DMA types design
│   ├── design_all_dma.xsa    # Latest XSA with all DMAs
│   └── export/               # Exported files
├── vitis/                     # Vitis projects
│   ├── dma_benchmark/src/    # Source code
│   └── workspace_all_dma/    # Vitis workspace
└── docs/                      # Documentation
```

## DMA Types in Design

1. **AXI CDMA** - Memory-to-memory transfers (Scatter-Gather)
2. **AXI DMA** - Stream DMA with MM2S/S2MM (Loopback FIFO)
3. **AXI MCDMA** - Multi-channel DMA (4 channels, Loopback FIFO)
4. **LPD DMA (ADMA)** - PS Hard IP DMA

## Memory Map

| Memory | Base Address | Size |
|--------|--------------|------|
| LPDDR4 | 0x0000_0000 | 2GB |
| OCM | 0xFFFC_0000 | 256KB |

## Build Commands

### Vivado Build
```bash
cd vivado && /c/Xilinx_2023_2/Vivado/2023.2/bin/vivado -mode batch -source build_script.tcl
```

### Vitis Build
```bash
cd vitis && /c/Xilinx_2023_2/Vitis/2023.2/bin/xsct build_script.tcl
```

## Notes

- Always use Xilinx 2023.2 tools for this project
- XSA and PDI files are in `vivado/export/` or `vivado/` root
- Google Drive backup: `G:/My Drive/versal_dma/`
