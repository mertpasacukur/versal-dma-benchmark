################################################################################
# Versal DMA Benchmark - Memory IP Configuration
# Target: VPK120 (Versal Premium VP1202)
# Vivado 2023.2 Compatible - Using Embedded Memory Generator for Versal
################################################################################

puts "INFO: Adding Memory IPs..."

################################################################################
# AXI BRAM Controller + Embedded Memory (64KB)
################################################################################

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 axi_bram_ctrl_0

set_property -dict [list \
    CONFIG.SINGLE_PORT_BRAM {1} \
    CONFIG.DATA_WIDTH {64} \
    CONFIG.ECC_TYPE {0} \
] [get_bd_cells axi_bram_ctrl_0]

# Create Embedded Memory Generator for Versal (instead of blk_mem_gen)
create_bd_cell -type ip -vlnv xilinx.com:ip:emb_mem_gen:1.0 emb_mem_gen_0

set_property -dict [list \
    CONFIG.MEMORY_TYPE {True_Dual_Port_RAM} \
    CONFIG.MEMORY_SIZE {65536} \
    CONFIG.MEMORY_PRIMITIVE {BRAM} \
    CONFIG.READ_DATA_WIDTH_A {64} \
    CONFIG.WRITE_DATA_WIDTH_A {64} \
    CONFIG.READ_DATA_WIDTH_B {64} \
    CONFIG.WRITE_DATA_WIDTH_B {64} \
    CONFIG.BYTE_WRITE_WIDTH_A {8} \
    CONFIG.READ_LATENCY_A {1} \
    CONFIG.READ_LATENCY_B {1} \
] [get_bd_cells emb_mem_gen_0]

# Connect BRAM Controller to Embedded Memory
connect_bd_intf_net [get_bd_intf_pins axi_bram_ctrl_0/BRAM_PORTA] [get_bd_intf_pins emb_mem_gen_0/BRAM_PORTA]

################################################################################
# Connect Memory Controllers to SmartConnect
################################################################################

# Connect BRAM Controller to SmartConnect - use M03_AXI
connect_bd_intf_net [get_bd_intf_pins axi_smc_pl/M03_AXI] [get_bd_intf_pins axi_bram_ctrl_0/S_AXI]

################################################################################
# Connect Clocks
################################################################################

set pl_clk [get_bd_pins versal_cips_0/pl0_ref_clk]

# BRAM Controller clock
connect_bd_net ${pl_clk} [get_bd_pins axi_bram_ctrl_0/s_axi_aclk]

puts "INFO: Memory IP configuration complete"
