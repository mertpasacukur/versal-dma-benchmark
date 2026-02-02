################################################################################
# Versal DMA Benchmark - Implementation and PDI Generation
# Target: VPK120 (Versal Premium VP1202)
################################################################################

set project_dir [file normalize [file dirname [info script]]/../..]
set output_dir "${project_dir}/vivado/output"
set project_name "versal_dma_benchmark"

# Open project
puts "INFO: Opening project..."
open_project "${output_dir}/${project_name}/${project_name}.xpr"

################################################################################
# Implementation
################################################################################

puts "INFO: Starting implementation..."

# Reset implementation run if needed
if {[get_runs -quiet impl_1] != ""} {
    if {[get_property STATUS [get_runs impl_1]] != "Not started"} {
        reset_run impl_1
    }
}

# Launch implementation
launch_runs impl_1 -jobs 8
wait_on_run impl_1

# Check implementation status
if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    puts "ERROR: Implementation failed!"
    exit 1
}

puts "INFO: Implementation completed successfully"

################################################################################
# Timing Analysis
################################################################################

puts "INFO: Running timing analysis..."

open_run impl_1

set reports_dir "${output_dir}/${project_name}/reports"
file mkdir ${reports_dir}

report_timing_summary -file ${reports_dir}/timing_summary.rpt
report_utilization -file ${reports_dir}/utilization.rpt

# Check timing
set wns [get_property STATS.WNS [get_runs impl_1]]
set tns [get_property STATS.TNS [get_runs impl_1]]

puts "INFO: WNS = ${wns} ns, TNS = ${tns} ns"

if {$wns < 0} {
    puts "WARNING: Timing violations detected!"
}

################################################################################
# Generate Bitstream/PDI
################################################################################

puts "INFO: Generating bitstream and PDI..."

launch_runs impl_1 -to_step write_device_image -jobs 8
wait_on_run impl_1

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
# Summary
################################################################################

puts ""
puts "============================================================"
puts "        IMPLEMENTATION AND PDI COMPLETE"
puts "============================================================"
puts "WNS: ${wns} ns"
puts "TNS: ${tns} ns"
puts "------------------------------------------------------------"
puts "PDI: ${xsa_dir}/dma_benchmark.pdi"
puts "XSA: ${xsa_dir}/dma_benchmark.xsa"
puts "Reports: ${reports_dir}/"
puts "============================================================"
