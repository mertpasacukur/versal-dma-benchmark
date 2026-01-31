# Versal DMA Benchmark Application

Comprehensive DMA benchmarking suite for Xilinx Versal VPK120 board.

## Supported DMA Types

| DMA Type | Description | Mode |
|----------|-------------|------|
| AXI DMA | Stream DMA with MM2S/S2MM | Scatter-Gather |
| AXI CDMA | Memory-to-Memory DMA | Scatter-Gather |
| AXI MCDMA | Multi-Channel DMA (4 channels) | Scatter-Gather |
| LPD DMA (ADMA) | PS Hard IP DMA (8 channels) | Descriptor |

## Memory Regions

- **LPDDR4**: 2GB on-board memory
- **OCM**: 256KB On-Chip Memory

## Building

### Prerequisites
- Xilinx Vitis 2023.2
- VPK120 platform BSP

### Build Commands
```bash
make clean
make
```

### Output
- `Debug/dma_benchmark_app.elf` - Standalone application

## Test Features

1. **Throughput Tests** - Measure transfer speed (MB/s)
2. **Latency Tests** - Measure first-byte latency (µs)
3. **Data Integrity Tests** - Verify data correctness with patterns:
   - Incremental (0x00, 0x01, 0x02...)
   - All Ones (0xFF)
   - All Zeros (0x00)
   - Random (PRNG)
   - Checkerboard (0xAA, 0x55...)

## Hardware Requirements

- Xilinx VPK120 Evaluation Board
- Vivado design with:
  - AXI DMA IP (SG mode)
  - AXI CDMA IP (SG mode)
  - AXI MCDMA IP (4 channels)
  - NoC connections to LPDDR4

## License

For educational and development purposes.

## Changelog

See [CHANGELOG.txt](CHANGELOG.txt) for version history.
