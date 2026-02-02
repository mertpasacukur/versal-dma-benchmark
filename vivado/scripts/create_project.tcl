################################################################################
# Versal DMA Benchmark - Project Creation Script
# Target: VPK120 (Versal Premium VP1202)
################################################################################

# Project Settings
set project_name "versal_dma_benchmark"
set project_dir  [file normalize [file dirname [info script]]/../..]
set output_dir   "${project_dir}/vivado/output"

# VPK120 Board Part
set board_part "xilinx.com:vpk120:part0:1.1"
set part_name  "xcvp1202-vsva2785-2MP-e-S"

# Create output directory
file mkdir ${output_dir}

# Create project
puts "INFO: Creating Vivado project: ${project_name}"
create_project ${project_name} ${output_dir}/${project_name} -part ${part_name}

# Set board part
set_property board_part ${board_part} [current_project]

# Set project properties
set_property target_language Verilog [current_project]
set_property simulator_language Mixed [current_project]
set_property default_lib work [current_project]

# Set IP repository paths (empty for now, using default)
set_property ip_repo_paths {} [current_project]
update_ip_catalog

# Set synthesis and implementation strategies (Vivado 2023.2 compatible)
set_property strategy Vivado\ Synthesis\ Defaults [get_runs synth_1]
set_property strategy Vivado\ Implementation\ Defaults [get_runs impl_1]

# Add constraints file
add_files -fileset constrs_1 -norecurse ${project_dir}/vivado/constraints/vpk120_dma.xdc
set_property used_in_synthesis true [get_files */vpk120_dma.xdc]
set_property used_in_implementation true [get_files */vpk120_dma.xdc]

puts "INFO: Project created successfully"
puts "INFO: Project location: ${output_dir}/${project_name}"

# Source the block design creation script
puts "INFO: Creating block design..."
source ${project_dir}/vivado/scripts/create_block_design.tcl

puts "INFO: Project setup complete"
