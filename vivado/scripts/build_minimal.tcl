#===============================================================================
# Versal DMA Benchmark - Minimal Build (DDR + Basic DMA)
# VPK120 Development Board (VP1202)
#
# Usage: vivado -mode batch -source build_minimal.tcl
#
# Simpler design focusing on getting DDR working
#===============================================================================

set script_dir [file dirname [file normalize [info script]]]
set project_dir [file normalize "$script_dir/.."]
set project_name "versal_dma_minimal"

puts "============================================================"
puts "  Versal DMA - Minimal Build (DDR Focus)"
puts "  VPK120 (VP1202)"
puts "============================================================"

#-------------------------------------------------------------------------------
# Create Project
#-------------------------------------------------------------------------------
catch {close_project}

if {[file exists "$project_dir/$project_name"]} {
    file delete -force "$project_dir/$project_name"
}

create_project $project_name "$project_dir/$project_name" -part xcvp1202-vsva2785-2MP-e-S
set_property board_part xilinx.com:vpk120:part0:1.4 [current_project]

puts "Project created"

#-------------------------------------------------------------------------------
# Create Block Design with Board Preset
#-------------------------------------------------------------------------------
create_bd_design "design_1"

# Add CIPS with full board automation (this sets up everything including LPDDR4)
create_bd_cell -type ip -vlnv xilinx.com:ip:versal_cips:* versal_cips_0

# Apply board automation - this is the key step that configures LPDDR4
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

puts "CIPS and NoC added with board preset"

#-------------------------------------------------------------------------------
# Add AXI CDMA for memory-to-memory transfers
#-------------------------------------------------------------------------------
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_cdma:* axi_cdma_0

set_property -dict [list \
    CONFIG.C_INCLUDE_SG {0} \
    CONFIG.C_M_AXI_DATA_WIDTH {128} \
    CONFIG.C_M_AXI_MAX_BURST_LEN {256} \
    CONFIG.C_ADDR_WIDTH {64} \
] [get_bd_cells axi_cdma_0]

puts "AXI CDMA added"

#-------------------------------------------------------------------------------
# Add Processor System Reset
#-------------------------------------------------------------------------------
create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:* proc_sys_reset_0

#-------------------------------------------------------------------------------
# Run Connection Automation
#-------------------------------------------------------------------------------
puts "Running connection automation..."

# This will automatically connect CDMA to NoC and CIPS
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { \
    Clk_master {Auto} \
    Clk_slave {Auto} \
    Clk_xbar {Auto} \
    Master {/versal_cips_0/M_AXI_FPD} \
    Slave {/axi_cdma_0/S_AXI_LITE} \
    ddr_seg {Auto} \
    intc_ip {New AXI Interconnect} \
    master_apm {0} \
} [get_bd_intf_pins axi_cdma_0/S_AXI_LITE]

apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { \
    Clk_master {Auto} \
    Clk_slave {Auto} \
    Clk_xbar {Auto} \
    Master {/axi_cdma_0/M_AXI} \
    Slave {/axi_noc_0/S00_AXI} \
    ddr_seg {Auto} \
    intc_ip {/axi_noc_0} \
    master_apm {0} \
} [get_bd_intf_pins axi_cdma_0/M_AXI]

# Connect reset
apply_bd_automation -rule xilinx.com:bd_rule:clkrst -config { \
    Clk {/versal_cips_0/pl0_ref_clk (99 MHz)} \
    Freq {100} \
    Ref_Clk0 {} \
    Ref_Clk1 {} \
    Ref_Clk2 {} \
} [get_bd_pins proc_sys_reset_0/slowest_sync_clk]

puts "Connections made"

#-------------------------------------------------------------------------------
# Connect Interrupt
#-------------------------------------------------------------------------------
connect_bd_net [get_bd_pins axi_cdma_0/cdma_introut] [get_bd_pins versal_cips_0/pl_ps_irq0]

#-------------------------------------------------------------------------------
# Assign Addresses and Validate
#-------------------------------------------------------------------------------
regenerate_bd_layout
assign_bd_address
validate_bd_design

puts "Design validated"

#-------------------------------------------------------------------------------
# Generate Output Products
#-------------------------------------------------------------------------------
save_bd_design
generate_target all [get_files design_1.bd]

#-------------------------------------------------------------------------------
# Create HDL Wrapper
#-------------------------------------------------------------------------------
make_wrapper -files [get_files design_1.bd] -top
add_files -norecurse "$project_dir/$project_name/$project_name.gen/sources_1/bd/design_1/hdl/design_1_wrapper.v"

#-------------------------------------------------------------------------------
# Run Synthesis
#-------------------------------------------------------------------------------
puts "Starting synthesis... (10-20 min)"
launch_runs synth_1 -jobs 8
wait_on_run synth_1

if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    error "Synthesis failed!"
}
puts "Synthesis complete"

#-------------------------------------------------------------------------------
# Run Implementation
#-------------------------------------------------------------------------------
puts "Starting implementation... (20-40 min)"
launch_runs impl_1 -jobs 8
wait_on_run impl_1

if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    error "Implementation failed!"
}
puts "Implementation complete"

#-------------------------------------------------------------------------------
# Generate Device Image (PDI)
#-------------------------------------------------------------------------------
puts "Generating device image..."
launch_runs impl_1 -to_step write_device_image -jobs 8
wait_on_run impl_1
puts "Device image generated"

#-------------------------------------------------------------------------------
# Export XSA
#-------------------------------------------------------------------------------
set xsa_file "$project_dir/design_minimal.xsa"
write_hw_platform -fixed -include_bit -force -file $xsa_file

puts ""
puts "============================================================"
puts "  BUILD COMPLETE!"
puts "============================================================"
puts "XSA: $xsa_file"
puts ""
