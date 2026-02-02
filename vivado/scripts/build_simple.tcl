#===============================================================================
# Versal DMA - Simple Build with Board Preset
# VPK120 Development Board
#
# Usage: vivado -mode batch -source build_simple.tcl
#
# This script uses maximum automation from board presets
#===============================================================================

set script_dir [file dirname [file normalize [info script]]]
set project_dir [file normalize "$script_dir/.."]
set project_name "versal_simple"

puts "============================================================"
puts "  Versal Simple Build - VPK120"
puts "============================================================"

# Cleanup
catch {close_project}
if {[file exists "$project_dir/$project_name"]} {
    file delete -force "$project_dir/$project_name"
}

# Create project with board
create_project $project_name "$project_dir/$project_name" -part xcvp1202-vsva2785-2MP-e-S
set_property board_part xilinx.com:vpk120:part0:1.4 [current_project]

puts "Project created for VPK120"

#-------------------------------------------------------------------------------
# Create Block Design using Example Design approach
#-------------------------------------------------------------------------------
create_bd_design "design_1"

# Add CIPS (PS) - this is required
create_bd_cell -type ip -vlnv xilinx.com:ip:versal_cips:* cips_0

# Use board preset automation - this configures LPDDR4 automatically
puts "Applying board automation for CIPS and NoC..."

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

puts "Board automation applied - LPDDR4 configured"

#-------------------------------------------------------------------------------
# Add Simple AXI CDMA
#-------------------------------------------------------------------------------
puts "Adding AXI CDMA..."

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_cdma:* axi_cdma_0

# Simple mode (no scatter-gather) for easier debugging
set_property -dict [list \
    CONFIG.C_INCLUDE_SG {0} \
    CONFIG.C_M_AXI_DATA_WIDTH {64} \
    CONFIG.C_M_AXI_MAX_BURST_LEN {16} \
    CONFIG.C_ADDR_WIDTH {44} \
] [get_bd_cells axi_cdma_0]

#-------------------------------------------------------------------------------
# Connect CDMA with automation
#-------------------------------------------------------------------------------
puts "Connecting CDMA..."

# Connect CDMA control port to PS
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { \
    Clk_master {Auto} \
    Clk_slave {Auto} \
    Clk_xbar {Auto} \
    Master {/cips_0/M_AXI_FPD} \
    Slave {/axi_cdma_0/S_AXI_LITE} \
    ddr_seg {Auto} \
    intc_ip {New AXI Interconnect} \
    master_apm {0} \
} [get_bd_intf_pins axi_cdma_0/S_AXI_LITE]

# Connect CDMA memory port to NoC (for DDR access)
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

puts "CDMA connected"

#-------------------------------------------------------------------------------
# Connect Interrupt
#-------------------------------------------------------------------------------
# Create concat for interrupts
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:* xlconcat_0
set_property CONFIG.NUM_PORTS {1} [get_bd_cells xlconcat_0]

connect_bd_net [get_bd_pins axi_cdma_0/cdma_introut] [get_bd_pins xlconcat_0/In0]
connect_bd_net [get_bd_pins xlconcat_0/dout] [get_bd_pins cips_0/pl_ps_irq0]

puts "Interrupts connected"

#-------------------------------------------------------------------------------
# Finalize Design
#-------------------------------------------------------------------------------
puts "Finalizing design..."

regenerate_bd_layout
assign_bd_address

# Save before validation
save_bd_design

# Validate
puts "Validating design..."
set validation_result [validate_bd_design -force]

if {$validation_result != ""} {
    puts "Validation warnings/errors:"
    puts $validation_result
}

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

reset_run synth_1
launch_runs synth_1 -jobs 8
wait_on_run synth_1

set synth_status [get_property STATUS [get_runs synth_1]]
puts "Synthesis status: $synth_status"

if {$synth_status != "synth_design Complete!"} {
    puts "ERROR: Synthesis did not complete successfully"
    puts "Check the log for details"
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

set xsa_path "$project_dir/design_simple.xsa"
write_hw_platform -fixed -include_bit -force -file $xsa_path

puts ""
puts "============================================================"
puts "  BUILD COMPLETE!"
puts "============================================================"
puts ""
puts "XSA file: $xsa_path"
puts ""
puts "Memory Map (from address editor):"
report_bd_addr_segments

puts ""
puts "Next: Use this XSA in Vitis to create platform and application"
puts ""
