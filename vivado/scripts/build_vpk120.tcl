#===============================================================================
# Versal DMA - VPK120 Board Build
# Uses VPK120 board files for automatic DDR configuration
#===============================================================================

set script_dir [file dirname [file normalize [info script]]]
set project_dir [file normalize "$script_dir/.."]
set project_name "versal_vpk120"

puts "============================================================"
puts "  Versal VPK120 Build"
puts "  Using board automation for LPDDR4"
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

#-------------------------------------------------------------------------------
# CIPS with board preset
#-------------------------------------------------------------------------------
puts "Adding CIPS..."
create_bd_cell -type ip -vlnv xilinx.com:ip:versal_cips:* cips_0

# Apply board automation for CIPS
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

puts "CIPS configured with board preset"

#-------------------------------------------------------------------------------
# Enable M_AXI_FPD for CDMA control
#-------------------------------------------------------------------------------
set_property -dict [list \
    CONFIG.PS_PMC_CONFIG { \
        PS_USE_M_AXI_FPD {1} \
        PS_IRQ_USAGE {{CH0 1} {CH1 0} {CH10 0} {CH11 0} {CH12 0} {CH13 0} {CH14 0} {CH15 0} {CH2 0} {CH3 0} {CH4 0} {CH5 0} {CH6 0} {CH7 0} {CH8 0} {CH9 0}} \
    } \
] [get_bd_cells cips_0]

#-------------------------------------------------------------------------------
# CDMA
#-------------------------------------------------------------------------------
puts "Adding CDMA..."
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_cdma:* axi_cdma_0

set_property -dict [list \
    CONFIG.C_INCLUDE_SG {0} \
    CONFIG.C_M_AXI_DATA_WIDTH {128} \
    CONFIG.C_M_AXI_MAX_BURST_LEN {256} \
] [get_bd_cells axi_cdma_0]

#-------------------------------------------------------------------------------
# SmartConnect for PS to CDMA control
#-------------------------------------------------------------------------------
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:* axi_smc_ctrl
set_property -dict [list CONFIG.NUM_SI {1} CONFIG.NUM_MI {1}] [get_bd_cells axi_smc_ctrl]

#-------------------------------------------------------------------------------
# SmartConnect for CDMA data
#-------------------------------------------------------------------------------
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:* axi_smc_data
set_property -dict [list CONFIG.NUM_SI {1} CONFIG.NUM_MI {1}] [get_bd_cells axi_smc_data]

#-------------------------------------------------------------------------------
# Reset
#-------------------------------------------------------------------------------
create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:* rst_pl

#-------------------------------------------------------------------------------
# Get NoC created by board automation
#-------------------------------------------------------------------------------
set noc_cell [get_bd_cells -filter {VLNV =~ *axi_noc*}]
puts "NoC cell: $noc_cell"

# Add one more slave interface to NoC for PL CDMA
set current_si [get_property CONFIG.NUM_SI [get_bd_cells $noc_cell]]
set new_si [expr $current_si + 1]
set_property CONFIG.NUM_SI $new_si [get_bd_cells $noc_cell]

# Get the new slave interface name
set new_si_name "S[format %02d $current_si]_AXI"
puts "Adding NoC interface: $new_si_name"

# Configure new slave interface for PL
set_property -dict [list \
    CONFIG.CONNECTIONS {MC_0 {read_bw {500} write_bw {500} read_avg_burst {4} write_avg_burst {4}}} \
    CONFIG.CATEGORY {pl} \
] [get_bd_intf_pins $noc_cell/$new_si_name]

# Add clock for new interface
set current_clks [get_property CONFIG.NUM_CLKS [get_bd_cells $noc_cell]]
set new_clks [expr $current_clks + 1]
set_property CONFIG.NUM_CLKS $new_clks [get_bd_cells $noc_cell]

set new_clk_name "aclk[expr $current_clks]"
set_property -dict [list CONFIG.ASSOCIATED_BUSIF $new_si_name] [get_bd_pins $noc_cell/$new_clk_name]

#-------------------------------------------------------------------------------
# Connections
#-------------------------------------------------------------------------------
puts "Making connections..."

# PL clock
set pl_clk [get_bd_pins cips_0/pl0_ref_clk]
set pl_rst [get_bd_pins cips_0/pl0_resetn]

# Reset module
connect_bd_net $pl_clk [get_bd_pins rst_pl/slowest_sync_clk]
connect_bd_net $pl_rst [get_bd_pins rst_pl/ext_reset_in]

# SmartConnect clocks
connect_bd_net $pl_clk [get_bd_pins axi_smc_ctrl/aclk]
connect_bd_net $pl_clk [get_bd_pins axi_smc_data/aclk]
connect_bd_net [get_bd_pins rst_pl/peripheral_aresetn] [get_bd_pins axi_smc_ctrl/aresetn]
connect_bd_net [get_bd_pins rst_pl/peripheral_aresetn] [get_bd_pins axi_smc_data/aresetn]

# CDMA clocks
connect_bd_net $pl_clk [get_bd_pins axi_cdma_0/m_axi_aclk]
connect_bd_net $pl_clk [get_bd_pins axi_cdma_0/s_axi_lite_aclk]
connect_bd_net [get_bd_pins rst_pl/peripheral_aresetn] [get_bd_pins axi_cdma_0/s_axi_lite_aresetn]

# CIPS M_AXI_FPD clock
connect_bd_net $pl_clk [get_bd_pins cips_0/m_axi_fpd_aclk]

# NoC PL clock
connect_bd_net $pl_clk [get_bd_pins $noc_cell/$new_clk_name]

# PS to CDMA control
connect_bd_intf_net [get_bd_intf_pins cips_0/M_AXI_FPD] [get_bd_intf_pins axi_smc_ctrl/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_smc_ctrl/M00_AXI] [get_bd_intf_pins axi_cdma_0/S_AXI_LITE]

# CDMA to NoC (DDR access)
connect_bd_intf_net [get_bd_intf_pins axi_cdma_0/M_AXI] [get_bd_intf_pins axi_smc_data/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_smc_data/M00_AXI] [get_bd_intf_pins $noc_cell/$new_si_name]

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

set xsa_path "$project_dir/design_vpk120.xsa"
write_hw_platform -fixed -include_bit -force -file $xsa_path

puts ""
puts "============================================================"
puts "  BUILD COMPLETE!"
puts "============================================================"
puts "XSA: $xsa_path"
puts ""
puts "Memory Map:"
puts "  LPDDR4: 0x0000_0000 - 0x7FFF_FFFF (2GB)"
puts "  CDMA:   Assigned by Vivado"
puts ""
