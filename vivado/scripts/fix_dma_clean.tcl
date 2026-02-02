################################################################################
# CLEAN FIX: DMA to NoC DDR Access
# Start from clean state, add one SI to NoC, connect DMA
################################################################################

set project_dir "C:/vdma/versal_dma_benchmark"
set output_dir "C:/Users/mpcukur/claude_code_ws/versal_dma/vivado/output"

puts "============================================================"
puts "  CLEAN FIX: DMA to NoC for DDR Access"
puts "============================================================"

open_project ${project_dir}/versal_dma_benchmark.xpr
open_bd_design [get_files dma_benchmark_bd.bd]

################################################################################
# Step 1: Check current state
################################################################################

puts ""
puts "=== Current State ==="

set noc [get_bd_cells axi_noc_0]
set num_si [get_property CONFIG.NUM_SI $noc]
set num_mc [get_property CONFIG.NUM_MC $noc]
puts "NoC: NUM_SI=$num_si, NUM_MC=$num_mc"

# Check where axi_smc_dma is connected
set smc_m00 [get_bd_intf_pins axi_smc_dma/M00_AXI]
set smc_net [get_bd_intf_nets -of_objects $smc_m00 -quiet]
if {$smc_net ne ""} {
    set dest [get_bd_intf_pins -of_objects $smc_net -filter {MODE == Slave} -quiet]
    puts "axi_smc_dma/M00_AXI -> $dest"
} else {
    puts "axi_smc_dma/M00_AXI -> NOT CONNECTED"
}

################################################################################
# Step 2: Remove DMA address segments and disconnect
################################################################################

puts ""
puts "=== Cleaning Up ==="

# Delete DMA address segments
puts "Removing DMA address segments..."
set all_dma_segs [get_bd_addr_segs -of_objects [get_bd_addr_spaces -of_objects [get_bd_cells axi_dma_0]] -quiet]
foreach seg $all_dma_segs {
    delete_bd_objs $seg -quiet
}

set all_cdma_segs [get_bd_addr_segs -of_objects [get_bd_addr_spaces -of_objects [get_bd_cells axi_cdma_0]] -quiet]
foreach seg $all_cdma_segs {
    delete_bd_objs $seg -quiet
}
puts "  Done"

# Disconnect axi_smc_dma
if {$smc_net ne ""} {
    puts "Disconnecting axi_smc_dma..."
    delete_bd_objs $smc_net
    puts "  Done"
}

################################################################################
# Step 3: Configure NoC - Add SI for DMA with MC connection
################################################################################

puts ""
puts "=== Configuring NoC ==="

# Calculate new SI index
set new_si_idx $num_si
set new_si_name "S[format %02d $new_si_idx]_AXI"
set new_num_si [expr {$num_si + 1}]

puts "Adding $new_si_name (NUM_SI: $num_si -> $new_num_si)"

# Update NoC: Add SI and configure its connection to MC_0
set_property -dict [list \
    CONFIG.NUM_SI $new_num_si \
] $noc

# CRITICAL: Configure the new SI's connection to MC (Memory Controller)
# This tells the NoC to route traffic from this SI to the DDR memory controller
set_property CONFIG.SI${new_si_idx}_CONNECTIONS {MC_0 {read_bw {1000} write_bw {1000}}} $noc

puts "Configured SI${new_si_idx} to route to MC_0"

################################################################################
# Step 4: Connect axi_smc_dma to new NoC SI
################################################################################

puts ""
puts "=== Connecting DMA SmartConnect to NoC ==="

connect_bd_intf_net \
    [get_bd_intf_pins axi_smc_dma/M00_AXI] \
    [get_bd_intf_pins axi_noc_0/$new_si_name]

puts "Connected axi_smc_dma/M00_AXI -> axi_noc_0/$new_si_name"

# Connect clock
set pl_clk [get_bd_pins versal_cips_0/pl0_ref_clk]
connect_bd_net $pl_clk [get_bd_pins axi_noc_0/aclk${new_si_idx}] -quiet
puts "Connected clock aclk${new_si_idx}"

################################################################################
# Step 5: Assign DDR address to DMA
################################################################################

puts ""
puts "=== Assigning DDR Addresses ==="

# Run auto address assignment - this should now assign DDR via NoC
assign_bd_address

# Verify
puts "Checking DMA address assignments:"
foreach {ip spaces} {axi_dma_0 {Data_SG Data_MM2S Data_S2MM} axi_cdma_0 {Data_SG Data}} {
    foreach space $spaces {
        set segs [get_bd_addr_segs ${ip}/${space}/* -quiet]
        set has_ddr 0
        foreach seg $segs {
            set name [get_property NAME $seg]
            if {[string match "*DDR*" $name] || [string match "*C0_*" $name]} {
                set offset [get_property OFFSET $seg]
                set range [get_property RANGE $seg]
                puts "  ${ip}/${space}: $name @ $offset ($range)"
                set has_ddr 1
            }
        }
        if {!$has_ddr} {
            puts "  ${ip}/${space}: WARNING - No DDR segment found!"
        }
    }
}

################################################################################
# Step 6: Validate and Save
################################################################################

puts ""
puts "=== Validating Design ==="

set val_errors [validate_bd_design -quiet -force]
if {$val_errors ne ""} {
    puts "Validation warnings/errors:"
    puts $val_errors
} else {
    puts "Validation passed"
}

save_bd_design
puts "Design saved"

################################################################################
# Summary
################################################################################

puts ""
puts "============================================================"
puts "  SUMMARY"
puts "============================================================"
puts "NoC NUM_SI: [get_property CONFIG.NUM_SI $noc]"

set final_net [get_bd_intf_nets -of_objects [get_bd_intf_pins axi_smc_dma/M00_AXI] -quiet]
set final_dest [get_bd_intf_pins -of_objects $final_net -filter {MODE == Slave} -quiet]
puts "axi_smc_dma/M00_AXI -> $final_dest"

puts ""
puts "Next: Run synthesis and implementation"
puts "============================================================"
