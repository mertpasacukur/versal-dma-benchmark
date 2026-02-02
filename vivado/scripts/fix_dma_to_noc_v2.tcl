################################################################################
# CORRECT FIX v2: Connect DMA to NoC for DDR access
# Clear old address mappings and create proper NoC path
################################################################################

set project_dir "C:/vdma/versal_dma_benchmark"
set output_dir "C:/Users/mpcukur/claude_code_ws/versal_dma/vivado/output"

puts "============================================================"
puts "  CORRECT FIX v2: DMA to NoC for DDR Access"
puts "============================================================"

open_project ${project_dir}/versal_dma_benchmark.xpr
open_bd_design [get_files dma_benchmark_bd.bd]

################################################################################
# Step 1: Delete ALL address segments for DMA first
################################################################################

puts "INFO: Clearing all DMA address segments..."

# Delete all address segments for AXI DMA
foreach space {Data_SG Data_MM2S Data_S2MM} {
    set segs [get_bd_addr_segs /axi_dma_0/${space}/* -quiet]
    if {$segs ne ""} {
        foreach seg $segs {
            delete_bd_objs $seg -quiet
        }
    }
}

# Delete all address segments for AXI CDMA
foreach space {Data_SG Data} {
    set segs [get_bd_addr_segs /axi_cdma_0/${space}/* -quiet]
    if {$segs ne ""} {
        foreach seg $segs {
            delete_bd_objs $seg -quiet
        }
    }
}

puts "  Address segments cleared"

################################################################################
# Step 2: Disconnect axi_smc_dma from current target
################################################################################

puts "INFO: Disconnecting axi_smc_dma..."

set smc_m00 [get_bd_intf_pins axi_smc_dma/M00_AXI]
set current_net [get_bd_intf_nets -of_objects $smc_m00 -quiet]

if {$current_net ne ""} {
    puts "  Removing: $current_net"
    delete_bd_objs $current_net
}

################################################################################
# Step 3: Add new slave port to NoC for DMA
################################################################################

puts "INFO: Adding DMA port to NoC..."

set noc [get_bd_cells axi_noc_0]
set current_si [get_property CONFIG.NUM_SI $noc]
puts "  Current NUM_SI: $current_si"

# Add new SI
set new_si [expr {$current_si + 1}]
set dma_si_idx [expr {$new_si - 1}]
set dma_si_name "S[format %02d $dma_si_idx]_AXI"

puts "  Adding $dma_si_name (NUM_SI -> $new_si)"

# Update NoC configuration
set_property CONFIG.NUM_SI $new_si $noc

# Configure new SI to route to Memory Controller (MC_0)
set_property CONFIG.SI${dma_si_idx}_CONNECTIONS {MC_0 {read_bw {1000} write_bw {1000}}} $noc

################################################################################
# Step 4: Connect axi_smc_dma to new NoC port
################################################################################

puts "INFO: Connecting axi_smc_dma to NoC/${dma_si_name}..."

connect_bd_intf_net [get_bd_intf_pins axi_smc_dma/M00_AXI] \
                    [get_bd_intf_pins axi_noc_0/${dma_si_name}]

# Connect clock
set pl_clk [get_bd_pins versal_cips_0/pl0_ref_clk]
connect_bd_net $pl_clk [get_bd_pins axi_noc_0/aclk${dma_si_idx}] -quiet

################################################################################
# Step 5: Assign ONLY DDR addresses to DMA
################################################################################

puts "INFO: Assigning DDR addresses to DMA..."

# Get the DDR/LPDDR segment from NoC
set noc_segs [get_bd_addr_segs -of_objects [get_bd_addr_spaces -of_objects $noc] -quiet]
puts "  Available NoC segments: $noc_segs"

# Find MC_0 (Memory Controller) segment
set mc_seg [get_bd_addr_segs axi_noc_0/S00_AXI/C0_DDR_LOW0 -quiet]
if {$mc_seg eq ""} {
    set mc_seg [get_bd_addr_segs axi_noc_0/*/C0_DDR_LOW0 -quiet]
}
puts "  MC segment: $mc_seg"

# Assign DDR to each DMA address space
# AXI DMA
foreach space {Data_SG Data_MM2S Data_S2MM} {
    set target [get_bd_addr_segs axi_noc_0/${dma_si_name}/C0_DDR_LOW0 -quiet]
    if {$target ne ""} {
        assign_bd_address -offset 0x00000000 -range 2G -target_address_space [get_bd_addr_spaces axi_dma_0/${space}] $target -quiet
        puts "  Assigned DDR to axi_dma_0/${space}"
    }
}

# AXI CDMA
foreach space {Data_SG Data} {
    set target [get_bd_addr_segs axi_noc_0/${dma_si_name}/C0_DDR_LOW0 -quiet]
    if {$target ne ""} {
        assign_bd_address -offset 0x00000000 -range 2G -target_address_space [get_bd_addr_spaces axi_cdma_0/${space}] $target -quiet
        puts "  Assigned DDR to axi_cdma_0/${space}"
    }
}

################################################################################
# Step 6: Validate
################################################################################

puts "INFO: Validating design..."
validate_bd_design

puts "INFO: Saving design..."
save_bd_design

################################################################################
# Step 7: Summary
################################################################################

puts ""
puts "============================================================"
puts "  CONFIGURATION COMPLETE"
puts "============================================================"
puts "NoC NUM_SI: [get_property CONFIG.NUM_SI $noc]"

set net [get_bd_intf_nets -of_objects [get_bd_intf_pins axi_smc_dma/M00_AXI] -quiet]
set dest [get_bd_intf_pins -of_objects $net -filter {MODE == Slave} -quiet]
puts "axi_smc_dma/M00_AXI -> $dest"

# Check DMA address assignments
puts ""
puts "DMA Address Assignments:"
foreach {ip spaces} {axi_dma_0 {Data_SG Data_MM2S Data_S2MM} axi_cdma_0 {Data_SG Data}} {
    foreach space $spaces {
        set segs [get_bd_addr_segs //${ip}/${space}/* -quiet]
        if {$segs ne ""} {
            foreach seg $segs {
                puts "  ${ip}/${space}: [get_property NAME $seg] @ [get_property OFFSET $seg]"
            }
        } else {
            puts "  ${ip}/${space}: NO SEGMENTS!"
        }
    }
}

puts ""
puts "============================================================"
puts "  Next: Run synthesis and implementation"
puts "============================================================"
