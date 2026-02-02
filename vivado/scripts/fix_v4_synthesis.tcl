# Fix V4 Synthesis - Retry with fewer parallel jobs
# The block design is already valid, just synthesis failed due to memory/path issues

puts "============================================================"
puts "  Fixing V4 Synthesis - Using fewer parallel jobs"
puts "============================================================"

# Open the existing project
set project_dir [file normalize "V:/versal_all_dma_v4"]
open_project "$project_dir/versal_all_dma_v4.xpr"

# Reset all failed synthesis runs
puts "Resetting failed synthesis runs..."
reset_run design_1_axi_dma_0_0_synth_1 -quiet
reset_run design_1_axi_cdma_0_0_synth_1 -quiet
reset_run design_1_axi_smc_ctrl_0_synth_1 -quiet
reset_run synth_1 -quiet

# Clean up to free memory
puts "Cleaning IP cache to free memory..."
config_ip_cache -clear_output_repo

# Set synthesis strategies for lower memory usage
set_property strategy Flow_AreaOptimized_high [get_runs synth_1]

puts ""
puts "============================================================"
puts "  Running Synthesis with 2 parallel jobs..."
puts "============================================================"

# Launch synthesis with only 2 jobs to reduce memory usage
launch_runs synth_1 -jobs 2
wait_on_run synth_1

# Check synthesis result
if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    puts "ERROR: Synthesis failed!"
    set synth_log [get_property DIRECTORY [get_runs synth_1]]/runme.log
    puts "Check log: $synth_log"
    exit 1
}

puts "Synthesis completed successfully!"

puts ""
puts "============================================================"
puts "  Running Implementation with 2 parallel jobs..."
puts "============================================================"

launch_runs impl_1 -jobs 2
wait_on_run impl_1

# Check implementation result
if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    puts "ERROR: Implementation failed!"
    exit 1
}

puts "Implementation completed successfully!"

puts ""
puts "============================================================"
puts "  Generating PDI and XSA..."
puts "============================================================"

# Generate device image (PDI)
open_run impl_1
write_device_image -force -file "$project_dir/../export/design_all_dma_v4.pdi"

# Write hardware platform (XSA)
write_hw_platform -fixed -include_bit -force -file "$project_dir/../export/design_all_dma_v4.xsa"

puts ""
puts "============================================================"
puts "  V4 BUILD COMPLETE!"
puts "============================================================"
puts "  PDI: V:/export/design_all_dma_v4.pdi"
puts "  XSA: V:/export/design_all_dma_v4.xsa"
puts ""
puts "  KEY FEATURE: LPD_AXI_NOC enabled for ADMA DDR access"
puts "============================================================"

close_project
