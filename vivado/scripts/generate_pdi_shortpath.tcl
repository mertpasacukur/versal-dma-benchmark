################################################################################
# Generate PDI using short path
# The project should be accessed via V: drive (subst)
################################################################################

# Use short path
cd V:
set output_dir "C:/Users/mpcukur/claude_code_ws/versal_dma/vivado/output"

puts "============================================================"
puts "  GENERATING PDI FROM SHORT PATH"
puts "============================================================"

# Open project from short path
puts "INFO: Opening project from V: drive..."
open_project V:/versal_dma_benchmark.xpr

# Open the routed implementation
puts "INFO: Opening implementation run..."
open_run impl_1

# Get timestamp
set timestamp [clock format [clock seconds] -format "%Y%m%d_%H%M%S"]

# Change working directory to short path for PLM build
cd V:/versal_dma_benchmark.runs/impl_1

# Try write_device_image
puts "INFO: Generating PDI..."
write_device_image -force -file ${output_dir}/versal_dma_fixed_${timestamp}.pdi

puts "============================================================"
puts "  PDI GENERATION COMPLETE"
puts "============================================================"
puts "PDI: ${output_dir}/versal_dma_fixed_${timestamp}.pdi"
puts "============================================================"
