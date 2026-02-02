# Generate PDI from routed DCP - using shorter path via subst drive
# Run this after: subst V: "C:\Users\mpcukur\claude_code_ws\versal_dma\vivado"

set project_dir "V:/versal_all_dma_v3"
set export_dir "V:/export"

file mkdir $export_dir

# Open routed design
open_checkpoint "$project_dir/versal_all_dma_v3.runs/impl_1/design_1_wrapper_routed.dcp"

# Generate device image
puts "Generating device image..."
write_device_image -force -file "$export_dir/design_all_dma_v3.pdi"

# Generate XSA
puts "Generating XSA..."
write_hw_platform -fixed -include_bit -force -file "$export_dir/design_all_dma_v3.xsa"

puts "Done!"
puts "PDI: $export_dir/design_all_dma_v3.pdi"
puts "XSA: $export_dir/design_all_dma_v3.xsa"
