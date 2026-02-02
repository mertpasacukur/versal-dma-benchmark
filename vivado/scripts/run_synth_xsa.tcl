################################################################################
# Versal DMA Benchmark - Synthesis and XSA Export
# Target: VPK120 (Versal Premium VP1202)
################################################################################

set project_dir [file normalize [file dirname [info script]]/../..]
set output_dir "${project_dir}/vivado/output"
set project_name "versal_dma_benchmark"

# Open project
puts "INFO: Opening project..."
open_project "${output_dir}/${project_name}/${project_name}.xpr"

################################################################################
# Synthesis
################################################################################

puts "INFO: Starting synthesis..."

# Reset synthesis run if it exists
if {[get_runs -quiet synth_1] != ""} {
    reset_run synth_1
}

# Launch synthesis
launch_runs synth_1 -jobs 8
wait_on_run synth_1

# Check synthesis status
if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    puts "ERROR: Synthesis failed!"
    exit 1
}

puts "INFO: Synthesis completed successfully"

################################################################################
# Export XSA (Pre-Implementation for faster Vitis platform)
################################################################################

puts "INFO: Exporting hardware platform (XSA) - pre-implementation..."

set xsa_dir "${output_dir}/xsa"
file mkdir ${xsa_dir}

# Export XSA without bitstream (for faster platform creation)
# This allows Vitis platform and app development in parallel with implementation
write_hw_platform -fixed -force -file ${xsa_dir}/dma_benchmark_preimpl.xsa

puts "INFO: Pre-implementation XSA exported: ${xsa_dir}/dma_benchmark_preimpl.xsa"

################################################################################
# Summary
################################################################################

puts ""
puts "============================================================"
puts "        SYNTHESIS AND XSA EXPORT COMPLETE"
puts "============================================================"
puts "XSA (pre-impl): ${xsa_dir}/dma_benchmark_preimpl.xsa"
puts "============================================================"
puts ""
puts "Next: Create Vitis platform with this XSA"
