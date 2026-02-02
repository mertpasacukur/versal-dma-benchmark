################################################################################
# Export XSA only (PDI will need workaround)
################################################################################

set project_dir "C:/Users/mpcukur/claude_code_ws/versal_dma/vivado/output/versal_dma_benchmark"
set output_dir "C:/Users/mpcukur/claude_code_ws/versal_dma/vivado/output"

puts "============================================================"
puts "  EXPORTING XSA"
puts "============================================================"

# Open project
puts "INFO: Opening project..."
open_project ${project_dir}/versal_dma_benchmark.xpr

# Get current timestamp
set timestamp [clock format [clock seconds] -format "%Y%m%d_%H%M%S"]

# Check if impl_1 is routed
set impl_status [get_property STATUS [get_runs impl_1]]
puts "INFO: Implementation status: $impl_status"

# Open the routed implementation
puts "INFO: Opening implementation run..."
open_run impl_1

# Export XSA with bitstream
puts "INFO: Exporting XSA..."
write_hw_platform -fixed -include_bit -force \
    -file ${output_dir}/versal_dma_fixed_${timestamp}.xsa

puts "============================================================"
puts "  XSA EXPORT COMPLETE"
puts "============================================================"
puts "XSA: ${output_dir}/versal_dma_fixed_${timestamp}.xsa"
puts "============================================================"
puts ""
puts "NOTE: PDI generation failed due to Windows path length limit."
puts "      To generate PDI, either:"
puts "      1. Enable long paths in Windows"
puts "      2. Move project to shorter path like C:/vivado/"
puts "      3. Use subst command and regenerate"
puts "============================================================"
