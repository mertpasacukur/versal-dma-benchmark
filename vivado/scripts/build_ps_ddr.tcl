#===============================================================================
# Versal DMA - PS DDR Build Script
# VPK120 (VP1202) - Uses PS DDR (built-in LPDDR4) instead of PL DDR4
# This avoids needing external DDR pin constraints
#===============================================================================

set script_dir [file dirname [file normalize [info script]]]
set project_dir [file normalize "$script_dir/.."]
set project_name "versal_ps_ddr"

puts "============================================================"
puts "  Versal PS DDR Build - VP1202"
puts "  Using PS-side LPDDR4 via NoC"
puts "============================================================"

# Cleanup
catch {close_project}
if {[file exists "$project_dir/$project_name"]} {
    file delete -force "$project_dir/$project_name"
}

create_project $project_name "$project_dir/$project_name" -part xcvp1202-vsva2785-2MP-e-S
create_bd_design "design_1"

#-------------------------------------------------------------------------------
# CIPS - With DDR enabled through PS
#-------------------------------------------------------------------------------
puts "Adding CIPS with PS DDR..."
create_bd_cell -type ip -vlnv xilinx.com:ip:versal_cips:* cips_0

# Enable PS DDR and CCI for PL access to memory
set_property -dict [list \
    CONFIG.PS_PMC_CONFIG { \
        DDR_MEMORY_MODE {Connectivity to DDR via NOC} \
        DEBUG_MODE {JTAG} \
        DESIGN_MODE {1} \
        PMC_CRP_PL0_REF_CTRL_FREQMHZ {100} \
        PMC_USE_PMC_NOC_AXI0 {1} \
        PS_BOARD_INTERFACE {Custom} \
        PS_NUM_FABRIC_RESETS {1} \
        PS_UART0_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 42 .. 43}}} \
        PS_USE_FPD_AXI_NOC0 {1} \
        PS_USE_FPD_AXI_NOC1 {0} \
        PS_USE_FPD_CCI_NOC {1} \
        PS_USE_FPD_CCI_NOC0 {1} \
        PS_USE_FPD_CCI_NOC1 {0} \
        PS_USE_FPD_CCI_NOC2 {0} \
        PS_USE_FPD_CCI_NOC3 {0} \
        PS_USE_M_AXI_FPD {1} \
        PS_USE_NOC_LPD_AXI0 {1} \
        PS_USE_PMCPL_CLK0 {1} \
        PS_USE_S_AXI_FPD {0} \
        PS_IRQ_USAGE {{CH0 1} {CH1 0} {CH10 0} {CH11 0} {CH12 0} {CH13 0} {CH14 0} {CH15 0} {CH2 0} {CH3 0} {CH4 0} {CH5 0} {CH6 0} {CH7 0} {CH8 0} {CH9 0}} \
    } \
] [get_bd_cells cips_0]

puts "CIPS configured"

#-------------------------------------------------------------------------------
# NoC - Routes PL traffic to PS DDR (no MC, just routing)
#-------------------------------------------------------------------------------
puts "Adding AXI NoC for PS DDR routing..."
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_noc:* axi_noc_0

# Configure NoC as router only (no memory controller)
# NI = Network Interface, connects CIPS and PL to the NoC fabric
set_property -dict [list \
    CONFIG.NUM_CLKS {2} \
    CONFIG.NUM_MC {0} \
    CONFIG.NUM_MCP {0} \
    CONFIG.NUM_MI {0} \
    CONFIG.NUM_NMI {1} \
    CONFIG.NUM_NSI {0} \
    CONFIG.NUM_SI {2} \
] [get_bd_cells axi_noc_0]

# S00_AXI from CIPS CCI - PS accessing memory through NoC
set_property -dict [list \
    CONFIG.CONNECTIONS {M00_INI {read_bw {500} write_bw {500}}} \
    CONFIG.DEST_IDS {} \
    CONFIG.CATEGORY {ps_cci} \
] [get_bd_intf_pins /axi_noc_0/S00_AXI]

# S01_AXI from PL CDMA - PL accessing memory through NoC
set_property -dict [list \
    CONFIG.CONNECTIONS {M00_INI {read_bw {500} write_bw {500}}} \
    CONFIG.DEST_IDS {} \
    CONFIG.CATEGORY {pl} \
] [get_bd_intf_pins /axi_noc_0/S01_AXI]

# Clock associations
set_property -dict [list CONFIG.ASSOCIATED_BUSIF {S00_AXI}] [get_bd_pins /axi_noc_0/aclk0]
set_property -dict [list CONFIG.ASSOCIATED_BUSIF {S01_AXI}] [get_bd_pins /axi_noc_0/aclk1]

puts "NoC configured for PS DDR routing"

#-------------------------------------------------------------------------------
# NoC Interconnect for PS DDR
#-------------------------------------------------------------------------------
puts "Adding NoC INI for PS DDR..."
create_bd_cell -type ip -vlnv xilinx.com:ip:noc_mc_ddr4:* noc_ddr4_0

# Use LPDDR4 for PS - this is the built-in memory
apply_bd_automation -rule xilinx.com:bd_rule:noc_mc_ddr4 -config {  }  [get_bd_cells noc_ddr4_0]

puts "NoC DDR4 INI configured"

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
# SmartConnect for PS to CDMA
#-------------------------------------------------------------------------------
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:* axi_smc_0
set_property CONFIG.NUM_SI {1} [get_bd_cells axi_smc_0]
set_property CONFIG.NUM_MI {1} [get_bd_cells axi_smc_0]

#-------------------------------------------------------------------------------
# SmartConnect for CDMA to NoC
#-------------------------------------------------------------------------------
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:* axi_smc_1
set_property CONFIG.NUM_SI {1} [get_bd_cells axi_smc_1]
set_property CONFIG.NUM_MI {1} [get_bd_cells axi_smc_1]

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

# NoC clocks
connect_bd_net [get_bd_pins cips_0/fpd_cci_noc_axi0_clk] [get_bd_pins axi_noc_0/aclk0]
connect_bd_net $pl_clk [get_bd_pins axi_noc_0/aclk1]

# SmartConnect clocks
connect_bd_net $pl_clk [get_bd_pins axi_smc_0/aclk]
connect_bd_net $pl_clk [get_bd_pins axi_smc_1/aclk]
connect_bd_net [get_bd_pins rst_0/peripheral_aresetn] [get_bd_pins axi_smc_0/aresetn]
connect_bd_net [get_bd_pins rst_0/peripheral_aresetn] [get_bd_pins axi_smc_1/aresetn]

# CDMA clocks
connect_bd_net $pl_clk [get_bd_pins axi_cdma_0/m_axi_aclk]
connect_bd_net $pl_clk [get_bd_pins axi_cdma_0/s_axi_lite_aclk]
connect_bd_net [get_bd_pins rst_0/peripheral_aresetn] [get_bd_pins axi_cdma_0/s_axi_lite_aresetn]

# M_AXI_FPD clock
connect_bd_net $pl_clk [get_bd_pins cips_0/m_axi_fpd_aclk]

# CIPS CCI to NoC
connect_bd_intf_net [get_bd_intf_pins cips_0/FPD_CCI_NOC_0] [get_bd_intf_pins axi_noc_0/S00_AXI]

# NoC to DDR INI
connect_bd_intf_net [get_bd_intf_pins axi_noc_0/M00_INI] [get_bd_intf_pins noc_ddr4_0/S00_INI]

# PS FPD to SmartConnect to CDMA
connect_bd_intf_net [get_bd_intf_pins cips_0/M_AXI_FPD] [get_bd_intf_pins axi_smc_0/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_smc_0/M00_AXI] [get_bd_intf_pins axi_cdma_0/S_AXI_LITE]

# CDMA to SmartConnect to NoC
connect_bd_intf_net [get_bd_intf_pins axi_cdma_0/M_AXI] [get_bd_intf_pins axi_smc_1/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_smc_1/M00_AXI] [get_bd_intf_pins axi_noc_0/S01_AXI]

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

set xsa_path "$project_dir/design_ps_ddr.xsa"
write_hw_platform -fixed -include_bit -force -file $xsa_path

puts ""
puts "============================================================"
puts "  BUILD COMPLETE!"
puts "============================================================"
puts "XSA: $xsa_path"
puts ""
report_bd_addr_seg
