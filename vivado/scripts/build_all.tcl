#===============================================================================
# Versal DMA Benchmark - Master Build Script
# VPK120 Development Board (VP1202)
#
# Usage: vivado -mode batch -source build_all.tcl
#        or in Vivado Tcl Console: source build_all.tcl
#
# This script creates the complete hardware design and generates XSA
#===============================================================================

# Get script directory
set script_dir [file dirname [file normalize [info script]]]
set project_dir [file normalize "$script_dir/.."]
set project_name "versal_dma_benchmark"

puts "============================================================"
puts "  Versal DMA Benchmark - Automated Build"
puts "  VPK120 (VP1202) Development Board"
puts "============================================================"
puts "Script directory: $script_dir"
puts "Project directory: $project_dir"
puts ""

#-------------------------------------------------------------------------------
# Step 1: Create Project
#-------------------------------------------------------------------------------
puts "Step 1: Creating Vivado project..."

# Close any open project
catch {close_project}

# Remove existing project if exists
if {[file exists "$project_dir/$project_name"]} {
    puts "  Removing existing project..."
    file delete -force "$project_dir/$project_name"
}

# Create project for VPK120 board
create_project $project_name "$project_dir/$project_name" -part xcvp1202-vsva2785-2MP-e-S

# Set board part for VPK120
set_property board_part xilinx.com:vpk120:part0:1.4 [current_project]

# Set project properties
set_property target_language Verilog [current_project]
set_property simulator_language Mixed [current_project]

puts "  Project created successfully"
puts ""

#-------------------------------------------------------------------------------
# Step 2: Create Block Design
#-------------------------------------------------------------------------------
puts "Step 2: Creating block design..."

create_bd_design "dma_system"

# Update IP catalog
update_ip_catalog

puts "  Block design created"
puts ""

#-------------------------------------------------------------------------------
# Step 3: Add and Configure CIPS (Versal PS)
#-------------------------------------------------------------------------------
puts "Step 3: Adding CIPS (Control, Interfaces, Processing System)..."

# Create CIPS instance
create_bd_cell -type ip -vlnv xilinx.com:ip:versal_cips:* versal_cips_0

# Apply board automation for basic setup
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
} [get_bd_cells versal_cips_0]

puts "  CIPS added with board preset"

# Configure CIPS for our needs
set_property -dict [list \
    CONFIG.PS_PMC_CONFIG { \
        CLOCK_MODE {Custom} \
        DDR_MEMORY_MODE {Custom} \
        DEBUG_MODE {JTAG} \
        DESIGN_MODE {1} \
        DEVICE_INTEGRITY_MODE {Sysmon temperature voltage and external IO monitoring} \
        PMC_CRP_PL0_REF_CTRL_FREQMHZ {100} \
        PMC_GPIO0_MIO_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 0 .. 25}}} \
        PMC_GPIO1_MIO_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 26 .. 51}}} \
        PMC_MIO37 {{AUX_IO 0} {DIRECTION out} {DRIVE_STRENGTH 8mA} {OUTPUT_DATA high} {PULL pullup} {SCHMITT 0} {SLEW slow} {USAGE GPIO}} \
        PMC_OSPI_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 0 .. 11}} {MODE Single}} \
        PMC_REF_CLK_FREQMHZ {33.3333} \
        PMC_SD1 {{CD_ENABLE 1} {CD_IO {PMC_MIO 28}} {POW_ENABLE 1} {POW_IO {PMC_MIO 51}} {RESET_ENABLE 0} {RESET_IO {PMC_MIO 12}} {WP_ENABLE 0} {WP_IO {PMC_MIO 1}}} \
        PMC_SD1_PERIPHERAL {{CLK_100_SDR_OTAP_DLY 0x3} {CLK_200_SDR_OTAP_DLY 0x2} {CLK_50_DDR_ITAP_DLY 0x36} {CLK_50_DDR_OTAP_DLY 0x3} {CLK_50_SDR_ITAP_DLY 0x2C} {CLK_50_SDR_OTAP_DLY 0x4} {ENABLE 1} {IO {PMC_MIO 26 .. 36}}} \
        PMC_SD1_SLOT_TYPE {SD 3.0} \
        PMC_USE_PMC_NOC_AXI0 {1} \
        PS_BOARD_INTERFACE {ps_pmc_fixed_io} \
        PS_ENET0_MDIO {{ENABLE 1} {IO {PS_MIO 24 .. 25}}} \
        PS_ENET0_PERIPHERAL {{ENABLE 1} {IO {PS_MIO 0 .. 11}}} \
        PS_GEN_IPI0_ENABLE {1} \
        PS_GEN_IPI1_ENABLE {1} \
        PS_GEN_IPI2_ENABLE {1} \
        PS_GEN_IPI3_ENABLE {1} \
        PS_GEN_IPI4_ENABLE {1} \
        PS_GEN_IPI5_ENABLE {1} \
        PS_GEN_IPI6_ENABLE {1} \
        PS_HSDP_EGRESS_TRAFFIC {JTAG} \
        PS_HSDP_INGRESS_TRAFFIC {JTAG} \
        PS_HSDP_MODE {NONE} \
        PS_I2C0_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 46 .. 47}}} \
        PS_I2C1_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 44 .. 45}}} \
        PS_IRQ_USAGE {{CH0 1} {CH1 1} {CH10 1} {CH11 1} {CH12 1} {CH13 1} {CH14 1} {CH15 1} {CH2 1} {CH3 1} {CH4 1} {CH5 1} {CH6 1} {CH7 1} {CH8 1} {CH9 1}} \
        PS_MIO7 {{AUX_IO 0} {DIRECTION in} {DRIVE_STRENGTH 8mA} {OUTPUT_DATA default} {PULL disable} {SCHMITT 0} {SLEW slow} {USAGE Reserved}} \
        PS_MIO9 {{AUX_IO 0} {DIRECTION in} {DRIVE_STRENGTH 8mA} {OUTPUT_DATA default} {PULL disable} {SCHMITT 0} {SLEW slow} {USAGE Reserved}} \
        PS_NUM_FABRIC_RESETS {1} \
        PS_PCIE_EP_RESET1_IO {PMC_MIO 38} \
        PS_PCIE_EP_RESET2_IO {PMC_MIO 39} \
        PS_PCIE_RESET {ENABLE 1} \
        PS_PL_CONNECTIVITY_MODE {Custom} \
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
        SMON_INTERFACE_TO_USE {I2C} \
        SMON_PMBUS_ADDRESS {0x18} \
        SMON_TEMP_AVERAGING_SAMPLES {0} \
    } \
    CONFIG.PS_PMC_CONFIG_APPLIED {1} \
] [get_bd_cells versal_cips_0]

puts "  CIPS configured"
puts ""

#-------------------------------------------------------------------------------
# Step 4: Add and Configure NoC for Memory Access
#-------------------------------------------------------------------------------
puts "Step 4: Configuring AXI NoC for memory access..."

# The board automation should have created axi_noc_0
# If not, create it
if {[llength [get_bd_cells -quiet axi_noc_0]] == 0} {
    create_bd_cell -type ip -vlnv xilinx.com:ip:axi_noc:* axi_noc_0
}

# Configure NoC for LPDDR4 memory controller
set_property -dict [list \
    CONFIG.CH0_LPDDR4_0_BOARD_INTERFACE {ch0_lpddr4_trip1} \
    CONFIG.CH0_LPDDR4_1_BOARD_INTERFACE {ch0_lpddr4_trip2} \
    CONFIG.CH1_LPDDR4_0_BOARD_INTERFACE {ch1_lpddr4_trip1} \
    CONFIG.CH1_LPDDR4_1_BOARD_INTERFACE {ch1_lpddr4_trip2} \
    CONFIG.MC_CHANNEL_INTERLEAVING {true} \
    CONFIG.MC_CH_INTERLEAVING_SIZE {128_Bytes} \
    CONFIG.MC_LPDDR4_REFRESH_TYPE {PER_BANK} \
    CONFIG.NUM_CLKS {9} \
    CONFIG.NUM_MC {1} \
    CONFIG.NUM_MCP {4} \
    CONFIG.NUM_MI {1} \
    CONFIG.NUM_NMI {0} \
    CONFIG.NUM_NSI {0} \
    CONFIG.NUM_SI {8} \
    CONFIG.sys_clk0_BOARD_INTERFACE {lpddr4_sma_clk1} \
    CONFIG.sys_clk1_BOARD_INTERFACE {lpddr4_sma_clk2} \
] [get_bd_cells axi_noc_0]

puts "  NoC configured for LPDDR4"
puts ""

#-------------------------------------------------------------------------------
# Step 5: Add AXI CDMA (Central DMA for memory-to-memory)
#-------------------------------------------------------------------------------
puts "Step 5: Adding AXI CDMA..."

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_cdma:* axi_cdma_0

set_property -dict [list \
    CONFIG.C_INCLUDE_SG {1} \
    CONFIG.C_M_AXI_DATA_WIDTH {128} \
    CONFIG.C_M_AXI_MAX_BURST_LEN {256} \
    CONFIG.C_ADDR_WIDTH {64} \
] [get_bd_cells axi_cdma_0]

puts "  AXI CDMA added"
puts ""

#-------------------------------------------------------------------------------
# Step 6: Add AXI DMA (for stream interfaces)
#-------------------------------------------------------------------------------
puts "Step 6: Adding AXI DMA..."

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dma:* axi_dma_0

set_property -dict [list \
    CONFIG.c_include_sg {1} \
    CONFIG.c_m_axi_mm2s_data_width {128} \
    CONFIG.c_m_axi_s2mm_data_width {128} \
    CONFIG.c_mm2s_burst_size {256} \
    CONFIG.c_s2mm_burst_size {256} \
    CONFIG.c_sg_include_stscntrl_strm {0} \
    CONFIG.c_addr_width {64} \
] [get_bd_cells axi_dma_0]

puts "  AXI DMA added"
puts ""

#-------------------------------------------------------------------------------
# Step 7: Add AXI Stream FIFO for DMA loopback
#-------------------------------------------------------------------------------
puts "Step 7: Adding AXI Stream FIFO for loopback..."

create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:* axis_data_fifo_0

set_property -dict [list \
    CONFIG.FIFO_DEPTH {1024} \
    CONFIG.FIFO_MODE {2} \
    CONFIG.HAS_TKEEP {1} \
    CONFIG.HAS_TLAST {1} \
    CONFIG.TDATA_NUM_BYTES {16} \
] [get_bd_cells axis_data_fifo_0]

puts "  AXI Stream FIFO added"
puts ""

#-------------------------------------------------------------------------------
# Step 8: Add SmartConnect for AXI interconnect
#-------------------------------------------------------------------------------
puts "Step 8: Adding SmartConnect..."

create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:* smartconnect_0

set_property -dict [list \
    CONFIG.NUM_MI {1} \
    CONFIG.NUM_SI {3} \
] [get_bd_cells smartconnect_0]

puts "  SmartConnect added"
puts ""

#-------------------------------------------------------------------------------
# Step 9: Add AXI BRAM Controller and BRAM
#-------------------------------------------------------------------------------
puts "Step 9: Adding BRAM..."

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:* axi_bram_ctrl_0

set_property -dict [list \
    CONFIG.DATA_WIDTH {128} \
    CONFIG.SINGLE_PORT_BRAM {1} \
] [get_bd_cells axi_bram_ctrl_0]

create_bd_cell -type ip -vlnv xilinx.com:ip:emb_mem_gen:* emb_mem_gen_0

set_property -dict [list \
    CONFIG.MEMORY_TYPE {True_Dual_Port_RAM} \
    CONFIG.MEMORY_SIZE {131072} \
] [get_bd_cells emb_mem_gen_0]

puts "  BRAM added (128KB)"
puts ""

#-------------------------------------------------------------------------------
# Step 10: Add Processor System Reset
#-------------------------------------------------------------------------------
puts "Step 10: Adding Processor System Reset..."

create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:* proc_sys_reset_0

puts "  Processor System Reset added"
puts ""

#-------------------------------------------------------------------------------
# Step 11: Add Clocking Wizard for PL clocks
#-------------------------------------------------------------------------------
puts "Step 11: Adding Clocking Wizard..."

create_bd_cell -type ip -vlnv xilinx.com:ip:clk_wizard:* clk_wizard_0

set_property -dict [list \
    CONFIG.CLKOUT_DRIVES {BUFG,BUFG,BUFG,BUFG,BUFG,BUFG,BUFG} \
    CONFIG.CLKOUT_DYN_PS {None,None,None,None,None,None,None} \
    CONFIG.CLKOUT_GROUPING {Auto,Auto,Auto,Auto,Auto,Auto,Auto} \
    CONFIG.CLKOUT_MATCHED_ROUTING {false,false,false,false,false,false,false} \
    CONFIG.CLKOUT_PORT {clk_out1,clk_out2,clk_out3,clk_out4,clk_out5,clk_out6,clk_out7} \
    CONFIG.CLKOUT_REQUESTED_DUTY_CYCLE {50.000,50.000,50.000,50.000,50.000,50.000,50.000} \
    CONFIG.CLKOUT_REQUESTED_OUT_FREQUENCY {100.000,250.000,100,100,100,100,100} \
    CONFIG.CLKOUT_REQUESTED_PHASE {0.000,0.000,0.000,0.000,0.000,0.000,0.000} \
    CONFIG.CLKOUT_USED {true,true,false,false,false,false,false} \
    CONFIG.PRIM_SOURCE {Global_buffer} \
    CONFIG.USE_LOCKED {true} \
    CONFIG.USE_RESET {true} \
] [get_bd_cells clk_wizard_0]

puts "  Clocking Wizard added (100MHz, 250MHz)"
puts ""

#-------------------------------------------------------------------------------
# Step 12: Make Connections
#-------------------------------------------------------------------------------
puts "Step 12: Making connections..."

# Connect clocks
connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins clk_wizard_0/clk_in1]
connect_bd_net [get_bd_pins versal_cips_0/pl0_resetn] [get_bd_pins clk_wizard_0/resetn]

# Get clock outputs
set clk_100 [get_bd_pins clk_wizard_0/clk_out1]
set clk_250 [get_bd_pins clk_wizard_0/clk_out2]

# Connect proc_sys_reset
connect_bd_net $clk_100 [get_bd_pins proc_sys_reset_0/slowest_sync_clk]
connect_bd_net [get_bd_pins clk_wizard_0/locked] [get_bd_pins proc_sys_reset_0/dcm_locked]
connect_bd_net [get_bd_pins versal_cips_0/pl0_resetn] [get_bd_pins proc_sys_reset_0/ext_reset_in]

# Connect DMA clocks and resets
connect_bd_net $clk_100 [get_bd_pins axi_cdma_0/m_axi_aclk]
connect_bd_net $clk_100 [get_bd_pins axi_cdma_0/s_axi_lite_aclk]
connect_bd_net [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_pins axi_cdma_0/s_axi_lite_aresetn]

connect_bd_net $clk_100 [get_bd_pins axi_dma_0/m_axi_mm2s_aclk]
connect_bd_net $clk_100 [get_bd_pins axi_dma_0/m_axi_s2mm_aclk]
connect_bd_net $clk_100 [get_bd_pins axi_dma_0/m_axi_sg_aclk]
connect_bd_net $clk_100 [get_bd_pins axi_dma_0/s_axi_lite_aclk]
connect_bd_net [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_pins axi_dma_0/axi_resetn]

# Connect FIFO
connect_bd_net $clk_100 [get_bd_pins axis_data_fifo_0/s_axis_aclk]
connect_bd_net [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_pins axis_data_fifo_0/s_axis_aresetn]

# Connect DMA stream to FIFO (loopback)
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXIS_MM2S] [get_bd_intf_pins axis_data_fifo_0/S_AXIS]
connect_bd_intf_net [get_bd_intf_pins axis_data_fifo_0/M_AXIS] [get_bd_intf_pins axi_dma_0/S_AXIS_S2MM]

# Connect SmartConnect
connect_bd_net $clk_100 [get_bd_pins smartconnect_0/aclk]
connect_bd_net [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_pins smartconnect_0/aresetn]

# Connect DMA memory interfaces to SmartConnect
connect_bd_intf_net [get_bd_intf_pins axi_cdma_0/M_AXI] [get_bd_intf_pins smartconnect_0/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXI_MM2S] [get_bd_intf_pins smartconnect_0/S01_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXI_S2MM] [get_bd_intf_pins smartconnect_0/S02_AXI]

# Connect BRAM
connect_bd_net $clk_100 [get_bd_pins axi_bram_ctrl_0/s_axi_aclk]
connect_bd_net [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_pins axi_bram_ctrl_0/s_axi_aresetn]
connect_bd_intf_net [get_bd_intf_pins axi_bram_ctrl_0/BRAM_PORTA] [get_bd_intf_pins emb_mem_gen_0/BRAM_PORTA]

puts "  Basic connections made"
puts ""

#-------------------------------------------------------------------------------
# Step 13: Run Connection Automation
#-------------------------------------------------------------------------------
puts "Step 13: Running connection automation..."

# Connect SmartConnect to NoC
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { \
    Clk_master {/clk_wizard_0/clk_out1 (100 MHz)} \
    Clk_slave {Auto} \
    Clk_xbar {Auto} \
    Master {/smartconnect_0/M00_AXI} \
    Slave {/axi_noc_0/S00_AXI} \
    ddr_seg {Auto} \
    intc_ip {/axi_noc_0} \
    master_apm {0} \
} [get_bd_intf_pins axi_noc_0/S00_AXI]

# Connect CIPS to DMA control interfaces
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { \
    Clk_master {Auto} \
    Clk_slave {/clk_wizard_0/clk_out1 (100 MHz)} \
    Clk_xbar {Auto} \
    Master {/versal_cips_0/M_AXI_FPD} \
    Slave {/axi_cdma_0/S_AXI_LITE} \
    ddr_seg {Auto} \
    intc_ip {New AXI Interconnect} \
    master_apm {0} \
} [get_bd_intf_pins axi_cdma_0/S_AXI_LITE]

apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { \
    Clk_master {Auto} \
    Clk_slave {/clk_wizard_0/clk_out1 (100 MHz)} \
    Clk_xbar {Auto} \
    Master {/versal_cips_0/M_AXI_FPD} \
    Slave {/axi_dma_0/S_AXI_LITE} \
    ddr_seg {Auto} \
    intc_ip {Auto} \
    master_apm {0} \
} [get_bd_intf_pins axi_dma_0/S_AXI_LITE]

apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { \
    Clk_master {Auto} \
    Clk_slave {/clk_wizard_0/clk_out1 (100 MHz)} \
    Clk_xbar {Auto} \
    Master {/versal_cips_0/M_AXI_FPD} \
    Slave {/axi_bram_ctrl_0/S_AXI} \
    ddr_seg {Auto} \
    intc_ip {Auto} \
    master_apm {0} \
} [get_bd_intf_pins axi_bram_ctrl_0/S_AXI]

# Connect DMA SG interface
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { \
    Clk_master {/clk_wizard_0/clk_out1 (100 MHz)} \
    Clk_slave {Auto} \
    Clk_xbar {Auto} \
    Master {/axi_dma_0/M_AXI_SG} \
    Slave {/axi_noc_0/S00_AXI} \
    ddr_seg {Auto} \
    intc_ip {/axi_noc_0} \
    master_apm {0} \
} [get_bd_intf_pins axi_dma_0/M_AXI_SG]

# Connect CDMA SG interface
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { \
    Clk_master {/clk_wizard_0/clk_out1 (100 MHz)} \
    Clk_slave {Auto} \
    Clk_xbar {Auto} \
    Master {/axi_cdma_0/M_AXI_SG} \
    Slave {/axi_noc_0/S00_AXI} \
    ddr_seg {Auto} \
    intc_ip {/axi_noc_0} \
    master_apm {0} \
} [get_bd_intf_pins axi_cdma_0/M_AXI_SG]

puts "  Connection automation complete"
puts ""

#-------------------------------------------------------------------------------
# Step 14: Connect Interrupts
#-------------------------------------------------------------------------------
puts "Step 14: Connecting interrupts..."

# Create interrupt concat
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:* xlconcat_0

set_property -dict [list \
    CONFIG.NUM_PORTS {4} \
] [get_bd_cells xlconcat_0]

# Connect DMA interrupts
connect_bd_net [get_bd_pins axi_cdma_0/cdma_introut] [get_bd_pins xlconcat_0/In0]
connect_bd_net [get_bd_pins axi_dma_0/mm2s_introut] [get_bd_pins xlconcat_0/In1]
connect_bd_net [get_bd_pins axi_dma_0/s2mm_introut] [get_bd_pins xlconcat_0/In2]

# Connect to CIPS
connect_bd_net [get_bd_pins xlconcat_0/dout] [get_bd_pins versal_cips_0/pl_ps_irq0]

puts "  Interrupts connected"
puts ""

#-------------------------------------------------------------------------------
# Step 15: Assign Addresses
#-------------------------------------------------------------------------------
puts "Step 15: Assigning addresses..."

# Regenerate layout
regenerate_bd_layout

# Assign addresses automatically
assign_bd_address

# Validate design
puts "Step 16: Validating design..."
validate_bd_design

puts "  Design validated"
puts ""

#-------------------------------------------------------------------------------
# Step 17: Save and Generate Output Products
#-------------------------------------------------------------------------------
puts "Step 17: Generating output products..."

save_bd_design
generate_target all [get_files "$project_dir/$project_name/$project_name.srcs/sources_1/bd/dma_system/dma_system.bd"]

puts "  Output products generated"
puts ""

#-------------------------------------------------------------------------------
# Step 18: Create HDL Wrapper
#-------------------------------------------------------------------------------
puts "Step 18: Creating HDL wrapper..."

make_wrapper -files [get_files "$project_dir/$project_name/$project_name.srcs/sources_1/bd/dma_system/dma_system.bd"] -top
add_files -norecurse "$project_dir/$project_name/$project_name.gen/sources_1/bd/dma_system/hdl/dma_system_wrapper.v"
update_compile_order -fileset sources_1

puts "  HDL wrapper created"
puts ""

#-------------------------------------------------------------------------------
# Step 19: Run Synthesis
#-------------------------------------------------------------------------------
puts "Step 19: Running synthesis..."
puts "  This may take 10-20 minutes..."

launch_runs synth_1 -jobs 8
wait_on_run synth_1

if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    puts "ERROR: Synthesis failed!"
    exit 1
}

puts "  Synthesis complete"
puts ""

#-------------------------------------------------------------------------------
# Step 20: Run Implementation
#-------------------------------------------------------------------------------
puts "Step 20: Running implementation..."
puts "  This may take 20-40 minutes..."

launch_runs impl_1 -jobs 8
wait_on_run impl_1

if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    puts "ERROR: Implementation failed!"
    exit 1
}

puts "  Implementation complete"
puts ""

#-------------------------------------------------------------------------------
# Step 21: Generate Bitstream
#-------------------------------------------------------------------------------
puts "Step 21: Generating bitstream..."
puts "  This may take 5-10 minutes..."

launch_runs impl_1 -to_step write_device_image -jobs 8
wait_on_run impl_1

puts "  Bitstream generated"
puts ""

#-------------------------------------------------------------------------------
# Step 22: Export Hardware (XSA)
#-------------------------------------------------------------------------------
puts "Step 22: Exporting hardware (XSA)..."

set xsa_file "$project_dir/dma_system.xsa"
write_hw_platform -fixed -include_bit -force -file $xsa_file

puts "  XSA exported to: $xsa_file"
puts ""

#-------------------------------------------------------------------------------
# Done
#-------------------------------------------------------------------------------
puts "============================================================"
puts "  BUILD COMPLETE!"
puts "============================================================"
puts ""
puts "Output files:"
puts "  XSA: $xsa_file"
puts "  Project: $project_dir/$project_name"
puts ""
puts "Next steps:"
puts "  1. Open Vitis: vitis -workspace vitis_workspace"
puts "  2. Create platform from XSA"
puts "  3. Build the dma_benchmark application"
puts ""
puts "============================================================"
