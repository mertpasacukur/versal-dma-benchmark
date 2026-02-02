################################################################################
# FIX AND REBUILD: DMA to DDR Connection via S_AXI_FPD
# This is the definitive fix for DMA decode errors
################################################################################

puts "============================================================"
puts "  FIXING DMA TO DDR CONNECTION"
puts "============================================================"

set project_dir "C:/Users/mpcukur/claude_code_ws/versal_dma/vivado/output/versal_dma_benchmark"
set bd_name "dma_benchmark_bd"

# Open project
puts "INFO: Opening project..."
open_project ${project_dir}/versal_dma_benchmark.xpr

# Open block design
puts "INFO: Opening block design..."
open_bd_design [get_files ${bd_name}.bd]

################################################################################
# Step 1: Check and Enable S_AXI_FPD on CIPS
################################################################################

puts "INFO: Configuring CIPS for S_AXI_FPD..."

set cips [get_bd_cells versal_cips_0]

# Check if S_AXI_FPD exists
set s_axi_fpd_pin [get_bd_intf_pins versal_cips_0/S_AXI_FPD -quiet]

if {$s_axi_fpd_pin eq ""} {
    puts "INFO: S_AXI_FPD not enabled, enabling now..."

    # Enable S_AXI_FPD in CIPS
    # This requires setting the appropriate PS-PL interface configuration
    set_property -dict [list \
        CONFIG.PS_PMC_CONFIG { \
            PS_USE_S_AXI_FPD {1} \
        } \
    ] [get_bd_cells versal_cips_0]

    puts "INFO: S_AXI_FPD enabled"
} else {
    puts "INFO: S_AXI_FPD already exists"
}

################################################################################
# Step 2: Disconnect axi_smc_dma from current target
################################################################################

puts "INFO: Checking axi_smc_dma connections..."

set smc_dma_m00 [get_bd_intf_pins axi_smc_dma/M00_AXI]
set current_net [get_bd_intf_nets -of_objects $smc_dma_m00 -quiet]

if {$current_net ne ""} {
    puts "INFO: Disconnecting axi_smc_dma/M00_AXI from: $current_net"
    delete_bd_objs $current_net
}

################################################################################
# Step 3: Connect axi_smc_dma to S_AXI_FPD
################################################################################

puts "INFO: Connecting axi_smc_dma to S_AXI_FPD..."

# Get or create S_AXI_FPD pin
set s_axi_fpd [get_bd_intf_pins versal_cips_0/S_AXI_FPD -quiet]

if {$s_axi_fpd ne ""} {
    connect_bd_intf_net [get_bd_intf_pins axi_smc_dma/M00_AXI] $s_axi_fpd
    puts "INFO: Connected axi_smc_dma/M00_AXI -> S_AXI_FPD"
} else {
    puts "ERROR: S_AXI_FPD still not available!"
    puts "       Manual fix required in Vivado GUI"
}

################################################################################
# Step 4: Connect S_AXI_FPD clock
################################################################################

puts "INFO: Connecting S_AXI_FPD clock..."

set pl_clk [get_bd_pins versal_cips_0/pl0_ref_clk]
set s_axi_fpd_clk [get_bd_pins versal_cips_0/s_axi_fpd_aclk -quiet]

if {$s_axi_fpd_clk ne ""} {
    # Check if already connected
    set clk_net [get_bd_nets -of_objects $s_axi_fpd_clk -quiet]
    if {$clk_net eq ""} {
        connect_bd_net $pl_clk $s_axi_fpd_clk
        puts "INFO: Connected pl0_ref_clk -> s_axi_fpd_aclk"
    } else {
        puts "INFO: s_axi_fpd_aclk already connected"
    }
}

################################################################################
# Step 5: Assign DDR Address to DMA Masters
################################################################################

puts "INFO: Assigning DDR addresses to DMA..."

# Clear any excluded segments first
set excluded [get_bd_addr_segs -excluded -quiet]
if {$excluded ne ""} {
    puts "INFO: Including excluded segments..."
    include_bd_addr_seg $excluded -quiet
}

# Auto-assign addresses
assign_bd_address -quiet

# Verify DMA has DDR access
puts "INFO: Verifying address assignments..."

# Check AXI DMA address spaces
foreach space {Data_SG Data_MM2S Data_S2MM} {
    set addr_space [get_bd_addr_spaces axi_dma_0/$space -quiet]
    if {$addr_space ne ""} {
        set segs [get_bd_addr_segs -of_objects $addr_space -quiet]
        puts "  axi_dma_0/$space segments: $segs"
    }
}

# Check AXI CDMA address spaces
foreach space {Data_SG Data} {
    set addr_space [get_bd_addr_spaces axi_cdma_0/$space -quiet]
    if {$addr_space ne ""} {
        set segs [get_bd_addr_segs -of_objects $addr_space -quiet]
        puts "  axi_cdma_0/$space segments: $segs"
    }
}

################################################################################
# Step 6: Validate and Save
################################################################################

puts "INFO: Validating block design..."
validate_bd_design

puts "INFO: Saving block design..."
save_bd_design

# Regenerate output products
puts "INFO: Regenerating output products..."
generate_target all [get_files ${bd_name}.bd]

################################################################################
# Step 7: Run Synthesis
################################################################################

puts "INFO: Running synthesis..."
reset_run synth_1
launch_runs synth_1 -jobs 8
wait_on_run synth_1

set synth_status [get_property STATUS [get_runs synth_1]]
puts "INFO: Synthesis status: $synth_status"

if {$synth_status ne "synth_design Complete!"} {
    puts "ERROR: Synthesis failed!"
    return -1
}

################################################################################
# Step 8: Run Implementation
################################################################################

puts "INFO: Running implementation..."
reset_run impl_1
launch_runs impl_1 -jobs 8
wait_on_run impl_1

set impl_status [get_property STATUS [get_runs impl_1]]
puts "INFO: Implementation status: $impl_status"

################################################################################
# Step 9: Generate PDI
################################################################################

puts "INFO: Generating device image (PDI)..."
launch_runs impl_1 -to_step write_device_image -jobs 8
wait_on_run impl_1

################################################################################
# Step 10: Export XSA
################################################################################

puts "INFO: Exporting hardware platform (XSA)..."

set output_dir "C:/Users/mpcukur/claude_code_ws/versal_dma/vivado/output"
set timestamp [clock format [clock seconds] -format "%Y%m%d_%H%M%S"]

# Export XSA with bitstream
write_hw_platform -fixed -include_bit -force \
    -file ${output_dir}/versal_dma_fixed_${timestamp}.xsa

# Copy PDI
set pdi_file [glob -nocomplain ${project_dir}/versal_dma_benchmark.runs/impl_1/*.pdi]
if {$pdi_file ne ""} {
    file copy -force $pdi_file ${output_dir}/versal_dma_fixed_${timestamp}.pdi
    puts "INFO: PDI copied to ${output_dir}/versal_dma_fixed_${timestamp}.pdi"
}

puts "============================================================"
puts "  BUILD COMPLETE"
puts "============================================================"
puts "XSA: ${output_dir}/versal_dma_fixed_${timestamp}.xsa"
puts "PDI: ${output_dir}/versal_dma_fixed_${timestamp}.pdi"
puts "============================================================"
