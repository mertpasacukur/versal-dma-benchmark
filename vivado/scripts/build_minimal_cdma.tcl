#===============================================================================
# Versal DMA - Minimal CDMA Build Script
# VPK120 (VP1202) - Simplest possible working design
# CIPS + NoC (no DDR) + CDMA for OCM access
#===============================================================================

set script_dir [file dirname [file normalize [info script]]]
set project_dir [file normalize "$script_dir/.."]
set project_name "versal_minimal"

puts "============================================================"
puts "  Versal Minimal CDMA Build - VP1202"
puts "  CIPS + CDMA accessing OCM"
puts "============================================================"

# Cleanup
catch {close_project}
if {[file exists "$project_dir/$project_name"]} {
    file delete -force "$project_dir/$project_name"
}

create_project $project_name "$project_dir/$project_name" -part xcvp1202-vsva2785-2MP-e-S
create_bd_design "design_1"

#-------------------------------------------------------------------------------
# CIPS - Minimal configuration
#-------------------------------------------------------------------------------
puts "Adding CIPS..."
create_bd_cell -type ip -vlnv xilinx.com:ip:versal_cips:* cips_0

# Minimal CIPS - just PS, no DDR through NoC
set_property -dict [list \
    CONFIG.PS_PMC_CONFIG { \
        DEBUG_MODE {JTAG} \
        DESIGN_MODE {1} \
        PMC_CRP_PL0_REF_CTRL_FREQMHZ {100} \
        PS_BOARD_INTERFACE {Custom} \
        PS_NUM_FABRIC_RESETS {1} \
        PS_UART0_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 42 .. 43}}} \
        PS_USE_M_AXI_FPD {1} \
        PS_USE_S_AXI_FPD {1} \
        PS_USE_PMCPL_CLK0 {1} \
        PS_IRQ_USAGE {{CH0 1} {CH1 0} {CH10 0} {CH11 0} {CH12 0} {CH13 0} {CH14 0} {CH15 0} {CH2 0} {CH3 0} {CH4 0} {CH5 0} {CH6 0} {CH7 0} {CH8 0} {CH9 0}} \
    } \
] [get_bd_cells cips_0]

puts "CIPS configured"

#-------------------------------------------------------------------------------
# CDMA
#-------------------------------------------------------------------------------
puts "Adding CDMA..."
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_cdma:* axi_cdma_0

set_property -dict [list \
    CONFIG.C_INCLUDE_SG {0} \
    CONFIG.C_M_AXI_DATA_WIDTH {64} \
    CONFIG.C_M_AXI_MAX_BURST_LEN {64} \
] [get_bd_cells axi_cdma_0]

#-------------------------------------------------------------------------------
# SmartConnect for PS to CDMA control
#-------------------------------------------------------------------------------
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:* axi_smc_ctrl
set_property -dict [list \
    CONFIG.NUM_SI {1} \
    CONFIG.NUM_MI {1} \
] [get_bd_cells axi_smc_ctrl]

#-------------------------------------------------------------------------------
# SmartConnect for CDMA data to PS (S_AXI_FPD)
#-------------------------------------------------------------------------------
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:* axi_smc_data
set_property -dict [list \
    CONFIG.NUM_SI {1} \
    CONFIG.NUM_MI {1} \
] [get_bd_cells axi_smc_data]

#-------------------------------------------------------------------------------
# Reset
#-------------------------------------------------------------------------------
create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:* rst_0

#-------------------------------------------------------------------------------
# Connections
#-------------------------------------------------------------------------------
puts "Making connections..."

# PL clock from CIPS
set pl_clk [get_bd_pins cips_0/pl0_ref_clk]
set pl_rst [get_bd_pins cips_0/pl0_resetn]

# Reset module
connect_bd_net $pl_clk [get_bd_pins rst_0/slowest_sync_clk]
connect_bd_net $pl_rst [get_bd_pins rst_0/ext_reset_in]

# SmartConnect clocks
connect_bd_net $pl_clk [get_bd_pins axi_smc_ctrl/aclk]
connect_bd_net $pl_clk [get_bd_pins axi_smc_data/aclk]
connect_bd_net [get_bd_pins rst_0/peripheral_aresetn] [get_bd_pins axi_smc_ctrl/aresetn]
connect_bd_net [get_bd_pins rst_0/peripheral_aresetn] [get_bd_pins axi_smc_data/aresetn]

# CDMA clocks
connect_bd_net $pl_clk [get_bd_pins axi_cdma_0/m_axi_aclk]
connect_bd_net $pl_clk [get_bd_pins axi_cdma_0/s_axi_lite_aclk]
connect_bd_net [get_bd_pins rst_0/peripheral_aresetn] [get_bd_pins axi_cdma_0/s_axi_lite_aresetn]

# CIPS AXI clocks
connect_bd_net $pl_clk [get_bd_pins cips_0/m_axi_fpd_aclk]
connect_bd_net $pl_clk [get_bd_pins cips_0/s_axi_fpd_aclk]

# PS FPD Master -> SmartConnect -> CDMA Control
connect_bd_intf_net [get_bd_intf_pins cips_0/M_AXI_FPD] [get_bd_intf_pins axi_smc_ctrl/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_smc_ctrl/M00_AXI] [get_bd_intf_pins axi_cdma_0/S_AXI_LITE]

# CDMA Data -> SmartConnect -> PS FPD Slave (access to OCM, DDR via PS)
connect_bd_intf_net [get_bd_intf_pins axi_cdma_0/M_AXI] [get_bd_intf_pins axi_smc_data/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_smc_data/M00_AXI] [get_bd_intf_pins cips_0/S_AXI_FPD]

# Interrupt
connect_bd_net [get_bd_pins axi_cdma_0/cdma_introut] [get_bd_pins cips_0/pl_ps_irq0]

puts "Connections complete"

#-------------------------------------------------------------------------------
# Finalize
#-------------------------------------------------------------------------------
regenerate_bd_layout
assign_bd_address

save_bd_design
puts "Validating..."
validate_bd_design
save_bd_design

#-------------------------------------------------------------------------------
# Generate
#-------------------------------------------------------------------------------
puts "Generating output products..."
generate_target all [get_files design_1.bd]

make_wrapper -files [get_files design_1.bd] -top
add_files -norecurse "$project_dir/$project_name/$project_name.gen/sources_1/bd/design_1/hdl/design_1_wrapper.v"
update_compile_order -fileset sources_1

#-------------------------------------------------------------------------------
# Synthesis
#-------------------------------------------------------------------------------
puts ""
puts "============================================================"
puts "  Synthesis..."
puts "============================================================"

launch_runs synth_1 -jobs 8
wait_on_run synth_1
puts "Synthesis: [get_property STATUS [get_runs synth_1]]"

#-------------------------------------------------------------------------------
# Implementation
#-------------------------------------------------------------------------------
puts ""
puts "============================================================"
puts "  Implementation..."
puts "============================================================"

launch_runs impl_1 -jobs 8
wait_on_run impl_1
puts "Implementation: [get_property STATUS [get_runs impl_1]]"

#-------------------------------------------------------------------------------
# Device Image & XSA
#-------------------------------------------------------------------------------
puts "Generating device image..."
launch_runs impl_1 -to_step write_device_image -jobs 8
wait_on_run impl_1

set xsa_path "$project_dir/design_minimal.xsa"
write_hw_platform -fixed -include_bit -force -file $xsa_path

puts ""
puts "============================================================"
puts "  BUILD COMPLETE!"
puts "============================================================"
puts "XSA: $xsa_path"
puts ""
puts "Memory Access:"
puts "  OCM: 0xFFFC_0000 - 0xFFFF_FFFF (256KB)"
puts "  DDR: 0x0000_0000 - 0x7FFF_FFFF (via PS)"
puts ""
report_bd_addr_seg
