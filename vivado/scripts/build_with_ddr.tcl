#===============================================================================
# Versal DMA - Build with PS DDR (LPDDR4)
# VPK120 (VP1202) - Uses CIPS integrated DDR controller
# No external pin constraints needed - PS DDR is hardwired
#===============================================================================

set script_dir [file dirname [file normalize [info script]]]
set project_dir [file normalize "$script_dir/.."]
set project_name "versal_ddr"

puts "============================================================"
puts "  Versal DDR Build - VP1202"
puts "  CIPS + NoC + LPDDR4 + CDMA"
puts "============================================================"

# Cleanup
catch {close_project}
if {[file exists "$project_dir/$project_name"]} {
    file delete -force "$project_dir/$project_name"
}

create_project $project_name "$project_dir/$project_name" -part xcvp1202-vsva2785-2MP-e-S
create_bd_design "design_1"

#-------------------------------------------------------------------------------
# CIPS - With integrated LPDDR4
#-------------------------------------------------------------------------------
puts "Adding CIPS with LPDDR4..."
create_bd_cell -type ip -vlnv xilinx.com:ip:versal_cips:* cips_0

# Enable PS DDR (LPDDR4) through CIPS - this is hardwired, no PL constraints needed
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
        PS_IRQ_USAGE {{CH0 1} {CH1 0} {CH10 0} {CH11 0} {CH12 0} {CH13 0} {CH14 0} {CH15 0} {CH2 0} {CH3 0} {CH4 0} {CH5 0} {CH6 0} {CH7 0} {CH8 0} {CH9 0}} \
    } \
    CONFIG.DDR_MEMORY_MODE {Connectivity to DDR via NOC} \
] [get_bd_cells cips_0]

puts "CIPS configured with DDR"

#-------------------------------------------------------------------------------
# NoC - Integrated LPDDR4 Memory Controller
#-------------------------------------------------------------------------------
puts "Adding AXI NoC with LPDDR4..."
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_noc:* axi_noc_0

# Configure NoC with integrated LPDDR4 controller
set_property -dict [list \
    CONFIG.CH0_LPDDR4_0_BOARD_INTERFACE {Custom} \
    CONFIG.CONTROLLERTYPE {LPDDR4_SDRAM} \
    CONFIG.MC0_FLIPPED_PINOUT {true} \
    CONFIG.MC_CHAN_REGION0 {DDR_CH1} \
    CONFIG.MC_CHAN_REGION1 {NONE} \
    CONFIG.MC_COMPONENT_WIDTH {x32} \
    CONFIG.MC_DATAWIDTH {32} \
    CONFIG.MC_INTERLEAVE_SIZE {128} \
    CONFIG.MC_LPDDR4_REFRESH_TYPE {PER_BANK} \
    CONFIG.MC_LP4_OVERWRITE_IO_PROP {true} \
    CONFIG.MC_LP4_PIN_EFFICIENT {true} \
    CONFIG.MC_MEMORY_SPEEDGRADE {LPDDR4-4267} \
    CONFIG.MC_NO_CHANNELS {Single} \
    CONFIG.MC_RANK {1} \
    CONFIG.MC_ROWADDRESSWIDTH {16} \
    CONFIG.MC_STACKHEIGHT {1} \
    CONFIG.MC_SYSTEM_CLOCK {Differential} \
    CONFIG.NUM_CLKS {9} \
    CONFIG.NUM_MC {1} \
    CONFIG.NUM_MCP {4} \
    CONFIG.NUM_MI {0} \
    CONFIG.NUM_NSI {0} \
    CONFIG.NUM_SI {8} \
    CONFIG.sys_clk0_BOARD_INTERFACE {Custom} \
] [get_bd_cells axi_noc_0]

# Configure slave interfaces - PS connections
set_property -dict [list CONFIG.CATEGORY {ps_cci} CONFIG.CONNECTIONS {MC_0 {read_bw {500} write_bw {500} read_avg_burst {4} write_avg_burst {4}}}] [get_bd_intf_pins /axi_noc_0/S00_AXI]
set_property -dict [list CONFIG.CATEGORY {ps_cci} CONFIG.CONNECTIONS {MC_0 {read_bw {500} write_bw {500} read_avg_burst {4} write_avg_burst {4}}}] [get_bd_intf_pins /axi_noc_0/S01_AXI]
set_property -dict [list CONFIG.CATEGORY {ps_cci} CONFIG.CONNECTIONS {MC_0 {read_bw {500} write_bw {500} read_avg_burst {4} write_avg_burst {4}}}] [get_bd_intf_pins /axi_noc_0/S02_AXI]
set_property -dict [list CONFIG.CATEGORY {ps_cci} CONFIG.CONNECTIONS {MC_0 {read_bw {500} write_bw {500} read_avg_burst {4} write_avg_burst {4}}}] [get_bd_intf_pins /axi_noc_0/S03_AXI]
set_property -dict [list CONFIG.CATEGORY {ps_nci} CONFIG.CONNECTIONS {MC_0 {read_bw {500} write_bw {500} read_avg_burst {4} write_avg_burst {4}}}] [get_bd_intf_pins /axi_noc_0/S04_AXI]
set_property -dict [list CONFIG.CATEGORY {ps_nci} CONFIG.CONNECTIONS {MC_0 {read_bw {500} write_bw {500} read_avg_burst {4} write_avg_burst {4}}}] [get_bd_intf_pins /axi_noc_0/S05_AXI]
set_property -dict [list CONFIG.CATEGORY {ps_rpu} CONFIG.CONNECTIONS {MC_0 {read_bw {500} write_bw {500} read_avg_burst {4} write_avg_burst {4}}}] [get_bd_intf_pins /axi_noc_0/S06_AXI]
set_property -dict [list CONFIG.CATEGORY {ps_pmc} CONFIG.CONNECTIONS {MC_0 {read_bw {500} write_bw {500} read_avg_burst {4} write_avg_burst {4}}}] [get_bd_intf_pins /axi_noc_0/S07_AXI]

# Clock associations for each interface
set_property -dict [list CONFIG.ASSOCIATED_BUSIF {S00_AXI}] [get_bd_pins /axi_noc_0/aclk0]
set_property -dict [list CONFIG.ASSOCIATED_BUSIF {S01_AXI}] [get_bd_pins /axi_noc_0/aclk1]
set_property -dict [list CONFIG.ASSOCIATED_BUSIF {S02_AXI}] [get_bd_pins /axi_noc_0/aclk2]
set_property -dict [list CONFIG.ASSOCIATED_BUSIF {S03_AXI}] [get_bd_pins /axi_noc_0/aclk3]
set_property -dict [list CONFIG.ASSOCIATED_BUSIF {S04_AXI}] [get_bd_pins /axi_noc_0/aclk4]
set_property -dict [list CONFIG.ASSOCIATED_BUSIF {S05_AXI}] [get_bd_pins /axi_noc_0/aclk5]
set_property -dict [list CONFIG.ASSOCIATED_BUSIF {S06_AXI}] [get_bd_pins /axi_noc_0/aclk6]
set_property -dict [list CONFIG.ASSOCIATED_BUSIF {S07_AXI}] [get_bd_pins /axi_noc_0/aclk7]

puts "NoC configured with LPDDR4"

#-------------------------------------------------------------------------------
# Second NoC for PL access to DDR
#-------------------------------------------------------------------------------
puts "Adding second NoC for PL DDR access..."
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_noc:* axi_noc_1

set_property -dict [list \
    CONFIG.NUM_CLKS {1} \
    CONFIG.NUM_MC {0} \
    CONFIG.NUM_MCP {0} \
    CONFIG.NUM_MI {0} \
    CONFIG.NUM_NMI {1} \
    CONFIG.NUM_NSI {1} \
    CONFIG.NUM_SI {1} \
] [get_bd_cells axi_noc_1]

set_property -dict [list CONFIG.CATEGORY {pl} CONFIG.CONNECTIONS {M00_INI {read_bw {500} write_bw {500}}}] [get_bd_intf_pins /axi_noc_1/S00_AXI]
set_property -dict [list CONFIG.ASSOCIATED_BUSIF {S00_AXI}] [get_bd_pins /axi_noc_1/aclk0]

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
# SmartConnects
#-------------------------------------------------------------------------------
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:* axi_smc_ctrl
set_property -dict [list CONFIG.NUM_SI {1} CONFIG.NUM_MI {1}] [get_bd_cells axi_smc_ctrl]

create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:* axi_smc_data
set_property -dict [list CONFIG.NUM_SI {1} CONFIG.NUM_MI {1}] [get_bd_cells axi_smc_data]

#-------------------------------------------------------------------------------
# Reset
#-------------------------------------------------------------------------------
create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:* rst_0

#-------------------------------------------------------------------------------
# Connections
#-------------------------------------------------------------------------------
puts "Making connections..."

# PL clock
set pl_clk [get_bd_pins cips_0/pl0_ref_clk]
set pl_rst [get_bd_pins cips_0/pl0_resetn]

# Reset module
connect_bd_net $pl_clk [get_bd_pins rst_0/slowest_sync_clk]
connect_bd_net $pl_rst [get_bd_pins rst_0/ext_reset_in]

# CIPS to main NoC (PS DDR access)
connect_bd_intf_net [get_bd_intf_pins cips_0/FPD_CCI_NOC_0] [get_bd_intf_pins axi_noc_0/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins cips_0/FPD_CCI_NOC_1] [get_bd_intf_pins axi_noc_0/S01_AXI]
connect_bd_intf_net [get_bd_intf_pins cips_0/FPD_CCI_NOC_2] [get_bd_intf_pins axi_noc_0/S02_AXI]
connect_bd_intf_net [get_bd_intf_pins cips_0/FPD_CCI_NOC_3] [get_bd_intf_pins axi_noc_0/S03_AXI]
connect_bd_intf_net [get_bd_intf_pins cips_0/FPD_AXI_NOC_0] [get_bd_intf_pins axi_noc_0/S04_AXI]
connect_bd_intf_net [get_bd_intf_pins cips_0/FPD_AXI_NOC_1] [get_bd_intf_pins axi_noc_0/S05_AXI]
connect_bd_intf_net [get_bd_intf_pins cips_0/NOC_LPD_AXI_0] [get_bd_intf_pins axi_noc_0/S06_AXI]
connect_bd_intf_net [get_bd_intf_pins cips_0/PMC_NOC_AXI_0] [get_bd_intf_pins axi_noc_0/S07_AXI]

# NoC clocks from CIPS
connect_bd_net [get_bd_pins cips_0/fpd_cci_noc_axi0_clk] [get_bd_pins axi_noc_0/aclk0]
connect_bd_net [get_bd_pins cips_0/fpd_cci_noc_axi1_clk] [get_bd_pins axi_noc_0/aclk1]
connect_bd_net [get_bd_pins cips_0/fpd_cci_noc_axi2_clk] [get_bd_pins axi_noc_0/aclk2]
connect_bd_net [get_bd_pins cips_0/fpd_cci_noc_axi3_clk] [get_bd_pins axi_noc_0/aclk3]
connect_bd_net [get_bd_pins cips_0/fpd_axi_noc_axi0_clk] [get_bd_pins axi_noc_0/aclk4]
connect_bd_net [get_bd_pins cips_0/fpd_axi_noc_axi1_clk] [get_bd_pins axi_noc_0/aclk5]
connect_bd_net [get_bd_pins cips_0/lpd_axi_noc_clk] [get_bd_pins axi_noc_0/aclk6]
connect_bd_net [get_bd_pins cips_0/pmc_axi_noc_axi0_clk] [get_bd_pins axi_noc_0/aclk7]

# PL NoC connection
connect_bd_net $pl_clk [get_bd_pins axi_noc_1/aclk0]

# NoC INI interconnect (PL NoC to main NoC)
connect_bd_intf_net [get_bd_intf_pins axi_noc_1/M00_INI] [get_bd_intf_pins axi_noc_0/S00_INI]

# SmartConnect clocks
connect_bd_net $pl_clk [get_bd_pins axi_smc_ctrl/aclk]
connect_bd_net $pl_clk [get_bd_pins axi_smc_data/aclk]
connect_bd_net [get_bd_pins rst_0/peripheral_aresetn] [get_bd_pins axi_smc_ctrl/aresetn]
connect_bd_net [get_bd_pins rst_0/peripheral_aresetn] [get_bd_pins axi_smc_data/aresetn]

# CDMA clocks
connect_bd_net $pl_clk [get_bd_pins axi_cdma_0/m_axi_aclk]
connect_bd_net $pl_clk [get_bd_pins axi_cdma_0/s_axi_lite_aclk]
connect_bd_net [get_bd_pins rst_0/peripheral_aresetn] [get_bd_pins axi_cdma_0/s_axi_lite_aresetn]

# CIPS M_AXI_FPD clock
connect_bd_net $pl_clk [get_bd_pins cips_0/m_axi_fpd_aclk]

# PS to CDMA control path
connect_bd_intf_net [get_bd_intf_pins cips_0/M_AXI_FPD] [get_bd_intf_pins axi_smc_ctrl/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_smc_ctrl/M00_AXI] [get_bd_intf_pins axi_cdma_0/S_AXI_LITE]

# CDMA to PL NoC (for DDR access)
connect_bd_intf_net [get_bd_intf_pins axi_cdma_0/M_AXI] [get_bd_intf_pins axi_smc_data/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_smc_data/M00_AXI] [get_bd_intf_pins axi_noc_1/S00_AXI]

# Interrupt
connect_bd_net [get_bd_pins axi_cdma_0/cdma_introut] [get_bd_pins cips_0/pl_ps_irq0]

# External LPDDR4 interface
make_bd_intf_pins_external [get_bd_intf_pins axi_noc_0/CH0_LPDDR4_0]
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

set xsa_path "$project_dir/design_ddr.xsa"
write_hw_platform -fixed -include_bit -force -file $xsa_path

puts ""
puts "============================================================"
puts "  BUILD COMPLETE!"
puts "============================================================"
puts "XSA: $xsa_path"
puts ""
puts "Memory Map:"
puts "  LPDDR4: 0x0000_0000 - 0x7FFF_FFFF (2GB)"
puts "  OCM:    0xFFFC_0000 - 0xFFFF_FFFF (256KB)"
puts "  CDMA:   0xA400_0000 (control registers)"
puts ""
