################################################################################
# Generate PDI from routed design
# Target: VPK120 (Versal Premium VP1202)
################################################################################

set project_dir [file normalize [file dirname [info script]]/../..]
set output_dir "${project_dir}/vivado/output"
set project_name "versal_dma_benchmark"

# Open project
puts "INFO: Opening project..."
open_project "${output_dir}/${project_name}/${project_name}.xpr"

################################################################################
# Generate PDI
################################################################################

puts "INFO: Opening routed design..."
open_run impl_1

puts "INFO: Generating device image (PDI)..."
write_device_image -force -file "${output_dir}/${project_name}/${project_name}.runs/impl_1/dma_benchmark_bd_wrapper.pdi"

################################################################################
# Export PDI and XSA with bitstream
################################################################################

puts "INFO: Exporting PDI and XSA..."

set xsa_dir "${output_dir}/xsa"
file mkdir ${xsa_dir}

# Find PDI file
set pdi_file "${output_dir}/${project_name}/${project_name}.runs/impl_1/dma_benchmark_bd_wrapper.pdi"

if {[file exists ${pdi_file}]} {
    file copy -force ${pdi_file} ${xsa_dir}/dma_benchmark.pdi
    puts "INFO: PDI exported: ${xsa_dir}/dma_benchmark.pdi"
} else {
    puts "WARNING: PDI file not found at expected location"
    # Try to find it
    set found_pdi [glob -nocomplain "${output_dir}/${project_name}/${project_name}.runs/impl_1/*.pdi"]
    if {[llength $found_pdi] > 0} {
        set pdi_file [lindex $found_pdi 0]
        file copy -force ${pdi_file} ${xsa_dir}/dma_benchmark.pdi
        puts "INFO: PDI exported: ${xsa_dir}/dma_benchmark.pdi"
    }
}

# Export XSA with bitstream
write_hw_platform -fixed -include_bit -force -file ${xsa_dir}/dma_benchmark.xsa

puts "INFO: XSA with bitstream exported: ${xsa_dir}/dma_benchmark.xsa"

################################################################################
# Get timing info
################################################################################

set wns [get_property STATS.WNS [get_runs impl_1]]
set tns [get_property STATS.TNS [get_runs impl_1]]

################################################################################
# Summary
################################################################################

puts ""
puts "============================================================"
puts "        PDI GENERATION COMPLETE"
puts "============================================================"
puts "WNS: ${wns} ns"
puts "TNS: ${tns} ns"
puts "------------------------------------------------------------"
puts "PDI: ${xsa_dir}/dma_benchmark.pdi"
puts "XSA: ${xsa_dir}/dma_benchmark.xsa"
puts "============================================================"
