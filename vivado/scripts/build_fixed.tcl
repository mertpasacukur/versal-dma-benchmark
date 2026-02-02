#===============================================================================
# Versal DMA - Fixed Build Script
# VPK120 (VP1202)
#===============================================================================

set script_dir [file dirname [file normalize [info script]]]
set project_dir [file normalize "$script_dir/.."]
set project_name "versal_fixed"

puts "============================================================"
puts "  Versal Fixed Build - VP1202"
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

# Only enable what we need
set_property -dict [list \
    CONFIG.PS_PMC_CONFIG { \
        DDR_MEMORY_MODE {Connectivity to DDR via NOC} \
        DEBUG_MODE {JTAG} \
        DESIGN_MODE {1} \
        PMC_CRP_PL0_REF_CTRL_FREQMHZ {100} \
        PMC_USE_PMC_NOC_AXI0 {0} \
        PS_NUM_FABRIC_RESETS {1} \
        PS_UART0_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 42 .. 43}}} \
        PS_USE_FPD_AXI_NOC0 {0} \
        PS_USE_FPD_AXI_NOC1 {0} \
        PS_USE_FPD_CCI_NOC {1} \
        PS_USE_FPD_CCI_NOC0 {1} \
        PS_USE_FPD_CCI_NOC1 {0} \
        PS_USE_FPD_CCI_NOC2 {0} \
        PS_USE_FPD_CCI_NOC3 {0} \
        PS_USE_M_AXI_FPD {1} \
        PS_USE_NOC_LPD_AXI0 {0} \
        PS_USE_PMCPL_CLK0 {1} \
        PS_IRQ_USAGE {{CH0 1} {CH1 0} {CH10 0} {CH11 0} {CH12 0} {CH13 0} {CH14 0} {CH15 0} {CH2 0} {CH3 0} {CH4 0} {CH5 0} {CH6 0} {CH7 0} {CH8 0} {CH9 0}} \
    } \
] [get_bd_cells cips_0]

puts "CIPS configured"

#-------------------------------------------------------------------------------
# NoC with DDR4
#-------------------------------------------------------------------------------
puts "Adding NoC..."
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_noc:* axi_noc_0

# NoC needs 2 clocks: one from CIPS CCI, one from PL
set_property -dict [list \
    CONFIG.CONTROLLERTYPE {DDR4_SDRAM} \
    CONFIG.MC_CHAN_REGION0 {DDR_LOW0} \
    CONFIG.MC_COMPONENT_WIDTH {x16} \
    CONFIG.MC_DATAWIDTH {64} \
    CONFIG.MC_INPUTCLK0_PERIOD {5000} \
    CONFIG.MC_INPUT_FREQUENCY0 {200.000} \
    CONFIG.MC_MEMORY_SPEEDGRADE {DDR4-3200AA(22-22-22)} \
    CONFIG.MC_NO_CHANNELS {Single} \
    CONFIG.MC_RANK {1} \
    CONFIG.MC_ROWADDRESSWIDTH {16} \
    CONFIG.MC_SYSTEM_CLOCK {Differential} \
    CONFIG.NUM_CLKS {2} \
    CONFIG.NUM_MC {1} \
    CONFIG.NUM_MCP {4} \
    CONFIG.NUM_MI {0} \
    CONFIG.NUM_SI {2} \
] [get_bd_cells axi_noc_0]

# S00_AXI from CIPS - uses aclk0 (will be connected to CIPS CCI clock)
set_property -dict [list \
    CONFIG.CONNECTIONS {MC_0 {read_bw {500} write_bw {500} read_avg_burst {4} write_avg_burst {4}}} \
    CONFIG.CATEGORY {ps_cci} \
] [get_bd_intf_pins /axi_noc_0/S00_AXI]

# S01_AXI from PL CDMA - uses aclk1 (will be connected to PL clock)
set_property -dict [list \
    CONFIG.CONNECTIONS {MC_0 {read_bw {500} write_bw {500} read_avg_burst {4} write_avg_burst {4}}} \
    CONFIG.CATEGORY {pl} \
] [get_bd_intf_pins /axi_noc_0/S01_AXI]

# Associate each bus interface with its clock
set_property -dict [list CONFIG.ASSOCIATED_BUSIF {S00_AXI}] [get_bd_pins /axi_noc_0/aclk0]
set_property -dict [list CONFIG.ASSOCIATED_BUSIF {S01_AXI}] [get_bd_pins /axi_noc_0/aclk1]

puts "NoC configured"

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

# NoC clocks: aclk0 = CIPS CCI clock, aclk1 = PL clock
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

# PS FPD to SmartConnect to CDMA
connect_bd_intf_net [get_bd_intf_pins cips_0/M_AXI_FPD] [get_bd_intf_pins axi_smc_0/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_smc_0/M00_AXI] [get_bd_intf_pins axi_cdma_0/S_AXI_LITE]

# CDMA to SmartConnect to NoC
connect_bd_intf_net [get_bd_intf_pins axi_cdma_0/M_AXI] [get_bd_intf_pins axi_smc_1/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_smc_1/M00_AXI] [get_bd_intf_pins axi_noc_0/S01_AXI]

# Interrupt
connect_bd_net [get_bd_pins axi_cdma_0/cdma_introut] [get_bd_pins cips_0/pl_ps_irq0]

# External DDR interface - both data and clock
make_bd_intf_pins_external [get_bd_intf_pins axi_noc_0/CH0_DDR4_0]
make_bd_intf_pins_external [get_bd_intf_pins axi_noc_0/sys_clk0]

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
puts "  Synthesis (10-20 min)..."
puts "============================================================"

launch_runs synth_1 -jobs 8
wait_on_run synth_1
puts "Synthesis: [get_property STATUS [get_runs synth_1]]"

#-------------------------------------------------------------------------------
# Implementation
#-------------------------------------------------------------------------------
puts ""
puts "============================================================"
puts "  Implementation (20-40 min)..."
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

set xsa_path "$project_dir/design_fixed.xsa"
write_hw_platform -fixed -include_bit -force -file $xsa_path

puts ""
puts "============================================================"
puts "  BUILD COMPLETE!"
puts "============================================================"
puts "XSA: $xsa_path"
puts ""
report_bd_addr_seg
