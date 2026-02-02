################################################################################
# Generate PDI and XSA from short path
################################################################################

# Use shorter path via subst
set project_path "V:/versal_dma_benchmark.xpr"
set output_dir "C:/Users/mpcukur/claude_code_ws/versal_dma/vivado/output"

puts "============================================================"
puts "  GENERATING PDI AND XSA"
puts "============================================================"

# Open project from short path
puts "INFO: Opening project from short path..."
open_project $project_path

# Get current timestamp for filenames
set timestamp [clock format [clock seconds] -format "%Y%m%d_%H%M%S"]

# First, export XSA (this usually works even with path issues)
puts "INFO: Exporting XSA..."
write_hw_platform -fixed -include_bit -force \
    -file ${output_dir}/versal_dma_fixed_${timestamp}.xsa
puts "INFO: XSA exported to ${output_dir}/versal_dma_fixed_${timestamp}.xsa"

# Now try PDI generation
puts "INFO: Generating PDI..."
open_run impl_1

# Try write_device_image with working directory set to shorter path
cd V:/versal_dma_benchmark.runs/impl_1
write_device_image -force -file ${output_dir}/versal_dma_fixed_${timestamp}.pdi

puts "============================================================"
puts "  COMPLETE"
puts "============================================================"
puts "XSA: ${output_dir}/versal_dma_fixed_${timestamp}.xsa"
puts "PDI: ${output_dir}/versal_dma_fixed_${timestamp}.pdi"
puts "============================================================"
