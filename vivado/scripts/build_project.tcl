################################################################################
# Versal DMA Benchmark - Build Script
# Target: VPK120 (Versal Premium VP1202)
################################################################################

set project_dir [file normalize [file dirname [info script]]/../..]
set output_dir "${project_dir}/vivado/output"
set project_name "versal_dma_benchmark"

# Check if project exists, if not create it
if {![file exists "${output_dir}/${project_name}/${project_name}.xpr"]} {
    puts "INFO: Project not found, creating..."
    source ${project_dir}/vivado/scripts/create_project.tcl
} else {
    puts "INFO: Opening existing project..."
    open_project "${output_dir}/${project_name}/${project_name}.xpr"
}

################################################################################
# Synthesis
################################################################################

puts "INFO: Starting synthesis..."

# Reset synthesis run if it exists
if {[get_runs -quiet synth_1] != ""} {
    reset_run synth_1
}

# Configure synthesis settings (Vivado 2023.2 compatible)
set_property STEPS.SYNTH_DESIGN.ARGS.FLATTEN_HIERARCHY rebuilt [get_runs synth_1]

# Launch synthesis
launch_runs synth_1 -jobs 8
wait_on_run synth_1

# Check synthesis status
if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    puts "ERROR: Synthesis failed!"
    return -code error
}

puts "INFO: Synthesis completed successfully"

################################################################################
# Implementation
################################################################################

puts "INFO: Starting implementation..."

# Reset implementation run if it exists
if {[get_runs -quiet impl_1] != ""} {
    reset_run impl_1
}

# Configure implementation settings (Vivado 2023.2 compatible)
# Using default strategy

# Enable incremental implementation if checkpoint exists
set checkpoint_file "${output_dir}/${project_name}/${project_name}.runs/impl_1/dma_benchmark_bd_wrapper_routed.dcp"
if {[file exists ${checkpoint_file}]} {
    set_property incremental_checkpoint ${checkpoint_file} [get_runs impl_1]
    puts "INFO: Using incremental implementation"
}

# Launch implementation
launch_runs impl_1 -jobs 8
wait_on_run impl_1

# Check implementation status
if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    puts "ERROR: Implementation failed!"
    return -code error
}

puts "INFO: Implementation completed successfully"

################################################################################
# Timing Analysis
################################################################################

puts "INFO: Running timing analysis..."

open_run impl_1

# Generate timing reports
set reports_dir "${output_dir}/${project_name}/reports"
file mkdir ${reports_dir}

report_timing_summary -file ${reports_dir}/timing_summary.rpt
report_timing -max_paths 100 -file ${reports_dir}/timing_paths.rpt
report_clock_utilization -file ${reports_dir}/clock_utilization.rpt
report_utilization -file ${reports_dir}/utilization.rpt
report_power -file ${reports_dir}/power.rpt
report_drc -file ${reports_dir}/drc.rpt

# Check for timing violations
set wns [get_property STATS.WNS [get_runs impl_1]]
set tns [get_property STATS.TNS [get_runs impl_1]]

puts "INFO: Timing Summary:"
puts "INFO:   WNS (Worst Negative Slack): ${wns} ns"
puts "INFO:   TNS (Total Negative Slack): ${tns} ns"

if {$wns < 0} {
    puts "WARNING: Timing violations detected! WNS = ${wns} ns"
    puts "WARNING: Review timing reports in ${reports_dir}"
}

################################################################################
# Generate Bitstream
################################################################################

puts "INFO: Generating bitstream..."

launch_runs impl_1 -to_step write_bitstream -jobs 8
wait_on_run impl_1

# Check bitstream generation status
set bitstream_file "${output_dir}/${project_name}/${project_name}.runs/impl_1/dma_benchmark_bd_wrapper.bit"
if {![file exists ${bitstream_file}]} {
    puts "ERROR: Bitstream generation failed!"
    return -code error
}

puts "INFO: Bitstream generated: ${bitstream_file}"

################################################################################
# Export Hardware (XSA)
################################################################################

puts "INFO: Exporting hardware platform (XSA)..."

set xsa_dir "${output_dir}/xsa"
file mkdir ${xsa_dir}

# Export XSA with bitstream
write_hw_platform -fixed -include_bit -force -file ${xsa_dir}/dma_benchmark.xsa

puts "INFO: Hardware platform exported: ${xsa_dir}/dma_benchmark.xsa"

################################################################################
# Export PDI (for Versal boot)
################################################################################

puts "INFO: Exporting PDI..."

set pdi_file "${output_dir}/${project_name}/${project_name}.runs/impl_1/dma_benchmark_bd_wrapper.pdi"
if {[file exists ${pdi_file}]} {
    file copy -force ${pdi_file} ${xsa_dir}/dma_benchmark.pdi
    puts "INFO: PDI exported: ${xsa_dir}/dma_benchmark.pdi"
}

################################################################################
# Build Summary
################################################################################

puts ""
puts "============================================================"
puts "                    BUILD SUMMARY"
puts "============================================================"
puts "Project:        ${project_name}"
puts "Target:         VPK120 (Versal Premium VP1202)"
puts "------------------------------------------------------------"
puts "Synthesis:      PASSED"
puts "Implementation: PASSED"
puts "Bitstream:      GENERATED"
puts "XSA Export:     COMPLETED"
puts "------------------------------------------------------------"
puts "WNS:            ${wns} ns"
puts "TNS:            ${tns} ns"
puts "------------------------------------------------------------"
puts "Output Files:"
puts "  Bitstream: ${bitstream_file}"
puts "  XSA:       ${xsa_dir}/dma_benchmark.xsa"
puts "  Reports:   ${reports_dir}/"
puts "============================================================"
puts ""

puts "INFO: Build completed successfully!"
puts "INFO: Next step: Create Vitis platform using the XSA file"
