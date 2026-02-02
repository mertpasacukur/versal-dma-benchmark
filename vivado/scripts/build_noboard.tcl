#===============================================================================
# Versal DMA - Build WITHOUT Board Files
# VPK120 Development Board (VP1202) - Manual Configuration
#
# Usage: vivado -mode batch -source build_noboard.tcl
#
# This script configures LPDDR4 manually without requiring board files
#===============================================================================

set script_dir [file dirname [file normalize [info script]]]
set project_dir [file normalize "$script_dir/.."]
set project_name "versal_noboard"

puts "============================================================"
puts "  Versal Build - No Board Files Required"
puts "  Target: VPK120 (xcvp1202-vsva2785-2MP-e-S)"
puts "============================================================"

# Cleanup
catch {close_project}
if {[file exists "$project_dir/$project_name"]} {
    file delete -force "$project_dir/$project_name"
}

# Create project with just the part (no board)
create_project $project_name "$project_dir/$project_name" -part xcvp1202-vsva2785-2MP-e-S

puts "Project created for VP1202"

#-------------------------------------------------------------------------------
# Create Block Design
#-------------------------------------------------------------------------------
create_bd_design "design_1"
update_compile_order -fileset sources_1

puts "Block design created"

#-------------------------------------------------------------------------------
# Add CIPS (Versal PS)
#-------------------------------------------------------------------------------
puts "Adding CIPS..."

create_bd_cell -type ip -vlnv xilinx.com:ip:versal_cips:* cips_0

# Configure CIPS with LPDDR4 memory controller
set_property -dict [list \
    CONFIG.PS_PMC_CONFIG { \
        CLOCK_MODE {Custom} \
        DDR_MEMORY_MODE {Connectivity to DDR via NOC} \
        DEBUG_MODE {JTAG} \
        DESIGN_MODE {1} \
        PMC_CRP_PL0_REF_CTRL_FREQMHZ {100} \
        PMC_GPIO0_MIO_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 0 .. 25}}} \
        PMC_GPIO1_MIO_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 26 .. 51}}} \
        PMC_MIO37 {{AUX_IO 0} {DIRECTION out} {DRIVE_STRENGTH 8mA} {OUTPUT_DATA high} {PULL pullup} {SCHMITT 0} {SLEW slow} {USAGE GPIO}} \
        PMC_QSPI_FBCLK {{ENABLE 1} {IO {PMC_MIO 6}}} \
        PMC_QSPI_PERIPHERAL_DATA_MODE {x4} \
        PMC_QSPI_PERIPHERAL_ENABLE {1} \
        PMC_QSPI_PERIPHERAL_MODE {Dual Parallel} \
        PMC_REF_CLK_FREQMHZ {33.3333} \
        PMC_SD1 {{CD_ENABLE 1} {CD_IO {PMC_MIO 28}} {POW_ENABLE 1} {POW_IO {PMC_MIO 51}} {RESET_ENABLE 0} {RESET_IO {PMC_MIO 12}} {WP_ENABLE 0} {WP_IO {PMC_MIO 1}}} \
        PMC_SD1_PERIPHERAL {{CLK_100_SDR_OTAP_DLY 0x3} {CLK_200_SDR_OTAP_DLY 0x2} {CLK_50_DDR_ITAP_DLY 0x36} {CLK_50_DDR_OTAP_DLY 0x3} {CLK_50_SDR_ITAP_DLY 0x2C} {CLK_50_SDR_OTAP_DLY 0x4} {ENABLE 1} {IO {PMC_MIO 26 .. 36}}} \
        PMC_SD1_SLOT_TYPE {SD 3.0} \
        PMC_USE_PMC_NOC_AXI0 {1} \
        PS_BOARD_INTERFACE {Custom} \
        PS_ENET0_MDIO {{ENABLE 1} {IO {PS_MIO 24 .. 25}}} \
        PS_ENET0_PERIPHERAL {{ENABLE 1} {IO {PS_MIO 0 .. 11}}} \
        PS_GEN_IPI0_ENABLE {1} \
        PS_GEN_IPI1_ENABLE {1} \
        PS_GEN_IPI2_ENABLE {1} \
        PS_GEN_IPI3_ENABLE {1} \
        PS_GEN_IPI4_ENABLE {1} \
        PS_GEN_IPI5_ENABLE {1} \
        PS_GEN_IPI6_ENABLE {1} \
        PS_I2C0_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 46 .. 47}}} \
        PS_I2C1_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 44 .. 45}}} \
        PS_IRQ_USAGE {{CH0 1} {CH1 0} {CH10 0} {CH11 0} {CH12 0} {CH13 0} {CH14 0} {CH15 0} {CH2 0} {CH3 0} {CH4 0} {CH5 0} {CH6 0} {CH7 0} {CH8 0} {CH9 0}} \
        PS_NUM_FABRIC_RESETS {1} \
        PS_UART0_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 42 .. 43}}} \
        PS_USB3_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 13 .. 25}}} \
        PS_USE_FPD_AXI_NOC0 {1} \
        PS_USE_FPD_AXI_NOC1 {1} \
        PS_USE_FPD_CCI_NOC {1} \
        PS_USE_M_AXI_FPD {1} \
        PS_USE_NOC_LPD_AXI0 {1} \
        PS_USE_PMCPL_CLK0 {1} \
        SMON_ALARMS {Set_Coverage} \
        SMON_ENABLE_TEMP_AVERAGING {0} \
        SMON_TEMP_AVERAGING_SAMPLES {0} \
    } \
] [get_bd_cells cips_0]

puts "CIPS configured"

#-------------------------------------------------------------------------------
# Add AXI NoC with LPDDR4 Memory Controller
#-------------------------------------------------------------------------------
puts "Adding AXI NoC with LPDDR4..."

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_noc:* axi_noc_0

# Configure NoC for LPDDR4
set_property -dict [list \
    CONFIG.CH0_LPDDR4_0_BOARD_INTERFACE {Custom} \
    CONFIG.CH0_LPDDR4_1_BOARD_INTERFACE {Custom} \
    CONFIG.CH1_LPDDR4_0_BOARD_INTERFACE {Custom} \
    CONFIG.CH1_LPDDR4_1_BOARD_INTERFACE {Custom} \
    CONFIG.CONTROLLERTYPE {LPDDR4_SDRAM} \
    CONFIG.MC0_CONFIG_NUM {config26} \
    CONFIG.MC1_CONFIG_NUM {config26} \
    CONFIG.MC2_CONFIG_NUM {config26} \
    CONFIG.MC3_CONFIG_NUM {config26} \
    CONFIG.MC_ADDR_BIT9 {CA6} \
    CONFIG.MC_BA_WIDTH {3} \
    CONFIG.MC_BG_WIDTH {0} \
    CONFIG.MC_BURST_LENGTH {16} \
    CONFIG.MC_CASLATENCY {36} \
    CONFIG.MC_CASWRITELATENCY {18} \
    CONFIG.MC_CH0_LP4_CHA_ENABLE {true} \
    CONFIG.MC_CH0_LP4_CHB_ENABLE {true} \
    CONFIG.MC_CH1_LP4_CHA_ENABLE {true} \
    CONFIG.MC_CH1_LP4_CHB_ENABLE {true} \
    CONFIG.MC_CHANNEL_INTERLEAVING {true} \
    CONFIG.MC_CH_INTERLEAVING_SIZE {128_Bytes} \
    CONFIG.MC_CKE_WIDTH {0} \
    CONFIG.MC_CK_WIDTH {0} \
    CONFIG.MC_COMPONENT_DENSITY {16Gb} \
    CONFIG.MC_COMPONENT_WIDTH {x32} \
    CONFIG.MC_CONFIG_NUM {config26} \
    CONFIG.MC_DATAWIDTH {32} \
    CONFIG.MC_DDR_INIT_TIMEOUT {0x00036330} \
    CONFIG.MC_DM_WIDTH {4} \
    CONFIG.MC_DQS_WIDTH {4} \
    CONFIG.MC_DQ_WIDTH {32} \
    CONFIG.MC_ECC {false} \
    CONFIG.MC_ECC_SCRUB_PERIOD {0x004CBF} \
    CONFIG.MC_ECC_SCRUB_SIZE {4096} \
    CONFIG.MC_EN_BACKGROUND_SCRUBBING {true} \
    CONFIG.MC_EN_ECC_SCRUBBING {false} \
    CONFIG.MC_F1_CASLATENCY {36} \
    CONFIG.MC_F1_CASWRITELATENCY {18} \
    CONFIG.MC_F1_LPDDR4_MR1 {0x000} \
    CONFIG.MC_F1_LPDDR4_MR2 {0x000} \
    CONFIG.MC_F1_LPDDR4_MR3 {0x000} \
    CONFIG.MC_F1_LPDDR4_MR13 {0x0C0} \
    CONFIG.MC_F1_TCCD_L {0} \
    CONFIG.MC_F1_TCCD_L_MIN {0} \
    CONFIG.MC_F1_TFAW {30000} \
    CONFIG.MC_F1_TFAWMIN {30000} \
    CONFIG.MC_F1_TMOD {0} \
    CONFIG.MC_F1_TMOD_MIN {0} \
    CONFIG.MC_F1_TMRD {14000} \
    CONFIG.MC_F1_TMRDMIN {14000} \
    CONFIG.MC_F1_TMRW {10000} \
    CONFIG.MC_F1_TMRWMIN {10000} \
    CONFIG.MC_F1_TRAS {42000} \
    CONFIG.MC_F1_TRASMIN {42000} \
    CONFIG.MC_F1_TRCD {18000} \
    CONFIG.MC_F1_TRCDMIN {18000} \
    CONFIG.MC_F1_TRPAB {21000} \
    CONFIG.MC_F1_TRPABMIN {21000} \
    CONFIG.MC_F1_TRPPB {18000} \
    CONFIG.MC_F1_TRPPBMIN {18000} \
    CONFIG.MC_F1_TRRD {7500} \
    CONFIG.MC_F1_TRRDMIN {7500} \
    CONFIG.MC_F1_TRRD_L {0} \
    CONFIG.MC_F1_TRRD_L_MIN {0} \
    CONFIG.MC_F1_TRRD_S {0} \
    CONFIG.MC_F1_TRRD_S_MIN {0} \
    CONFIG.MC_F1_TWR {18000} \
    CONFIG.MC_F1_TWRMIN {18000} \
    CONFIG.MC_F1_TWTR {10000} \
    CONFIG.MC_F1_TWTRMIN {10000} \
    CONFIG.MC_F1_TWTR_L {0} \
    CONFIG.MC_F1_TWTR_L_MIN {0} \
    CONFIG.MC_F1_TWTR_S {0} \
    CONFIG.MC_F1_TWTR_S_MIN {0} \
    CONFIG.MC_F1_TZQLAT {30000} \
    CONFIG.MC_F1_TZQLATMIN {30000} \
    CONFIG.MC_INIT_MEM_USING_ECC_SCRUB {false} \
    CONFIG.MC_INPUTCLK0_PERIOD {4963} \
    CONFIG.MC_INPUT_FREQUENCY0 {201.501} \
    CONFIG.MC_INTERLEAVE_SIZE {128} \
    CONFIG.MC_IP_TIMEPERIOD0_FOR_OP {1071} \
    CONFIG.MC_LP4_OVERWRITE_IO_PROP {true} \
    CONFIG.MC_LP4_PIN_EFFICIENT {true} \
    CONFIG.MC_LPDDR4_REFRESH_TYPE {PER_BANK} \
    CONFIG.MC_MEMORY_DENSITY {2GB} \
    CONFIG.MC_MEMORY_DEVICE_DENSITY {16Gb} \
    CONFIG.MC_MEMORY_SPEEDGRADE {LPDDR4-4267} \
    CONFIG.MC_MEMORY_TIMEPERIOD0 {509} \
    CONFIG.MC_MEMORY_TIMEPERIOD1 {509} \
    CONFIG.MC_NO_CHANNELS {Dual} \
    CONFIG.MC_ODTLon {8} \
    CONFIG.MC_PER_RD_INTVL {0} \
    CONFIG.MC_PRE_DEF_ADDR_MAP_SEL {ROW_BANK_COLUMN} \
    CONFIG.MC_RANK {1} \
    CONFIG.MC_ROWADDRESSWIDTH {16} \
    CONFIG.MC_TRC {63000} \
    CONFIG.MC_TRCD {18000} \
    CONFIG.MC_TREFI {3904000} \
    CONFIG.MC_TREFIPB {488000} \
    CONFIG.MC_TRFC {0} \
    CONFIG.MC_TRFCAB {280000} \
    CONFIG.MC_TRFCMIN {0} \
    CONFIG.MC_TRFCPB {140000} \
    CONFIG.MC_TRFCPBMIN {140000} \
    CONFIG.MC_TRP {0} \
    CONFIG.MC_TRPAB {21000} \
    CONFIG.MC_TRPMIN {0} \
    CONFIG.MC_TRPPB {18000} \
    CONFIG.MC_TRRD {7500} \
    CONFIG.MC_TRRD_L {0} \
    CONFIG.MC_TRRD_S {0} \
    CONFIG.MC_TRRD_S_MIN {0} \
    CONFIG.MC_TRTP_nCK {16} \
    CONFIG.MC_TWPRE {1.8} \
    CONFIG.MC_TWPST {0.4} \
    CONFIG.MC_TWR {18000} \
    CONFIG.MC_TWTR {10000} \
    CONFIG.MC_TWTR_L {0} \
    CONFIG.MC_TWTR_S {0} \
    CONFIG.MC_TWTR_S_MIN {0} \
    CONFIG.MC_TXP {15000} \
    CONFIG.MC_TXPMIN {15000} \
    CONFIG.MC_TXPR {0} \
    CONFIG.MC_TZQCAL {1000000} \
    CONFIG.MC_TZQCAL_div4 {250000} \
    CONFIG.MC_TZQCS_ITVL {0} \
    CONFIG.MC_TZQLAT {30000} \
    CONFIG.MC_TZQ_START_ITVL {1000000000} \
    CONFIG.MC_USER_DEFINED_ADDRESS_MAP {16RA-3BA-10CA} \
    CONFIG.MC_WRITE_BANDWIDTH {7858.546} \
    CONFIG.MC_XPLL_CLKOUT1_PERIOD {1018} \
    CONFIG.MC_XPLL_CLKOUT1_PHASE {210.41257367387036} \
    CONFIG.NUM_CLKS {1} \
    CONFIG.NUM_MC {1} \
    CONFIG.NUM_MCP {4} \
    CONFIG.NUM_MI {0} \
    CONFIG.NUM_NMI {0} \
    CONFIG.NUM_NSI {0} \
    CONFIG.NUM_SI {1} \
] [get_bd_cells axi_noc_0]

# Configure NoC slave interface
set_property -dict [list \
    CONFIG.CONNECTIONS {MC_0 {read_bw {500} write_bw {500} read_avg_burst {4} write_avg_burst {4}}} \
    CONFIG.NOC_PARAMS {} \
    CONFIG.CATEGORY {ps_cci} \
] [get_bd_intf_pins /axi_noc_0/S00_AXI]

set_property -dict [list \
    CONFIG.ASSOCIATED_BUSIF {S00_AXI} \
] [get_bd_pins /axi_noc_0/aclk0]

puts "NoC configured for LPDDR4"

#-------------------------------------------------------------------------------
# Add AXI CDMA
#-------------------------------------------------------------------------------
puts "Adding AXI CDMA..."

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_cdma:* axi_cdma_0

set_property -dict [list \
    CONFIG.C_INCLUDE_SG {0} \
    CONFIG.C_M_AXI_DATA_WIDTH {64} \
    CONFIG.C_M_AXI_MAX_BURST_LEN {64} \
    CONFIG.C_ADDR_WIDTH {44} \
] [get_bd_cells axi_cdma_0]

puts "CDMA added"

#-------------------------------------------------------------------------------
# Add SmartConnect for PS to PL
#-------------------------------------------------------------------------------
puts "Adding SmartConnect..."

create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:* smartconnect_0

set_property -dict [list \
    CONFIG.NUM_MI {1} \
    CONFIG.NUM_SI {1} \
] [get_bd_cells smartconnect_0]

#-------------------------------------------------------------------------------
# Add SmartConnect for CDMA to NoC
#-------------------------------------------------------------------------------
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:* smartconnect_1

set_property -dict [list \
    CONFIG.NUM_MI {1} \
    CONFIG.NUM_SI {1} \
] [get_bd_cells smartconnect_1]

#-------------------------------------------------------------------------------
# Add Processor System Reset
#-------------------------------------------------------------------------------
puts "Adding Processor System Reset..."

create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:* proc_sys_reset_0

#-------------------------------------------------------------------------------
# Make Connections
#-------------------------------------------------------------------------------
puts "Making connections..."

# Connect CIPS to NoC
connect_bd_intf_net [get_bd_intf_pins cips_0/FPD_CCI_NOC_0] [get_bd_intf_pins axi_noc_0/S00_AXI]

# Clock connections
connect_bd_net [get_bd_pins cips_0/pl0_ref_clk] [get_bd_pins axi_noc_0/aclk0]
connect_bd_net [get_bd_pins cips_0/pl0_ref_clk] [get_bd_pins proc_sys_reset_0/slowest_sync_clk]
connect_bd_net [get_bd_pins cips_0/pl0_ref_clk] [get_bd_pins smartconnect_0/aclk]
connect_bd_net [get_bd_pins cips_0/pl0_ref_clk] [get_bd_pins smartconnect_1/aclk]
connect_bd_net [get_bd_pins cips_0/pl0_ref_clk] [get_bd_pins axi_cdma_0/m_axi_aclk]
connect_bd_net [get_bd_pins cips_0/pl0_ref_clk] [get_bd_pins axi_cdma_0/s_axi_lite_aclk]

# Reset connections
connect_bd_net [get_bd_pins cips_0/pl0_resetn] [get_bd_pins proc_sys_reset_0/ext_reset_in]
connect_bd_net [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_pins smartconnect_0/aresetn]
connect_bd_net [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_pins smartconnect_1/aresetn]
connect_bd_net [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_pins axi_cdma_0/s_axi_lite_aresetn]

# Connect PS M_AXI_FPD to SmartConnect to CDMA control
connect_bd_intf_net [get_bd_intf_pins cips_0/M_AXI_FPD] [get_bd_intf_pins smartconnect_0/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins smartconnect_0/M00_AXI] [get_bd_intf_pins axi_cdma_0/S_AXI_LITE]

# Connect CDMA M_AXI to SmartConnect to NoC
connect_bd_intf_net [get_bd_intf_pins axi_cdma_0/M_AXI] [get_bd_intf_pins smartconnect_1/S00_AXI]

# Need to add another NoC slave port for CDMA
set_property -dict [list CONFIG.NUM_SI {2}] [get_bd_cells axi_noc_0]

set_property -dict [list \
    CONFIG.CONNECTIONS {MC_0 {read_bw {500} write_bw {500} read_avg_burst {4} write_avg_burst {4}}} \
    CONFIG.NOC_PARAMS {} \
    CONFIG.CATEGORY {pl} \
] [get_bd_intf_pins /axi_noc_0/S01_AXI]

connect_bd_intf_net [get_bd_intf_pins smartconnect_1/M00_AXI] [get_bd_intf_pins axi_noc_0/S01_AXI]
connect_bd_net [get_bd_pins cips_0/pl0_ref_clk] [get_bd_pins axi_noc_0/aclk1]

set_property -dict [list \
    CONFIG.ASSOCIATED_BUSIF {S01_AXI} \
] [get_bd_pins /axi_noc_0/aclk1]

# Connect interrupt
connect_bd_net [get_bd_pins axi_cdma_0/cdma_introut] [get_bd_pins cips_0/pl_ps_irq0]

puts "Connections made"

#-------------------------------------------------------------------------------
# Create LPDDR4 external interface
#-------------------------------------------------------------------------------
puts "Creating LPDDR4 external interface..."

# Make LPDDR4 ports external
make_bd_intf_pins_external [get_bd_intf_pins axi_noc_0/CH0_LPDDR4_0]
make_bd_intf_pins_external [get_bd_intf_pins axi_noc_0/CH0_LPDDR4_1]
make_bd_intf_pins_external [get_bd_intf_pins axi_noc_0/CH1_LPDDR4_0]
make_bd_intf_pins_external [get_bd_intf_pins axi_noc_0/CH1_LPDDR4_1]
make_bd_pins_external [get_bd_pins axi_noc_0/sys_clk0]
make_bd_pins_external [get_bd_pins axi_noc_0/sys_clk1]

#-------------------------------------------------------------------------------
# Assign Addresses
#-------------------------------------------------------------------------------
puts "Assigning addresses..."

regenerate_bd_layout
assign_bd_address

# Validate
puts "Validating design..."
validate_bd_design

save_bd_design

#-------------------------------------------------------------------------------
# Generate Output Products
#-------------------------------------------------------------------------------
puts "Generating output products..."

set bd_file [get_files design_1.bd]
generate_target all $bd_file

#-------------------------------------------------------------------------------
# Create Wrapper
#-------------------------------------------------------------------------------
puts "Creating HDL wrapper..."

make_wrapper -files $bd_file -top
set wrapper_file "$project_dir/$project_name/$project_name.gen/sources_1/bd/design_1/hdl/design_1_wrapper.v"
add_files -norecurse $wrapper_file
update_compile_order -fileset sources_1

#-------------------------------------------------------------------------------
# Synthesis
#-------------------------------------------------------------------------------
puts ""
puts "============================================================"
puts "  Starting Synthesis (10-20 minutes)..."
puts "============================================================"

launch_runs synth_1 -jobs 8
wait_on_run synth_1

set synth_status [get_property STATUS [get_runs synth_1]]
puts "Synthesis status: $synth_status"

if {[string match "*ERROR*" $synth_status] || [string match "*FAILED*" $synth_status]} {
    puts "ERROR: Synthesis failed!"
    exit 1
}

#-------------------------------------------------------------------------------
# Implementation
#-------------------------------------------------------------------------------
puts ""
puts "============================================================"
puts "  Starting Implementation (20-40 minutes)..."
puts "============================================================"

launch_runs impl_1 -jobs 8
wait_on_run impl_1

set impl_status [get_property STATUS [get_runs impl_1]]
puts "Implementation status: $impl_status"

#-------------------------------------------------------------------------------
# Generate Device Image
#-------------------------------------------------------------------------------
puts ""
puts "============================================================"
puts "  Generating Device Image..."
puts "============================================================"

launch_runs impl_1 -to_step write_device_image -jobs 8
wait_on_run impl_1

#-------------------------------------------------------------------------------
# Export XSA
#-------------------------------------------------------------------------------
puts ""
puts "============================================================"
puts "  Exporting XSA..."
puts "============================================================"

set xsa_path "$project_dir/design_noboard.xsa"
write_hw_platform -fixed -include_bit -force -file $xsa_path

puts ""
puts "============================================================"
puts "  BUILD COMPLETE!"
puts "============================================================"
puts ""
puts "XSA file: $xsa_path"
puts ""
puts "Memory Map:"
report_bd_addr_seg
puts ""
