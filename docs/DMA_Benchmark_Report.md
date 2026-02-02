# Versal DMA Benchmark Report
## VPK120 Development Board - VP1202

---

# Table of Contents

1. [Introduction](#1-introduction)
2. [DMA Types Overview](#2-dma-types-overview)
3. [Memory Architecture](#3-memory-architecture)
4. [Block Design Architecture](#4-block-design-architecture)
5. [Benchmark Test Methodology](#5-benchmark-test-methodology)
6. [Expected Performance Results](#6-expected-performance-results)
7. [Implementation Details](#7-implementation-details)
8. [Usage Guide](#8-usage-guide)

---

# 1. Introduction

## 1.1 Project Overview

This document presents a comprehensive DMA benchmark suite for the AMD/Xilinx VPK120 development board featuring the Versal Premium VP1202 ACAP (Adaptive Compute Acceleration Platform). The benchmark evaluates all available DMA types across various memory regions to provide performance characterization data.

## 1.2 Target Platform Specifications

| Parameter | Value |
|-----------|-------|
| **Board** | VPK120 Evaluation Board |
| **Device** | Versal Premium VP1202 (xcvp1202-vsva2785-2MP-e-S) |
| **Process** | 7nm FinFET |
| **PL LUTs** | 899,840 |
| **PL Registers** | 1,799,680 |
| **Block RAM** | 967 (36Kb each) |
| **UltraRAM** | 463 (288Kb each) |
| **DSP Engines** | 1,968 |
| **AI Engines** | 400 tiles |
| **PS Cores** | Dual Cortex-A72 @ 1.7 GHz |
| **RPU** | Dual Cortex-R5F @ 533 MHz |

## 1.3 Objectives

1. Measure throughput and latency for each DMA type
2. Compare performance across different memory regions
3. Evaluate Scatter-Gather vs Simple mode performance
4. Test multi-channel DMA capabilities
5. Provide baseline performance data for system designers

---

# 2. DMA Types Overview

## 2.1 PL Soft IP DMAs

### 2.1.1 AXI DMA (axi_dma)

The AXI DMA IP provides high-bandwidth direct memory access between memory and AXI4-Stream peripherals.

| Feature | Configuration |
|---------|---------------|
| **Data Width** | 512-bit |
| **Address Width** | 64-bit |
| **Max Burst Length** | 256 beats |
| **Scatter-Gather** | Enabled |
| **Channels** | MM2S + S2MM |
| **Max Transfer** | 64 MB (26-bit length register) |

**Use Cases:**
- Streaming data to/from peripherals (video, audio, networking)
- High-throughput data movement with stream interfaces
- DMA offload for CPU-intensive data transfers

**Architecture:**
```
                    ┌─────────────────┐
    AXI4 MM ─────►  │                 │  ─────► AXI4-Stream (MM2S)
   (Memory)         │    AXI DMA      │
                    │                 │  ◄───── AXI4-Stream (S2MM)
    AXI4-Lite ────► │   Controller    │
   (Control)        │                 │  ─────► Interrupt
                    └─────────────────┘
```

### 2.1.2 AXI CDMA (axi_cdma)

The AXI Central DMA provides memory-to-memory transfers without CPU intervention.

| Feature | Configuration |
|---------|---------------|
| **Data Width** | 512-bit |
| **Address Width** | 64-bit |
| **Max Burst Length** | 256 beats |
| **Scatter-Gather** | Enabled |
| **Transfer Type** | Memory-to-Memory |

**Use Cases:**
- Memory copy operations (faster than CPU memcpy)
- Data block movement between memory regions
- Buffer management in high-throughput applications

**Architecture:**
```
                    ┌─────────────────┐
    AXI4 MM ─────►  │                 │  ─────► AXI4 MM
   (Source)         │    AXI CDMA     │        (Destination)
                    │                 │
    AXI4-Lite ────► │   Controller    │  ─────► Interrupt
   (Control)        │                 │
                    └─────────────────┘
```

### 2.1.3 AXI MCDMA (axi_mcdma)

The AXI Multi-Channel DMA provides up to 16 independent channels for concurrent transfers.

| Feature | Configuration |
|---------|---------------|
| **Data Width** | 512-bit |
| **Address Width** | 64-bit |
| **MM2S Channels** | 16 |
| **S2MM Channels** | 16 |
| **Scatter-Gather** | Built-in (always enabled) |
| **Scheduling** | Round-Robin / Strict Priority |

**Use Cases:**
- Multi-stream applications (multiple video/audio channels)
- Network packet processing with multiple queues
- Applications requiring concurrent independent DMA channels

**Architecture:**
```
                        ┌─────────────────────┐
    AXI4 MM ─────────►  │                     │  ──► AXI4-Stream CH0 (MM2S)
   (Memory)             │                     │  ──► AXI4-Stream CH1 (MM2S)
                        │     AXI MCDMA       │  ...
    AXI4-Lite ────────► │    (16 Channels)    │  ──► AXI4-Stream CH15 (MM2S)
   (Control)            │                     │
                        │                     │  ◄── AXI4-Stream CH0 (S2MM)
                        │                     │  ◄── AXI4-Stream CH1 (S2MM)
                        │                     │  ...
                        │                     │  ◄── AXI4-Stream CH15 (S2MM)
                        └─────────────────────┘
```

## 2.2 PS Hard IP DMAs

### 2.2.1 LPD DMA (ADMA)

The Low Power Domain DMA is a hardened DMA controller in the Versal PS.

| Feature | Specification |
|---------|---------------|
| **Channels** | 8 independent channels |
| **Data Width** | 128-bit |
| **Transfer Mode** | Descriptor-driven |
| **Max Transfer** | 4GB per descriptor |
| **Power Domain** | Low Power Domain (LPD) |

**Use Cases:**
- Low-power data movement
- PS peripheral data transfers
- Background data movement with minimal power impact

**Memory Map:**
```
LPD DMA Base: 0xFFA80000

Channel Registers:
  CH0: 0xFFA80000 - 0xFFA80FFF
  CH1: 0xFFA81000 - 0xFFA81FFF
  ...
  CH7: 0xFFA87000 - 0xFFA87FFF
```

## 2.3 DMA Comparison Summary

| Feature | AXI DMA | AXI CDMA | AXI MCDMA | LPD DMA |
|---------|---------|----------|-----------|---------|
| **Type** | Soft IP | Soft IP | Soft IP | Hard IP |
| **Interface** | Stream | MM-to-MM | Stream | MM-to-MM |
| **Channels** | 2 | 1 | 32 | 8 |
| **Data Width** | 512b | 512b | 512b | 128b |
| **SG Support** | Yes | Yes | Built-in | Yes |
| **Expected BW** | ~12 GB/s | ~15 GB/s | ~18 GB/s | ~8 GB/s |
| **Latency** | Low | Low | Medium | Very Low |
| **Power** | Medium | Medium | High | Low |

---

# 3. Memory Architecture

## 3.1 Memory Regions on VPK120

### 3.1.1 DDR4 SODIMM

| Parameter | Value |
|-----------|-------|
| **Type** | DDR4-3200 |
| **Size** | 8 GB |
| **Interface** | PS DDR Memory Controller |
| **Data Width** | 64-bit |
| **Theoretical BW** | 25.6 GB/s |
| **Address Range** | 0x00000000 - 0x1FFFFFFFF |

### 3.1.2 LPDDR4

| Parameter | Value |
|-----------|-------|
| **Type** | LPDDR4-4266 |
| **Size** | 2 GB |
| **Interface** | NoC Memory Controller |
| **Data Width** | 32-bit |
| **Theoretical BW** | 17.1 GB/s |

### 3.1.3 On-Chip Memory (OCM)

| Parameter | Value |
|-----------|-------|
| **Size** | 256 KB |
| **Access** | Single-cycle (from PS) |
| **Latency** | ~2-5 cycles |
| **Address Range** | 0xFFFC0000 - 0xFFFFFFFF |

**Use Cases:** Critical real-time data, interrupt handlers, stack space

### 3.1.4 PL Block RAM (BRAM)

| Parameter | Value |
|-----------|-------|
| **Configuration** | AXI BRAM Controller + BRAM |
| **Size** | 128 KB (instantiated) |
| **Data Width** | 512-bit |
| **Latency** | 1-2 cycles |

### 3.1.5 PL UltraRAM (URAM)

| Parameter | Value |
|-----------|-------|
| **Size** | 64 KB (instantiated) |
| **Data Width** | 72-bit per port |
| **Latency** | 1 cycle |

## 3.2 Memory Hierarchy

```
┌────────────────────────────────────────────────────────────┐
│                         CPU (A72)                          │
│                      L1 Cache (32KB)                       │
│                      L2 Cache (1MB)                        │
└─────────────────────────┬──────────────────────────────────┘
                          │
              ┌───────────┴───────────┐
              ▼                       ▼
┌─────────────────────┐   ┌─────────────────────┐
│        OCM          │   │    DDR4 / LPDDR4    │
│     (256 KB)        │   │     (8GB + 2GB)     │
│   ~5 cycles         │   │    ~100+ cycles     │
└─────────────────────┘   └─────────────────────┘
                                    │
                          ┌─────────┴─────────┐
                          ▼                   ▼
              ┌─────────────────┐   ┌─────────────────┐
              │    PL BRAM      │   │    PL URAM      │
              │   (128 KB)      │   │    (64 KB)      │
              │   ~2 cycles     │   │   ~1 cycle      │
              └─────────────────┘   └─────────────────┘
```

## 3.3 Expected Bandwidth Characteristics

| Memory Type | Read BW | Write BW | Latency |
|-------------|---------|----------|---------|
| **DDR4** | ~20-25 GB/s | ~18-22 GB/s | 80-120 ns |
| **LPDDR4** | ~15-17 GB/s | ~14-16 GB/s | 60-100 ns |
| **OCM** | ~8-10 GB/s | ~8-10 GB/s | 5-10 ns |
| **BRAM** | ~6-8 GB/s | ~6-8 GB/s | 2-5 ns |
| **URAM** | ~4-6 GB/s | ~4-6 GB/s | 2-4 ns |

---

# 4. Block Design Architecture

## 4.1 System Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                              CIPS (Versal PS)                                   │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐            │
│  │   ARM A72   │  │    NoC     │  │  LPD DMA    │  │    CPM      │            │
│  │   (APU)     │  │ (DDR/LPDDR)│  │  (ADMA)     │  │ (PCIe+QDMA) │            │
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘            │
└─────────────────────────────────────────────────────────────────────────────────┘
           │                │                │
           │          AXI NoC               │
           │                │                │
           ▼                ▼                ▼
     ┌─────────────────────────────────────────────────────────┐
     │                   AXI SmartConnect                       │
     │              (Interconnect for PL IPs)                   │
     └─────────────────────────────────────────────────────────┘
                │              │              │
                ▼              ▼              ▼
         ┌───────────┐  ┌───────────┐  ┌───────────┐
         │  AXI DMA  │  │ AXI CDMA  │  │AXI MCDMA  │
         │ (SG Mode) │  │ (SG Mode) │  │ (16 CH)   │
         └───────────┘  └───────────┘  └───────────┘
                │              │              │
                ▼              │              ▼
         ┌───────────┐         │       ┌───────────┐
         │ AXI Stream│         │       │AXI Stream │
         │ Data FIFO │         │       │Data FIFOs │
         └─────┬─────┘         │       └─────┬─────┘
               │               │             │
               └───────────────┴─────────────┘
                               │
                       ┌───────┴───────┐
                       ▼               ▼
                ┌───────────┐   ┌───────────┐
                │ PL BRAM   │   │ PL URAM   │
                │Controller │   │  (64KB)   │
                │ (128KB)   │   │           │
                └───────────┘   └───────────┘
```

## 4.2 Clock Domains

| Clock | Frequency | Usage |
|-------|-----------|-------|
| **pl0_ref_clk** | 100 MHz | AXI control interfaces |
| **pl1_ref_clk** | 250 MHz | Data path clocking |
| **pl2_ref_clk** | 300 MHz | High-speed data path |

## 4.3 Address Map

| IP | Base Address | Size | Description |
|----|--------------|------|-------------|
| **AXI DMA** | 0xA0000000 | 64KB | DMA control registers |
| **AXI CDMA** | 0xA0010000 | 64KB | CDMA control registers |
| **AXI MCDMA** | 0xA0020000 | 64KB | MCDMA control registers |
| **BRAM Controller** | 0xA0040000 | 128KB | PL BRAM access |
| **DDR4** | 0x00000000 | 8GB | Main memory |
| **OCM** | 0xFFFC0000 | 256KB | On-chip memory |

## 4.4 Interrupt Mapping

| Interrupt | Source | Description |
|-----------|--------|-------------|
| IRQ[0] | AXI DMA MM2S | Memory-to-Stream complete |
| IRQ[1] | AXI DMA S2MM | Stream-to-Memory complete |
| IRQ[2] | AXI CDMA | Transfer complete |
| IRQ[3] | AXI MCDMA MM2S CH1 | Multi-channel MM2S |
| IRQ[4] | AXI MCDMA S2MM CH1 | Multi-channel S2MM |

---

# 5. Benchmark Test Methodology

## 5.1 Test Categories

### 5.1.1 Throughput Tests

Measure maximum sustainable transfer rate.

**Test Parameters:**
- Transfer sizes: 64B to 16MB
- Iterations: 100-1000 per size
- Metric: MB/s or GB/s

**Calculation:**
```
Throughput (MB/s) = (Total Bytes Transferred / Total Time) × 10^6
```

### 5.1.2 Latency Tests

Measure single transfer completion time.

**Test Parameters:**
- Transfer sizes: 64B to 64KB
- Iterations: 1000
- Metric: microseconds (μs)

**Calculation:**
```
Latency (μs) = Transfer Complete Time - Transfer Start Time
```

### 5.1.3 Data Integrity Tests

Verify data correctness after transfer.

**Data Patterns:**
| Pattern | Value | Description |
|---------|-------|-------------|
| Incremental | 0x00, 0x01, 0x02... | Sequential bytes |
| All-Ones | 0xFF, 0xFF, 0xFF... | Maximum toggle test |
| All-Zeros | 0x00, 0x00, 0x00... | Minimum toggle test |
| Random | PRNG generated | Real-world simulation |
| Checkerboard | 0xAA, 0x55, 0xAA... | Bit-level integrity |

### 5.1.4 Multi-Channel Tests

Evaluate concurrent channel performance.

**Test Matrix:**
| Channels | Expected Aggregate BW |
|----------|----------------------|
| 1 | 100% of single channel |
| 2 | ~180% of single channel |
| 4 | ~340% of single channel |
| 8 | ~600% of single channel |
| 16 | ~900% of single channel |

## 5.2 Transfer Size Matrix

| Category | Sizes |
|----------|-------|
| **Small** | 64B, 128B, 256B, 512B |
| **Medium** | 1KB, 2KB, 4KB, 8KB, 16KB, 32KB, 64KB |
| **Large** | 128KB, 256KB, 512KB, 1MB, 2MB, 4MB, 8MB, 16MB |

## 5.3 Memory-to-Memory Test Matrix (CDMA)

| Source → Dest | DDR4 | LPDDR4 | OCM | BRAM | URAM |
|---------------|:----:|:------:|:---:|:----:|:----:|
| **DDR4** | ✓ | ✓ | ✓ | ✓ | ✓ |
| **LPDDR4** | ✓ | ✓ | ✓ | ✓ | ✓ |
| **OCM** | ✓ | ✓ | ✓ | ✓ | ✓ |
| **BRAM** | ✓ | ✓ | ✓ | ✓ | ✓ |
| **URAM** | ✓ | ✓ | ✓ | ✓ | ✓ |

---

# 6. Expected Performance Results

## 6.1 Theoretical Maximum Bandwidth

| DMA Type | Data Width | Clock | Theoretical Max |
|----------|------------|-------|-----------------|
| AXI DMA | 512-bit | 250 MHz | 16 GB/s |
| AXI CDMA | 512-bit | 250 MHz | 16 GB/s |
| AXI MCDMA | 512-bit | 250 MHz | 16 GB/s per direction |
| LPD DMA | 128-bit | 400 MHz | 6.4 GB/s |

## 6.2 Expected Practical Results

### 6.2.1 AXI DMA Performance (DDR4-to-DDR4 via FIFO)

| Transfer Size | Expected Throughput | Expected Latency |
|--------------|---------------------|------------------|
| 64 B | ~500 MB/s | ~0.5 μs |
| 1 KB | ~4 GB/s | ~0.8 μs |
| 64 KB | ~10 GB/s | ~7 μs |
| 1 MB | ~12 GB/s | ~85 μs |
| 16 MB | ~12.5 GB/s | ~1.3 ms |

### 6.2.2 AXI CDMA Performance (Memory-to-Memory)

| Transfer Size | DDR→DDR | DDR→BRAM | OCM→DDR |
|--------------|---------|----------|---------|
| 64 B | ~600 MB/s | ~800 MB/s | ~1 GB/s |
| 1 KB | ~5 GB/s | ~6 GB/s | ~4 GB/s |
| 64 KB | ~13 GB/s | ~8 GB/s | ~6 GB/s |
| 1 MB | ~14 GB/s | ~8 GB/s | ~6 GB/s |
| 16 MB | ~15 GB/s | ~8 GB/s | ~6 GB/s |

### 6.2.3 AXI MCDMA Aggregate Performance

| Channels Active | Expected Aggregate BW |
|----------------|----------------------|
| 1 | ~10 GB/s |
| 4 | ~25 GB/s |
| 8 | ~35 GB/s |
| 16 | ~40 GB/s |

### 6.2.4 LPD DMA Performance

| Transfer Size | Expected Throughput | Notes |
|--------------|---------------------|-------|
| 1 KB | ~3 GB/s | Best latency |
| 64 KB | ~6 GB/s | Good throughput |
| 1 MB | ~7 GB/s | Near maximum |

## 6.3 DMA vs CPU memcpy Comparison

| Transfer Size | CPU memcpy | AXI CDMA | Speedup |
|--------------|------------|----------|---------|
| 1 KB | ~3 GB/s | ~5 GB/s | 1.7x |
| 64 KB | ~6 GB/s | ~13 GB/s | 2.2x |
| 1 MB | ~8 GB/s | ~14 GB/s | 1.8x |
| 16 MB | ~8 GB/s | ~15 GB/s | 1.9x |

**Key Advantages of DMA:**
1. Zero CPU utilization during transfer
2. Higher sustained bandwidth
3. Scatter-Gather enables complex memory layouts
4. Interrupt-driven completion notification

---

# 7. Implementation Details

## 7.1 Implementation Summary

| Metric | Value |
|--------|-------|
| **Target Device** | xcvp1202-vsva2785-2MP-e-S |
| **Synthesis Tool** | Vivado 2023.2 |
| **Implementation Strategy** | Default |
| **PL Clock Frequency** | 100 MHz / 250 MHz / 300 MHz |

## 7.2 Timing Results

| Metric | Value | Status |
|--------|-------|--------|
| **WNS (Setup)** | +4.146 ns | ✅ Met |
| **TNS (Setup)** | 0.000 ns | ✅ Met |
| **WHS (Hold)** | +0.015 ns | ✅ Met |
| **THS (Hold)** | 0.000 ns | ✅ Met |

## 7.3 Resource Utilization

| Resource | Used | Available | Utilization |
|----------|------|-----------|-------------|
| **CLB LUTs** | ~15,000 | 899,840 | ~1.7% |
| **CLB Registers** | ~25,000 | 1,799,680 | ~1.4% |
| **Block RAM** | ~20 | 967 | ~2.1% |
| **URAM** | 0 | 463 | 0% |
| **DSP** | 0 | 1,968 | 0% |

## 7.4 Power Estimation

| Domain | Estimated Power |
|--------|-----------------|
| **PL Static** | ~0.5 W |
| **PL Dynamic** | ~1.2 W |
| **PS** | ~3.5 W |
| **Total** | ~5.2 W |

---

# 8. Usage Guide

## 8.1 Building the Project

### Step 1: Vivado Project Creation
```tcl
vivado -mode batch -source vivado/scripts/create_project.tcl
```

### Step 2: Block Design and Synthesis
```tcl
vivado -mode batch -source vivado/scripts/run_synth_xsa.tcl
```

### Step 3: Implementation and PDI Generation
```tcl
vivado -mode batch -source vivado/scripts/run_impl_pdi.tcl
```

## 8.2 Running the Benchmark

### Step 1: Download PDI
```
xsct> connect
xsct> target -set -filter {name =~ "*A72*0"}
xsct> rst -system
xsct> device program dma_benchmark.pdi
```

### Step 2: Download Application
```
xsct> dow dma_benchmark_app.elf
xsct> con
```

### Step 3: View Results
Connect to UART (115200 8N1) to view benchmark results in CSV format.

## 8.3 CSV Output Format

```csv
dma_type,test_type,src_memory,dst_memory,transfer_size,data_pattern,mode,throughput_mbps,latency_us,cpu_util,integrity,iterations
AXI_DMA,throughput,DDR4,DDR4,1048576,INCREMENTAL,SG,12500.5,83.2,0,PASS,100
AXI_CDMA,throughput,DDR4,BRAM,65536,RANDOM,SG,8200.3,8.1,0,PASS,100
...
```

## 8.4 Analyzing Results

The benchmark produces comprehensive data for:
- Throughput vs Transfer Size curves
- Latency distribution analysis
- Memory region performance comparison
- DMA type efficiency comparison

---

# Appendix A: Register Reference

## A.1 AXI DMA Registers

| Offset | Register | Description |
|--------|----------|-------------|
| 0x00 | MM2S_DMACR | MM2S DMA Control |
| 0x04 | MM2S_DMASR | MM2S DMA Status |
| 0x08 | MM2S_CURDESC | MM2S Current Descriptor |
| 0x10 | MM2S_TAILDESC | MM2S Tail Descriptor |
| 0x30 | S2MM_DMACR | S2MM DMA Control |
| 0x34 | S2MM_DMASR | S2MM DMA Status |
| 0x38 | S2MM_CURDESC | S2MM Current Descriptor |
| 0x40 | S2MM_TAILDESC | S2MM Tail Descriptor |

## A.2 Scatter-Gather Descriptor Format

```
Offset  Field           Description
0x00    NXTDESC         Next Descriptor Pointer (64-bit)
0x08    BUFFER_ADDR     Buffer Address (64-bit)
0x10    RESERVED        Reserved
0x18    CONTROL         Control (BTT, SOF, EOF)
0x1C    STATUS          Status (Transferred, Error)
0x20    APP[0-4]        Application-specific fields
```

---

# Appendix B: Troubleshooting

## B.1 Common Issues

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| DMA Timeout | SG descriptors not flushed | Use Xil_DCacheFlushRange() |
| Data Corruption | Cache coherency | Flush/invalidate cache |
| Low Throughput | Unaligned addresses | Use 64-byte aligned buffers |
| Interrupt not firing | IRQ not connected | Check CIPS interrupt mapping |

## B.2 Debug Commands

```c
// Print DMA status
printf("MM2S Status: 0x%08X\n", Xil_In32(DMA_BASE + 0x04));
printf("S2MM Status: 0x%08X\n", Xil_In32(DMA_BASE + 0x34));

// Check for errors
if (status & 0x70) {
    printf("DMA Error detected: 0x%X\n", (status >> 4) & 0x7);
}
```

---

**Document Version:** 1.0
**Date:** January 2026
**Author:** VPK120 DMA Benchmark Suite
**Target:** AMD/Xilinx Versal Premium VP1202
