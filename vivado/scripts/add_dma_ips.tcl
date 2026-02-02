################################################################################
# Versal DMA Benchmark - DMA IP Configuration
# Target: VPK120 (Versal Premium VP1202)
# Vivado 2023.2 Compatible
################################################################################

puts "INFO: Adding DMA IPs..."

################################################################################
# AXI DMA (Scatter-Gather Mode)
################################################################################

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dma:7.1 axi_dma_0

set_property -dict [list \
    CONFIG.c_include_sg {1} \
    CONFIG.c_sg_include_stscntrl_strm {0} \
    CONFIG.c_sg_length_width {23} \
    CONFIG.c_m_axi_mm2s_data_width {64} \
    CONFIG.c_m_axis_mm2s_tdata_width {64} \
    CONFIG.c_mm2s_burst_size {64} \
    CONFIG.c_m_axi_s2mm_data_width {64} \
    CONFIG.c_s_axis_s2mm_tdata_width {64} \
    CONFIG.c_s2mm_burst_size {64} \
    CONFIG.c_include_mm2s {1} \
    CONFIG.c_include_s2mm {1} \
    CONFIG.c_addr_width {64} \
] [get_bd_cells axi_dma_0]

################################################################################
# AXI CDMA (Central DMA for Memory-to-Memory)
################################################################################

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_cdma:4.1 axi_cdma_0

set_property -dict [list \
    CONFIG.C_INCLUDE_SG {1} \
    CONFIG.C_M_AXI_DATA_WIDTH {64} \
    CONFIG.C_M_AXI_MAX_BURST_LEN {64} \
    CONFIG.C_ADDR_WIDTH {64} \
] [get_bd_cells axi_cdma_0]

################################################################################
# AXI MCDMA (Multi-Channel DMA) - Simplified to 4 channels
################################################################################

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_mcdma:1.1 axi_mcdma_0

set_property -dict [list \
    CONFIG.c_num_mm2s_channels {4} \
    CONFIG.c_num_s2mm_channels {4} \
    CONFIG.c_include_sg {1} \
    CONFIG.c_sg_include_stscntrl_strm {0} \
    CONFIG.c_m_axi_mm2s_data_width {64} \
    CONFIG.c_m_axis_mm2s_tdata_width {64} \
    CONFIG.c_m_axi_s2mm_data_width {64} \
    CONFIG.c_addr_width {64} \
] [get_bd_cells axi_mcdma_0]

################################################################################
# AXI Stream Data FIFO for DMA loopback testing
################################################################################

create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:2.0 axis_data_fifo_0

set_property -dict [list \
    CONFIG.TDATA_NUM_BYTES {8} \
    CONFIG.FIFO_DEPTH {1024} \
    CONFIG.HAS_TKEEP {1} \
    CONFIG.HAS_TLAST {1} \
] [get_bd_cells axis_data_fifo_0]

################################################################################
# Connect DMA control interfaces to SmartConnect (PL peripherals)
################################################################################

puts "INFO: Connecting DMA control interfaces..."

connect_bd_intf_net [get_bd_intf_pins axi_smc_pl/M00_AXI] [get_bd_intf_pins axi_dma_0/S_AXI_LITE]
connect_bd_intf_net [get_bd_intf_pins axi_smc_pl/M01_AXI] [get_bd_intf_pins axi_cdma_0/S_AXI_LITE]
connect_bd_intf_net [get_bd_intf_pins axi_smc_pl/M02_AXI] [get_bd_intf_pins axi_mcdma_0/S_AXI_LITE]

################################################################################
# Connect DMA data interfaces to memory SmartConnect
################################################################################

puts "INFO: Connecting DMA data interfaces..."

# AXI DMA memory interfaces
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXI_SG] [get_bd_intf_pins axi_smc_dma/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXI_MM2S] [get_bd_intf_pins axi_smc_dma/S01_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXI_S2MM] [get_bd_intf_pins axi_smc_dma/S02_AXI]

# AXI CDMA memory interface - create additional SmartConnect
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:1.0 axi_smc_cdma

set_property -dict [list \
    CONFIG.NUM_SI {2} \
    CONFIG.NUM_MI {1} \
] [get_bd_cells axi_smc_cdma]

connect_bd_intf_net [get_bd_intf_pins axi_cdma_0/M_AXI_SG] [get_bd_intf_pins axi_smc_cdma/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_cdma_0/M_AXI] [get_bd_intf_pins axi_smc_cdma/S01_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_smc_cdma/M00_AXI] [get_bd_intf_pins axi_smc_dma/S03_AXI]

# Update axi_smc_dma to have more slave interfaces
set_property -dict [list CONFIG.NUM_SI {5}] [get_bd_cells axi_smc_dma]

# AXI MCDMA memory interfaces
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:1.0 axi_smc_mcdma

set_property -dict [list \
    CONFIG.NUM_SI {3} \
    CONFIG.NUM_MI {1} \
] [get_bd_cells axi_smc_mcdma]

connect_bd_intf_net [get_bd_intf_pins axi_mcdma_0/M_AXI_SG] [get_bd_intf_pins axi_smc_mcdma/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_mcdma_0/M_AXI_MM2S] [get_bd_intf_pins axi_smc_mcdma/S01_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_mcdma_0/M_AXI_S2MM] [get_bd_intf_pins axi_smc_mcdma/S02_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_smc_mcdma/M00_AXI] [get_bd_intf_pins axi_smc_dma/S04_AXI]

################################################################################
# Connect AXI Stream FIFO for DMA Loopback
################################################################################

puts "INFO: Connecting stream loopback..."

connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXIS_MM2S] [get_bd_intf_pins axis_data_fifo_0/S_AXIS]
connect_bd_intf_net [get_bd_intf_pins axis_data_fifo_0/M_AXIS] [get_bd_intf_pins axi_dma_0/S_AXIS_S2MM]

# MCDMA loopback - connect channel 1
create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:2.0 axis_data_fifo_mcdma

set_property -dict [list \
    CONFIG.TDATA_NUM_BYTES {8} \
    CONFIG.FIFO_DEPTH {1024} \
    CONFIG.HAS_TKEEP {1} \
    CONFIG.HAS_TLAST {1} \
] [get_bd_cells axis_data_fifo_mcdma]

connect_bd_intf_net [get_bd_intf_pins axi_mcdma_0/M_AXIS_MM2S] [get_bd_intf_pins axis_data_fifo_mcdma/S_AXIS]
connect_bd_intf_net [get_bd_intf_pins axis_data_fifo_mcdma/M_AXIS] [get_bd_intf_pins axi_mcdma_0/S_AXIS_S2MM]

################################################################################
# Connect Clocks and Resets
################################################################################

puts "INFO: Connecting DMA clocks and resets..."

set pl_clk [get_bd_pins versal_cips_0/pl0_ref_clk]

# Get reset from processor system reset created in block design
# If it doesn't exist yet, we'll create it in the main block design script

# AXI DMA
connect_bd_net ${pl_clk} [get_bd_pins axi_dma_0/s_axi_lite_aclk]
connect_bd_net ${pl_clk} [get_bd_pins axi_dma_0/m_axi_sg_aclk]
connect_bd_net ${pl_clk} [get_bd_pins axi_dma_0/m_axi_mm2s_aclk]
connect_bd_net ${pl_clk} [get_bd_pins axi_dma_0/m_axi_s2mm_aclk]

# AXI CDMA
connect_bd_net ${pl_clk} [get_bd_pins axi_cdma_0/s_axi_lite_aclk]
connect_bd_net ${pl_clk} [get_bd_pins axi_cdma_0/m_axi_aclk]

# AXI MCDMA
connect_bd_net ${pl_clk} [get_bd_pins axi_mcdma_0/s_axi_lite_aclk]
connect_bd_net ${pl_clk} [get_bd_pins axi_mcdma_0/s_axi_aclk]

# Stream FIFOs
connect_bd_net ${pl_clk} [get_bd_pins axis_data_fifo_0/s_axis_aclk]
connect_bd_net ${pl_clk} [get_bd_pins axis_data_fifo_mcdma/s_axis_aclk]

# SmartConnects
connect_bd_net ${pl_clk} [get_bd_pins axi_smc_cdma/aclk]
connect_bd_net ${pl_clk} [get_bd_pins axi_smc_mcdma/aclk]

puts "INFO: DMA IP configuration complete"
