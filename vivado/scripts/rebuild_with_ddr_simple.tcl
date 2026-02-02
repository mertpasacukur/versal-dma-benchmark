################################################################################
# Versal DMA Benchmark - Simple DDR4 Design
# Target: VPK120 (Versal Premium VP1202)
# Vivado 2023.2
#
# Uses CIPS internal DDR controller (not NoC DDR4 PHY)
# Software runs from DDR through PS memory subsystem
################################################################################

set project_dir [file normalize [file dirname [info script]]/../..]
set output_dir "${project_dir}/vivado/output"
set project_name "versal_dma_benchmark"

puts "============================================================"
puts "    REBUILDING PROJECT WITH DDR4 SUPPORT (SIMPLE)"
puts "============================================================"

################################################################################
# Remove Old Project and Create New
################################################################################

puts "INFO: Removing old project..."
if {[file exists "${output_dir}/${project_name}"]} {
    file delete -force "${output_dir}/${project_name}"
}

puts "INFO: Creating new project..."
create_project ${project_name} "${output_dir}/${project_name}" -part xcvp1202-vsva2785-2MP-e-S

set_property target_language Verilog [current_project]

################################################################################
# Create Block Design
################################################################################

set bd_name "dma_benchmark_bd"
puts "INFO: Creating block design: ${bd_name}"
create_bd_design ${bd_name}

################################################################################
# Add CIPS with PS DDR enabled
################################################################################

puts "INFO: Adding CIPS..."
create_bd_cell -type ip -vlnv xilinx.com:ip:versal_cips:3.4 versal_cips_0

# Configure CIPS with PS DDR, UART, and PL interfaces
# This uses the PS built-in DDR controller, NOT NoC DDR4 PHY
set_property -dict [list \
    CONFIG.PS_PMC_CONFIG { \
        PMC_CRP_PL0_REF_CTRL_FREQMHZ {100} \
        PMC_CRP_PL1_REF_CTRL_FREQMHZ {250} \
        PS_GEN_IPI0_ENABLE {1} \
        PS_GEN_IPI1_ENABLE {1} \
        PS_GEN_IPI2_ENABLE {1} \
        PS_GEN_IPI3_ENABLE {1} \
        PS_GEN_IPI4_ENABLE {1} \
        PS_GEN_IPI5_ENABLE {1} \
        PS_GEN_IPI6_ENABLE {1} \
        PS_IRQ_USAGE {{CH0 1} {CH1 1} {CH10 0} {CH11 0} {CH12 0} {CH13 0} {CH14 0} {CH15 0} {CH2 0} {CH3 0} {CH4 0} {CH5 0} {CH6 0} {CH7 0} {CH8 0} {CH9 0}} \
        PS_NUM_FABRIC_RESETS {1} \
        PS_UART0_BAUD_RATE {115200} \
        PS_UART0_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 42 .. 43}}} \
        PS_USE_M_AXI_FPD {1} \
        PS_USE_S_AXI_FPD {1} \
        PS_USE_PMCPL_CLK0 {1} \
        PS_USE_PMCPL_CLK1 {1} \
        PS_TTC0_PERIPHERAL_ENABLE {1} \
        PS_TTC1_PERIPHERAL_ENABLE {1} \
        PS_TTC2_PERIPHERAL_ENABLE {1} \
        PS_TTC3_PERIPHERAL_ENABLE {1} \
        SMON_ALARMS {Set_Coverage_Alarms} \
    } \
] [get_bd_cells versal_cips_0]

puts "INFO: CIPS configured with UART0 and PL interfaces"
puts "INFO: DDR4 memory will be accessible via PS memory subsystem at boot"

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
# AXI Stream FIFO for DMA Loopback
################################################################################

puts "INFO: Adding AXI Stream FIFO for loopback..."
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
# SmartConnect for DMA Data Path to PS Memory
################################################################################

puts "INFO: Adding SmartConnect for DMA data path..."
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:1.0 axi_smc_dma

set_property -dict [list \
    CONFIG.NUM_SI {5} \
    CONFIG.NUM_MI {1} \
    CONFIG.NUM_CLKS {1} \
] [get_bd_cells axi_smc_dma]

connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins axi_smc_dma/aclk]
connect_bd_net [get_bd_pins proc_sys_reset_0/interconnect_aresetn] [get_bd_pins axi_smc_dma/aresetn]

# Connect DMA master ports to SmartConnect
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXI_SG] [get_bd_intf_pins axi_smc_dma/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXI_MM2S] [get_bd_intf_pins axi_smc_dma/S01_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXI_S2MM] [get_bd_intf_pins axi_smc_dma/S02_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_cdma_0/M_AXI] [get_bd_intf_pins axi_smc_dma/S03_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_cdma_0/M_AXI_SG] [get_bd_intf_pins axi_smc_dma/S04_AXI]

# Connect SmartConnect to CIPS S_AXI_FPD for memory access
# This gives DMA engines access to PS memory space (including DDR)
connect_bd_intf_net [get_bd_intf_pins axi_smc_dma/M00_AXI] [get_bd_intf_pins versal_cips_0/S_AXI_FPD]
connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins versal_cips_0/s_axi_fpd_aclk]

################################################################################
# Interrupt Connections
################################################################################

puts "INFO: Connecting interrupts..."
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 xlconcat_0
set_property -dict [list CONFIG.NUM_PORTS {3}] [get_bd_cells xlconcat_0]

connect_bd_net [get_bd_pins axi_dma_0/mm2s_introut] [get_bd_pins xlconcat_0/In0]
connect_bd_net [get_bd_pins axi_dma_0/s2mm_introut] [get_bd_pins xlconcat_0/In1]
connect_bd_net [get_bd_pins axi_cdma_0/cdma_introut] [get_bd_pins xlconcat_0/In2]
connect_bd_net [get_bd_pins xlconcat_0/dout] [get_bd_pins versal_cips_0/pl_ps_irq0]

################################################################################
# Address Assignment
################################################################################

puts "INFO: Assigning addresses..."
assign_bd_address

################################################################################
# Validate and Save Design
################################################################################

puts "INFO: Validating block design..."
validate_bd_design

puts "INFO: Saving block design..."
save_bd_design

puts "INFO: Creating HDL wrapper..."
make_wrapper -files [get_files ${output_dir}/${project_name}/${project_name}.srcs/sources_1/bd/${bd_name}/${bd_name}.bd] -top
add_files -norecurse ${output_dir}/${project_name}/${project_name}.gen/sources_1/bd/${bd_name}/hdl/${bd_name}_wrapper.v

################################################################################
# Synthesis
################################################################################

puts "INFO: Running Synthesis..."
launch_runs synth_1 -jobs 8
wait_on_run synth_1

if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    puts "ERROR: Synthesis failed!"
    exit 1
}
puts "INFO: Synthesis completed"

################################################################################
# Implementation
################################################################################

puts "INFO: Running Implementation..."
launch_runs impl_1 -jobs 8
wait_on_run impl_1

if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    puts "ERROR: Implementation failed!"
    exit 1
}
puts "INFO: Implementation completed"

################################################################################
# Generate Hardware Platform (XSA)
################################################################################

puts "INFO: Generating Hardware Platform..."
open_run impl_1

# Set platform properties
set_property platform.default_output_type "sd_card" [current_project]

# Export XSA
write_hw_platform -fixed -include_bit -force ${output_dir}/${project_name}_with_ddr.xsa

# Generate PDI
write_device_image -force ${output_dir}/${project_name}_with_ddr.pdi

puts "============================================================"
puts "    BUILD COMPLETE"
puts "============================================================"
puts "XSA: ${output_dir}/${project_name}_with_ddr.xsa"
puts "PDI: ${output_dir}/${project_name}_with_ddr.pdi"
puts ""
puts "NOTE: DDR4 memory is available through PS memory subsystem."
puts "      Configure linker script to place code/data in DDR."
puts "      DDR4 on VPK120 is at address 0x0 (PS-DDR region)"
puts "============================================================"
