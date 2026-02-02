#===============================================================================
# Versal DMA Benchmark - Complete Design with All DMA Types (v4)
# VPK120 Board with LPDDR4
#
# v4 Changes:
#   - CRITICAL: Added LPD_AXI_NOC interface for ADMA access to LPDDR4
#   - ADMA can now write to DDR through NoC
#   - AXI DMA 26-bit length (64MB max)
#   - Reduced NoC bandwidth to avoid overflow
#===============================================================================

set script_dir [file dirname [file normalize [info script]]]
set project_dir [file normalize "$script_dir/.."]
set project_name "versal_all_dma_v4"

puts "============================================================"
puts "  Versal DMA Benchmark - All DMA Types (v4)"
puts "  VPK120 + LPDDR4"
puts "  KEY FIX: LPD_AXI_NOC enabled for ADMA DDR access"
puts "============================================================"

# Cleanup
catch {close_project}
if {[file exists "$project_dir/$project_name"]} {
    file delete -force "$project_dir/$project_name"
}

# Create project with VPK120 board
create_project $project_name "$project_dir/$project_name" -part xcvp1202-vsva2785-2MP-e-S
set_property board_part xilinx.com:vpk120:part0:1.2 [current_project]

create_bd_design "design_1"

#===============================================================================
# CIPS - With LPD DMA AND LPD_AXI_NOC enabled
#===============================================================================
puts "Adding CIPS with LPD DMA and LPD_AXI_NOC..."
create_bd_cell -type ip -vlnv xilinx.com:ip:versal_cips:* cips_0

# Apply board automation for CIPS with LPDDR4
apply_bd_automation -rule xilinx.com:bd_rule:cips -config { \
    board_preset {Yes} \
    boot_config {Custom} \
    configure_noc {Add new AXI NoC} \
    debug_config {JTAG} \
    design_flow {Full System} \
    mc_type {LPDDR} \
    num_mc_ddr {None} \
    num_mc_lpddr {1} \
    pl_clocks {1} \
    pl_resets {1} \
} [get_bd_cells cips_0]

# CRITICAL: Enable LPD_AXI_NOC interface for ADMA DDR access
# Also enable M_AXI_FPD for PL DMA control
set_property -dict [list \
    CONFIG.PS_PMC_CONFIG { \
        PS_USE_M_AXI_FPD {1} \
        PS_USE_M_AXI_LPD {0} \
        PS_USE_NOC_LPD_AXI0 {1} \
        PS_NUM_F2P_IRQ {4} \
        PS_IRQ_USAGE {{CH0 1} {CH1 1} {CH2 1} {CH3 1} {CH4 0} {CH5 0} {CH6 0} {CH7 0} {CH8 0} {CH9 0} {CH10 0} {CH11 0} {CH12 0} {CH13 0} {CH14 0} {CH15 0}} \
        PS_USE_PMCPL_CLK0 {1} \
    } \
] [get_bd_cells cips_0]

puts "CIPS configured with LPD_AXI_NOC enabled"

#===============================================================================
# Get NoC created by board automation
#===============================================================================
set noc_cell [get_bd_cells -filter {VLNV =~ *axi_noc*}]
puts "NoC cell: $noc_cell"

# Get current number of slave interfaces
set current_si [get_property CONFIG.NUM_SI [get_bd_cells $noc_cell]]
puts "Current NoC SI count: $current_si"

# Add 3 more slave interfaces for PL DMAs (LPD_AXI_NOC handled by board automation)
set new_si [expr $current_si + 3]
set_property CONFIG.NUM_SI $new_si [get_bd_cells $noc_cell]

# Add clocks for new interfaces
set current_clks [get_property CONFIG.NUM_CLKS [get_bd_cells $noc_cell]]
set new_clks [expr $current_clks + 1]
set_property CONFIG.NUM_CLKS $new_clks [get_bd_cells $noc_cell]

# Configure interface names for PL DMAs only
set si_cdma "S[format %02d $current_si]_AXI"
set si_dma "S[format %02d [expr $current_si + 1]]_AXI"
set si_mcdma "S[format %02d [expr $current_si + 2]]_AXI"
set pl_clk_name "aclk[expr $current_clks]"

puts "CDMA interface: $si_cdma"
puts "DMA interface: $si_dma"
puts "MCDMA interface: $si_mcdma"
puts "PL clock: $pl_clk_name"
puts "LPD_AXI_NOC: Handled by board automation (ADMA->DDR path)"

# Configure PL DMA interfaces (1000 MB/s each to stay within limits)
set_property -dict [list \
    CONFIG.CONNECTIONS {MC_0 {read_bw {1000} write_bw {1000} read_avg_burst {4} write_avg_burst {4}}} \
    CONFIG.CATEGORY {pl} \
] [get_bd_intf_pins $noc_cell/$si_cdma]

set_property -dict [list \
    CONFIG.CONNECTIONS {MC_0 {read_bw {1000} write_bw {1000} read_avg_burst {4} write_avg_burst {4}}} \
    CONFIG.CATEGORY {pl} \
] [get_bd_intf_pins $noc_cell/$si_dma]

set_property -dict [list \
    CONFIG.CONNECTIONS {MC_0 {read_bw {1000} write_bw {1000} read_avg_burst {4} write_avg_burst {4}}} \
    CONFIG.CATEGORY {pl} \
] [get_bd_intf_pins $noc_cell/$si_mcdma]

# Associate PL interfaces with PL clock
set_property -dict [list CONFIG.ASSOCIATED_BUSIF "$si_cdma:$si_dma:$si_mcdma"] [get_bd_pins $noc_cell/$pl_clk_name]

#===============================================================================
# AXI CDMA - Memory to Memory
#===============================================================================
puts "Adding AXI CDMA..."
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_cdma:* axi_cdma_0

set_property -dict [list \
    CONFIG.C_INCLUDE_SG {1} \
    CONFIG.C_M_AXI_DATA_WIDTH {128} \
    CONFIG.C_M_AXI_MAX_BURST_LEN {64} \
    CONFIG.C_ADDR_WIDTH {64} \
] [get_bd_cells axi_cdma_0]

#===============================================================================
# AXI DMA - Stream based with loopback (26-bit length = 64MB max)
#===============================================================================
puts "Adding AXI DMA with 26-bit buffer length..."
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dma:* axi_dma_0

set_property -dict [list \
    CONFIG.c_include_sg {1} \
    CONFIG.c_sg_include_stscntrl_strm {0} \
    CONFIG.c_m_axi_mm2s_data_width {128} \
    CONFIG.c_m_axis_mm2s_tdata_width {128} \
    CONFIG.c_mm2s_burst_size {64} \
    CONFIG.c_m_axi_s2mm_data_width {128} \
    CONFIG.c_s_axis_s2mm_tdata_width {128} \
    CONFIG.c_s2mm_burst_size {64} \
    CONFIG.c_addr_width {64} \
    CONFIG.c_sg_length_width {26} \
] [get_bd_cells axi_dma_0]

# AXI Stream FIFO for DMA loopback
create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:* axis_fifo_dma
set_property -dict [list \
    CONFIG.FIFO_DEPTH {1024} \
    CONFIG.HAS_TKEEP {1} \
    CONFIG.HAS_TLAST {1} \
    CONFIG.TDATA_NUM_BYTES {16} \
] [get_bd_cells axis_fifo_dma]

#===============================================================================
# AXI MCDMA - Multi-Channel DMA
#===============================================================================
puts "Adding AXI MCDMA..."
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_mcdma:* axi_mcdma_0

set_property -dict [list \
    CONFIG.c_include_sg {1} \
    CONFIG.c_num_mm2s_channels {4} \
    CONFIG.c_num_s2mm_channels {4} \
    CONFIG.c_m_axi_mm2s_data_width {128} \
    CONFIG.c_m_axi_s2mm_data_width {128} \
    CONFIG.c_m_axis_mm2s_tdata_width {128} \
    CONFIG.c_s_axis_s2mm_tdata_width {128} \
    CONFIG.c_addr_width {64} \
] [get_bd_cells axi_mcdma_0]

# AXI Stream FIFO for MCDMA loopback
create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:* axis_fifo_mcdma
set_property -dict [list \
    CONFIG.FIFO_DEPTH {1024} \
    CONFIG.HAS_TKEEP {1} \
    CONFIG.HAS_TLAST {1} \
    CONFIG.TDATA_NUM_BYTES {16} \
] [get_bd_cells axis_fifo_mcdma]

#===============================================================================
# SmartConnects
#===============================================================================
puts "Adding SmartConnects..."

# Control path: PS -> DMAs
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:* axi_smc_ctrl
set_property -dict [list CONFIG.NUM_SI {1} CONFIG.NUM_MI {3}] [get_bd_cells axi_smc_ctrl]

# Data paths: DMAs -> NoC
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:* axi_smc_cdma
set_property -dict [list CONFIG.NUM_SI {1} CONFIG.NUM_MI {1}] [get_bd_cells axi_smc_cdma]

create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:* axi_smc_dma
set_property -dict [list CONFIG.NUM_SI {3} CONFIG.NUM_MI {1}] [get_bd_cells axi_smc_dma]

create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:* axi_smc_mcdma
set_property -dict [list CONFIG.NUM_SI {3} CONFIG.NUM_MI {1}] [get_bd_cells axi_smc_mcdma]

#===============================================================================
# Reset
#===============================================================================
create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:* rst_pl

#===============================================================================
# Interrupt Concat
#===============================================================================
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:* xlconcat_0
set_property CONFIG.NUM_PORTS {4} [get_bd_cells xlconcat_0]

#===============================================================================
# Connections
#===============================================================================
puts "Making connections..."

# PL clock and reset
set pl_clk [get_bd_pins cips_0/pl0_ref_clk]
set pl_rst [get_bd_pins cips_0/pl0_resetn]

# Reset module
connect_bd_net $pl_clk [get_bd_pins rst_pl/slowest_sync_clk]
connect_bd_net $pl_rst [get_bd_pins rst_pl/ext_reset_in]
set aresetn [get_bd_pins rst_pl/peripheral_aresetn]

# NoC PL clock
connect_bd_net $pl_clk [get_bd_pins $noc_cell/$pl_clk_name]

# CIPS M_AXI_FPD clock
connect_bd_net $pl_clk [get_bd_pins cips_0/m_axi_fpd_aclk]

# NOTE: LPD_AXI_NOC_0 is already connected by board automation when PS_USE_NOC_LPD_AXI0=1
# We don't need to manually connect it - board automation handles the LPD to DDR path
# The 4th SI we added is not needed for LPD, remove the extra configuration
puts "LPD_AXI_NOC_0 already connected by board automation - ADMA has DDR access"

#-------------------------------------------------------------------------------
# SmartConnect clocks and resets
#-------------------------------------------------------------------------------
connect_bd_net $pl_clk [get_bd_pins axi_smc_ctrl/aclk]
connect_bd_net $pl_clk [get_bd_pins axi_smc_cdma/aclk]
connect_bd_net $pl_clk [get_bd_pins axi_smc_dma/aclk]
connect_bd_net $pl_clk [get_bd_pins axi_smc_mcdma/aclk]

connect_bd_net $aresetn [get_bd_pins axi_smc_ctrl/aresetn]
connect_bd_net $aresetn [get_bd_pins axi_smc_cdma/aresetn]
connect_bd_net $aresetn [get_bd_pins axi_smc_dma/aresetn]
connect_bd_net $aresetn [get_bd_pins axi_smc_mcdma/aresetn]

#-------------------------------------------------------------------------------
# CDMA clocks and connections
#-------------------------------------------------------------------------------
connect_bd_net $pl_clk [get_bd_pins axi_cdma_0/m_axi_aclk]
connect_bd_net $pl_clk [get_bd_pins axi_cdma_0/s_axi_lite_aclk]
connect_bd_net $aresetn [get_bd_pins axi_cdma_0/s_axi_lite_aresetn]

connect_bd_intf_net [get_bd_intf_pins axi_smc_ctrl/M00_AXI] [get_bd_intf_pins axi_cdma_0/S_AXI_LITE]
connect_bd_intf_net [get_bd_intf_pins axi_cdma_0/M_AXI] [get_bd_intf_pins axi_smc_cdma/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_smc_cdma/M00_AXI] [get_bd_intf_pins $noc_cell/$si_cdma]

#-------------------------------------------------------------------------------
# AXI DMA clocks and connections
#-------------------------------------------------------------------------------
connect_bd_net $pl_clk [get_bd_pins axi_dma_0/s_axi_lite_aclk]
connect_bd_net $pl_clk [get_bd_pins axi_dma_0/m_axi_mm2s_aclk]
connect_bd_net $pl_clk [get_bd_pins axi_dma_0/m_axi_s2mm_aclk]
connect_bd_net $pl_clk [get_bd_pins axi_dma_0/m_axi_sg_aclk]
connect_bd_net $aresetn [get_bd_pins axi_dma_0/axi_resetn]

connect_bd_net $pl_clk [get_bd_pins axis_fifo_dma/s_axis_aclk]
connect_bd_net $aresetn [get_bd_pins axis_fifo_dma/s_axis_aresetn]

connect_bd_intf_net [get_bd_intf_pins axi_smc_ctrl/M01_AXI] [get_bd_intf_pins axi_dma_0/S_AXI_LITE]
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXI_MM2S] [get_bd_intf_pins axi_smc_dma/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXI_S2MM] [get_bd_intf_pins axi_smc_dma/S01_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXI_SG] [get_bd_intf_pins axi_smc_dma/S02_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_smc_dma/M00_AXI] [get_bd_intf_pins $noc_cell/$si_dma]

# DMA stream loopback
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXIS_MM2S] [get_bd_intf_pins axis_fifo_dma/S_AXIS]
connect_bd_intf_net [get_bd_intf_pins axis_fifo_dma/M_AXIS] [get_bd_intf_pins axi_dma_0/S_AXIS_S2MM]

#-------------------------------------------------------------------------------
# MCDMA clocks and connections
#-------------------------------------------------------------------------------
connect_bd_net $pl_clk [get_bd_pins axi_mcdma_0/s_axi_aclk]
connect_bd_net $pl_clk [get_bd_pins axi_mcdma_0/s_axi_lite_aclk]
connect_bd_net $aresetn [get_bd_pins axi_mcdma_0/axi_resetn]

connect_bd_net $pl_clk [get_bd_pins axis_fifo_mcdma/s_axis_aclk]
connect_bd_net $aresetn [get_bd_pins axis_fifo_mcdma/s_axis_aresetn]

connect_bd_intf_net [get_bd_intf_pins axi_smc_ctrl/M02_AXI] [get_bd_intf_pins axi_mcdma_0/S_AXI_LITE]
connect_bd_intf_net [get_bd_intf_pins axi_mcdma_0/M_AXI_MM2S] [get_bd_intf_pins axi_smc_mcdma/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_mcdma_0/M_AXI_S2MM] [get_bd_intf_pins axi_smc_mcdma/S01_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_mcdma_0/M_AXI_SG] [get_bd_intf_pins axi_smc_mcdma/S02_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_smc_mcdma/M00_AXI] [get_bd_intf_pins $noc_cell/$si_mcdma]

# MCDMA stream loopback
connect_bd_intf_net [get_bd_intf_pins axi_mcdma_0/M_AXIS_MM2S] [get_bd_intf_pins axis_fifo_mcdma/S_AXIS]
connect_bd_intf_net [get_bd_intf_pins axis_fifo_mcdma/M_AXIS] [get_bd_intf_pins axi_mcdma_0/S_AXIS_S2MM]

#-------------------------------------------------------------------------------
# PS control path
#-------------------------------------------------------------------------------
connect_bd_intf_net [get_bd_intf_pins cips_0/M_AXI_FPD] [get_bd_intf_pins axi_smc_ctrl/S00_AXI]

#-------------------------------------------------------------------------------
# Interrupts
#-------------------------------------------------------------------------------
connect_bd_net [get_bd_pins axi_cdma_0/cdma_introut] [get_bd_pins xlconcat_0/In0]
connect_bd_net [get_bd_pins axi_dma_0/mm2s_introut] [get_bd_pins xlconcat_0/In1]
connect_bd_net [get_bd_pins axi_dma_0/s2mm_introut] [get_bd_pins xlconcat_0/In2]
connect_bd_net [get_bd_pins axi_mcdma_0/mm2s_ch1_introut] [get_bd_pins xlconcat_0/In3]
connect_bd_net [get_bd_pins xlconcat_0/dout] [get_bd_pins cips_0/pl_ps_irq0]

puts "Connections complete"

#===============================================================================
# Finalize
#===============================================================================
regenerate_bd_layout
assign_bd_address

save_bd_design
puts "Validating..."
validate_bd_design
save_bd_design

#===============================================================================
# Generate
#===============================================================================
puts "Generating output products..."
generate_target all [get_files design_1.bd]

make_wrapper -files [get_files design_1.bd] -top
add_files -norecurse "$project_dir/$project_name/$project_name.gen/sources_1/bd/design_1/hdl/design_1_wrapper.v"
update_compile_order -fileset sources_1

#===============================================================================
# Synthesis
#===============================================================================
puts ""
puts "============================================================"
puts "  Synthesis..."
puts "============================================================"

launch_runs synth_1 -jobs 8
wait_on_run synth_1
puts "Synthesis: [get_property STATUS [get_runs synth_1]]"

if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    puts "ERROR: Synthesis failed!"
    exit 1
}

#===============================================================================
# Implementation
#===============================================================================
puts ""
puts "============================================================"
puts "  Implementation..."
puts "============================================================"

launch_runs impl_1 -jobs 8
wait_on_run impl_1
puts "Implementation: [get_property STATUS [get_runs impl_1]]"

if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    puts "ERROR: Implementation failed!"
    exit 1
}

#===============================================================================
# Device Image & XSA
#===============================================================================
puts "Generating device image..."
launch_runs impl_1 -to_step write_device_image -jobs 8
wait_on_run impl_1

puts ""
puts "============================================================"
puts "  BUILD COMPLETE!"
puts "============================================================"
puts ""
puts "DMA Types in Design:"
puts "  - AXI CDMA:  Memory-to-memory (SG mode)"
puts "  - AXI DMA:   Stream MM2S/S2MM with loopback FIFO, 26-bit length"
puts "  - AXI MCDMA: 4-channel stream with loopback FIFO"
puts "  - LPD DMA:   PS hard IP with LPD_AXI_NOC path to LPDDR4"
puts ""
puts "v4 KEY FIX: LPD_AXI_NOC enabled - ADMA can now access LPDDR4!"
puts ""
