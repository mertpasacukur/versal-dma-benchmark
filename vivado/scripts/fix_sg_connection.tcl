#===============================================================================
# Fix AXI DMA Scatter-Gather Connection
#
# PROBLEM: M_AXI_SG interface is not connected, causing NoC slave errors (0xD)
#          when DMA tries to read descriptors from memory.
#
# SOLUTION: Connect M_AXI_SG to the SmartConnect that goes to NoC.
#===============================================================================

set script_dir [file dirname [file normalize [info script]]]
set project_dir [file normalize "$script_dir/../versal_all_dma"]
set project_name "versal_all_dma"

puts "============================================================"
puts "  Fixing AXI DMA Scatter-Gather Connection"
puts "============================================================"

# Open existing project
open_project "$project_dir/$project_name.xpr"

# Open block design
open_bd_design [get_files design_1.bd]

puts "Current design opened"

#===============================================================================
# Fix: Expand SmartConnect and connect M_AXI_SG
#===============================================================================

# The axi_smc_dma SmartConnect needs 3 slave inputs now (MM2S, S2MM, SG)
set_property CONFIG.NUM_SI {3} [get_bd_cells axi_smc_dma]

# Connect M_AXI_SG to the SmartConnect
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXI_SG] [get_bd_intf_pins axi_smc_dma/S02_AXI]

puts "Connected M_AXI_SG to SmartConnect"

#===============================================================================
# Also fix MCDMA SG connection if it exists
#===============================================================================
# MCDMA also has M_AXI_SG that needs to be connected
set mcdma_sg [get_bd_intf_pins -quiet axi_mcdma_0/M_AXI_SG]
if {$mcdma_sg != ""} {
    # Check if already connected
    set mcdma_sg_net [get_bd_intf_nets -quiet -of_objects $mcdma_sg]
    if {$mcdma_sg_net == ""} {
        puts "MCDMA M_AXI_SG not connected, fixing..."
        set_property CONFIG.NUM_SI {3} [get_bd_cells axi_smc_mcdma]
        connect_bd_intf_net [get_bd_intf_pins axi_mcdma_0/M_AXI_SG] [get_bd_intf_pins axi_smc_mcdma/S02_AXI]
        puts "Connected MCDMA M_AXI_SG"
    }
}

#===============================================================================
# Also fix CDMA SG connection if it exists (CDMA has only M_AXI, no separate SG)
#===============================================================================
# CDMA uses M_AXI for both data and SG, so no fix needed

#===============================================================================
# Validate and regenerate
#===============================================================================
regenerate_bd_layout
assign_bd_address

puts "Validating design..."
validate_bd_design
save_bd_design

#===============================================================================
# Regenerate output products
#===============================================================================
puts "Regenerating output products..."
generate_target all [get_files design_1.bd]

#===============================================================================
# Re-run synthesis
#===============================================================================
puts ""
puts "============================================================"
puts "  Re-running Synthesis..."
puts "============================================================"

reset_run synth_1
launch_runs synth_1 -jobs 8
wait_on_run synth_1
puts "Synthesis: [get_property STATUS [get_runs synth_1]]"

if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    puts "ERROR: Synthesis failed!"
    close_project
    exit 1
}

#===============================================================================
# Re-run implementation
#===============================================================================
puts ""
puts "============================================================"
puts "  Re-running Implementation..."
puts "============================================================"

reset_run impl_1
launch_runs impl_1 -jobs 8
wait_on_run impl_1
puts "Implementation: [get_property STATUS [get_runs impl_1]]"

if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    puts "ERROR: Implementation failed!"
    close_project
    exit 1
}

#===============================================================================
# Generate device image and XSA
#===============================================================================
puts "Generating device image..."
launch_runs impl_1 -to_step write_device_image -jobs 8
wait_on_run impl_1

set xsa_path "$project_dir/../design_all_dma_fixed.xsa"
write_hw_platform -fixed -include_bit -force -file $xsa_path

# Also export PDI
set pdi_path "$project_dir/$project_name.runs/impl_1/design_1_wrapper.pdi"
if {[file exists $pdi_path]} {
    file copy -force $pdi_path "$project_dir/../design_all_dma_fixed.pdi"
    puts "PDI exported to: $project_dir/../design_all_dma_fixed.pdi"
}

puts ""
puts "============================================================"
puts "  FIX COMPLETE!"
puts "============================================================"
puts ""
puts "XSA: $xsa_path"
puts ""
puts "The following connections were fixed:"
puts "  - AXI DMA M_AXI_SG -> axi_smc_dma/S02_AXI -> NoC -> LPDDR4"
puts "  - AXI MCDMA M_AXI_SG (if applicable)"
puts ""
puts "Please rebuild the Vitis application with the new XSA."
puts ""

close_project
