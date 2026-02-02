################################################################################
# Route DMA through axi_smc_ctrl which has DDR access via NoC
# axi_smc_ctrl/M00_AXI -> NoC/S01_AXI -> C2_DDR
################################################################################

set project_dir "C:/vdma/versal_dma_benchmark"
set output_dir "C:/Users/mpcukur/claude_code_ws/versal_dma/vivado/output"

puts "============================================================"
puts "  ROUTING DMA VIA AXI_SMC_CTRL FOR DDR ACCESS"
puts "============================================================"

# Restore fresh project
cd C:/vdma
file delete -force versal_dma_benchmark
file copy -force "C:/Users/mpcukur/claude_code_ws/versal_dma/vivado/output/versal_dma_benchmark" "C:/vdma/"

open_project ${project_dir}/versal_dma_benchmark.xpr
open_bd_design [get_files dma_benchmark_bd.bd]

################################################################################
# Step 1: Analyze current axi_smc_ctrl configuration
################################################################################

puts ""
puts "=== Current axi_smc_ctrl Configuration ==="

set smc_ctrl [get_bd_cells axi_smc_ctrl]
set ctrl_num_si [get_property CONFIG.NUM_SI $smc_ctrl]
set ctrl_num_mi [get_property CONFIG.NUM_MI $smc_ctrl]
puts "NUM_SI: $ctrl_num_si, NUM_MI: $ctrl_num_mi"

# Check what's connected to M00_AXI (should go to NoC)
set ctrl_m00_net [get_bd_intf_nets -of_objects [get_bd_intf_pins axi_smc_ctrl/M00_AXI] -quiet]
set ctrl_m00_dest [get_bd_intf_pins -of_objects $ctrl_m00_net -filter {MODE == Slave} -quiet]
puts "M00_AXI -> $ctrl_m00_dest"

################################################################################
# Step 2: Disconnect axi_smc_dma from S_AXI_FPD
################################################################################

puts ""
puts "=== Disconnecting axi_smc_dma from S_AXI_FPD ==="

set smc_dma_net [get_bd_intf_nets -of_objects [get_bd_intf_pins axi_smc_dma/M00_AXI] -quiet]
if {$smc_dma_net ne ""} {
    puts "Removing: $smc_dma_net"
    delete_bd_objs $smc_dma_net
}

################################################################################
# Step 3: Add slave interfaces to axi_smc_ctrl for DMA
################################################################################

puts ""
puts "=== Adding DMA inputs to axi_smc_ctrl ==="

# axi_smc_ctrl currently has 1 SI (from M_AXI_FPD)
# We need to add 1 more SI for axi_smc_dma output

set new_num_si [expr {$ctrl_num_si + 1}]
set new_si_name "S[format %02d $ctrl_num_si]_AXI"

puts "Adding $new_si_name to axi_smc_ctrl (NUM_SI: $ctrl_num_si -> $new_num_si)"

set_property CONFIG.NUM_SI $new_num_si $smc_ctrl

################################################################################
# Step 4: Connect axi_smc_dma to new axi_smc_ctrl SI
################################################################################

puts ""
puts "=== Connecting axi_smc_dma to axi_smc_ctrl ==="

connect_bd_intf_net \
    [get_bd_intf_pins axi_smc_dma/M00_AXI] \
    [get_bd_intf_pins axi_smc_ctrl/$new_si_name]

puts "Connected: axi_smc_dma/M00_AXI -> axi_smc_ctrl/$new_si_name"

################################################################################
# Step 5: Assign addresses
################################################################################

puts ""
puts "=== Assigning Addresses ==="

# Auto-assign should now route DMA through the proper path
assign_bd_address

################################################################################
# Step 6: Check DMA DDR segments
################################################################################

puts ""
puts "=== DMA Address Segments ==="

set has_ddr 0
foreach {ip spaces} {axi_dma_0 {Data_SG Data_MM2S Data_S2MM} axi_cdma_0 {Data_SG Data}} {
    foreach space $spaces {
        set segs [get_bd_addr_segs ${ip}/${space}/* -quiet]
        if {$segs ne ""} {
            foreach seg $segs {
                set name [get_property NAME $seg]
                set offset [get_property OFFSET $seg]
                set range [get_property RANGE $seg]
                if {[string match "*DDR*" $name] || [string match "*C*_DDR*" $name]} {
                    puts "  ${ip}/${space}: $name @ $offset ($range)"
                    set has_ddr 1
                }
            }
        }
    }
}

if {!$has_ddr} {
    puts "  WARNING: No DDR segments found for DMA!"
}

################################################################################
# Step 7: Validate and Save
################################################################################

puts ""
puts "=== Validating ==="

validate_bd_design
save_bd_design

puts ""
puts "============================================================"
puts "  Connection Path:"
puts "  DMA -> axi_smc_dma -> axi_smc_ctrl -> NoC -> DDR"
puts "============================================================"
