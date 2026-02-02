################################################################################
# Versal DMA Benchmark - VPK120 LPDDR4 Design
# Target: VPK120 (Versal Premium VP1202)
# Vivado 2023.2
#
# Uses VPK120 board preset for proper LPDDR4 configuration
################################################################################

set project_dir [file normalize [file dirname [info script]]/../..]
set output_dir "${project_dir}/vivado/output"
set project_name "versal_dma_benchmark"

puts "============================================================"
puts "    REBUILDING PROJECT WITH LPDDR4 (VPK120 Board Preset)"
puts "============================================================"

################################################################################
# Remove Old Project and Create New with Board Part
################################################################################

puts "INFO: Removing old project..."
if {[file exists "${output_dir}/${project_name}"]} {
    file delete -force "${output_dir}/${project_name}"
}

puts "INFO: Creating new project with VPK120 board..."
create_project ${project_name} "${output_dir}/${project_name}" -part xcvp1202-vsva2785-2MP-e-S

# Set board part for VPK120
set_property board_part xilinx.com:vpk120:part0:1.2 [current_project]

set_property target_language Verilog [current_project]

################################################################################
# Create Block Design
################################################################################

set bd_name "dma_benchmark_bd"
puts "INFO: Creating block design: ${bd_name}"
create_bd_design ${bd_name}

################################################################################
# Add CIPS with Board Preset
################################################################################

puts "INFO: Adding CIPS with board automation..."
create_bd_cell -type ip -vlnv xilinx.com:ip:versal_cips:3.4 versal_cips_0

# Apply board preset for CIPS
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
}  [get_bd_cells versal_cips_0]

puts "INFO: Board automation applied - LPDDR4 configured"

################################################################################
# Configure CIPS additional settings
################################################################################

# Add UART and additional settings
set_property -dict [list \
    CONFIG.PS_PMC_CONFIG { \
        PS_UART0_BAUD_RATE {115200} \
        PS_UART0_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 42 .. 43}}} \
        PS_USE_M_AXI_FPD {1} \
        PS_USE_S_AXI_FPD {1} \
        PS_TTC0_PERIPHERAL_ENABLE {1} \
        PS_TTC1_PERIPHERAL_ENABLE {1} \
        PS_TTC2_PERIPHERAL_ENABLE {1} \
        PS_TTC3_PERIPHERAL_ENABLE {1} \
    } \
] [get_bd_cells versal_cips_0]

################################################################################
# Processor System Reset
################################################################################

puts "INFO: Adding Processor System Reset..."
create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 proc_sys_reset_0

connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins proc_sys_reset_0/slowest_sync_clk]
connect_bd_net [get_bd_pins versal_cips_0/pl0_resetn] [get_bd_pins proc_sys_reset_0/ext_reset_in]

################################################################################
# SmartConnect for Control Path
################################################################################

puts "INFO: Adding SmartConnect for control path..."
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:1.0 axi_smc_ctrl

set_property -dict [list \
    CONFIG.NUM_SI {1} \
    CONFIG.NUM_MI {2} \
    CONFIG.NUM_CLKS {1} \
] [get_bd_cells axi_smc_ctrl]

connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins axi_smc_ctrl/aclk]
connect_bd_net [get_bd_pins proc_sys_reset_0/interconnect_aresetn] [get_bd_pins axi_smc_ctrl/aresetn]
connect_bd_intf_net [get_bd_intf_pins versal_cips_0/M_AXI_FPD] [get_bd_intf_pins axi_smc_ctrl/S00_AXI]
connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins versal_cips_0/m_axi_fpd_aclk]

################################################################################
# AXI DMA (Scatter-Gather Mode)
################################################################################

puts "INFO: Adding AXI DMA..."
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dma:7.1 axi_dma_0

set_property -dict [list \
    CONFIG.c_include_sg {1} \
    CONFIG.c_sg_include_stscntrl_strm {0} \
    CONFIG.c_include_mm2s {1} \
    CONFIG.c_include_s2mm {1} \
    CONFIG.c_m_axi_mm2s_data_width {64} \
    CONFIG.c_m_axis_mm2s_tdata_width {64} \
    CONFIG.c_mm2s_burst_size {256} \
    CONFIG.c_m_axi_s2mm_data_width {64} \
    CONFIG.c_s_axis_s2mm_tdata_width {64} \
    CONFIG.c_s2mm_burst_size {256} \
    CONFIG.c_sg_length_width {26} \
] [get_bd_cells axi_dma_0]

connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins axi_dma_0/s_axi_lite_aclk]
connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins axi_dma_0/m_axi_sg_aclk]
connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins axi_dma_0/m_axi_mm2s_aclk]
connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins axi_dma_0/m_axi_s2mm_aclk]
connect_bd_net [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_pins axi_dma_0/axi_resetn]

connect_bd_intf_net [get_bd_intf_pins axi_smc_ctrl/M00_AXI] [get_bd_intf_pins axi_dma_0/S_AXI_LITE]

################################################################################
# AXI CDMA (Memory-to-Memory)
################################################################################

puts "INFO: Adding AXI CDMA..."
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_cdma:4.1 axi_cdma_0

set_property -dict [list \
    CONFIG.C_INCLUDE_SG {1} \
    CONFIG.C_M_AXI_DATA_WIDTH {64} \
    CONFIG.C_M_AXI_MAX_BURST_LEN {256} \
] [get_bd_cells axi_cdma_0]

connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins axi_cdma_0/s_axi_lite_aclk]
connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins axi_cdma_0/m_axi_aclk]
connect_bd_net [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_pins axi_cdma_0/s_axi_lite_aresetn]

connect_bd_intf_net [get_bd_intf_pins axi_smc_ctrl/M01_AXI] [get_bd_intf_pins axi_cdma_0/S_AXI_LITE]

################################################################################
# Loopback for DMA (MM2S -> S2MM)
################################################################################

puts "INFO: Creating AXI Stream loopback..."
create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:2.0 axis_data_fifo_0

set_property -dict [list \
    CONFIG.FIFO_DEPTH {1024} \
    CONFIG.TDATA_NUM_BYTES {8} \
] [get_bd_cells axis_data_fifo_0]

connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins axis_data_fifo_0/s_axis_aclk]
connect_bd_net [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_pins axis_data_fifo_0/s_axis_aresetn]

connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXIS_MM2S] [get_bd_intf_pins axis_data_fifo_0/S_AXIS]
connect_bd_intf_net [get_bd_intf_pins axis_data_fifo_0/M_AXIS] [get_bd_intf_pins axi_dma_0/S_AXIS_S2MM]

################################################################################
# SmartConnect for DMA Memory Access to NoC
################################################################################

puts "INFO: Adding SmartConnect for DMA memory path..."
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:1.0 axi_smc_dma

set_property -dict [list \
    CONFIG.NUM_SI {4} \
    CONFIG.NUM_MI {1} \
    CONFIG.NUM_CLKS {1} \
] [get_bd_cells axi_smc_dma]

connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins axi_smc_dma/aclk]
connect_bd_net [get_bd_pins proc_sys_reset_0/interconnect_aresetn] [get_bd_pins axi_smc_dma/aresetn]

# Connect DMA memory interfaces to SmartConnect
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXI_SG] [get_bd_intf_pins axi_smc_dma/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXI_MM2S] [get_bd_intf_pins axi_smc_dma/S01_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXI_S2MM] [get_bd_intf_pins axi_smc_dma/S02_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_cdma_0/M_AXI] [get_bd_intf_pins axi_smc_dma/S03_AXI]

# Connect SmartConnect to S_AXI_FPD for memory access through PS/NoC
connect_bd_intf_net [get_bd_intf_pins axi_smc_dma/M00_AXI] [get_bd_intf_pins versal_cips_0/S_AXI_FPD]
connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins versal_cips_0/s_axi_fpd_aclk]

################################################################################
# Address Mapping
################################################################################

puts "INFO: Assigning addresses..."
assign_bd_address

# Show address assignment complete
puts "INFO: Address assignment completed"

################################################################################
# Validate and Save Design
################################################################################

puts "INFO: Validating design..."
validate_bd_design

regenerate_bd_layout
save_bd_design

################################################################################
# Create HDL Wrapper and Generate Output Products
################################################################################

puts "INFO: Creating HDL wrapper..."
make_wrapper -files [get_files ${output_dir}/${project_name}/${project_name}.srcs/sources_1/bd/${bd_name}/${bd_name}.bd] -top
add_files -norecurse ${output_dir}/${project_name}/${project_name}.gen/sources_1/bd/${bd_name}/hdl/${bd_name}_wrapper.v

update_compile_order -fileset sources_1

puts "INFO: Generating output products..."
generate_target all [get_files ${output_dir}/${project_name}/${project_name}.srcs/sources_1/bd/${bd_name}/${bd_name}.bd]

################################################################################
# Synthesis
################################################################################

puts "INFO: Running synthesis..."
reset_run synth_1
launch_runs synth_1 -jobs 8
wait_on_run synth_1

if {[get_property STATUS [get_runs synth_1]] != "synth_design Complete!"} {
    puts "ERROR: Synthesis failed!"
    exit 1
}

puts "INFO: Synthesis completed successfully"

################################################################################
# Implementation
################################################################################

puts "INFO: Running implementation..."
launch_runs impl_1 -jobs 8
wait_on_run impl_1

if {[get_property STATUS [get_runs impl_1]] != "route_design Complete!"} {
    puts "ERROR: Implementation failed!"
    exit 1
}

puts "INFO: Implementation completed successfully"

# Check timing
open_run impl_1
set wns [get_property STATS.WNS [get_runs impl_1]]
puts "INFO: Worst Negative Slack (WNS): $wns ns"

################################################################################
# Generate XSA and PDI
################################################################################

puts "INFO: Generating XSA..."
write_hw_platform -fixed -include_bit -force ${output_dir}/${project_name}_with_lpddr4.xsa

puts "INFO: Generating PDI..."
write_device_image -force ${output_dir}/${project_name}_with_lpddr4.pdi

################################################################################
# Done
################################################################################

puts "============================================================"
puts "    BUILD COMPLETE"
puts "============================================================"
puts "XSA: ${output_dir}/${project_name}_with_lpddr4.xsa"
puts "PDI: ${output_dir}/${project_name}_with_lpddr4.pdi"
puts ""
puts "LPDDR4 memory is configured via VPK120 board preset."
puts "DDR will be visible in Vitis linker script automatically."
puts "============================================================"

close_project
