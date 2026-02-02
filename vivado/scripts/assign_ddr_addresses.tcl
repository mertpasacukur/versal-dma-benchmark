################################################################################
# Manually assign DDR addresses to DMA through NoC
################################################################################

set project_dir "C:/vdma/versal_dma_benchmark"

puts "============================================================"
puts "  ASSIGNING DDR ADDRESSES TO DMA"
puts "============================================================"

open_project ${project_dir}/versal_dma_benchmark.xpr
open_bd_design [get_files dma_benchmark_bd.bd]

################################################################################
# Step 1: List all available address segments on NoC
################################################################################

puts ""
puts "=== Available NoC Address Segments ==="

set noc [get_bd_cells axi_noc_0]

# Get all address segs exposed by NoC
set noc_addr_segs [get_bd_addr_segs -of_objects $noc -quiet]
puts "NoC address segments:"
foreach seg $noc_addr_segs {
    puts "  $seg"
}

# Check specific SI segments
puts ""
puts "Checking S06_AXI (DMA port) segments:"
set s06_segs [get_bd_addr_segs axi_noc_0/S06_AXI/* -quiet]
foreach seg $s06_segs {
    puts "  $seg"
}

################################################################################
# Step 2: Find DDR/LPDDR segment
################################################################################

puts ""
puts "=== Looking for DDR Segment ==="

# The DDR segment should be something like C0_DDR_LOW0 or similar
# Let's search for it in different ways

# Method 1: Look for segments with DDR in name
set ddr_segs [get_bd_addr_segs -filter {NAME =~ "*DDR*" || NAME =~ "*ddr*"} -quiet]
puts "Segments with DDR in name: $ddr_segs"

# Method 2: Look for memory controller segments
set mc_segs [get_bd_addr_segs -filter {NAME =~ "*C0*" || NAME =~ "*MC*"} -quiet]
puts "Segments with C0/MC in name: $mc_segs"

# Method 3: Check what's connected to NoC MC port
puts ""
puts "NoC configuration:"
puts "  NUM_MC: [get_property CONFIG.NUM_MC $noc]"
puts "  NUM_MCP: [get_property CONFIG.NUM_MCP $noc]"

# Try to get MC info
set mc_type [get_property CONFIG.MC_TYPE $noc -quiet]
puts "  MC_TYPE: $mc_type"

################################################################################
# Step 3: Manual DDR address assignment
################################################################################

puts ""
puts "=== Manual Address Assignment ==="

# The LPDDR4 base address on VPK120 is 0x00000000
# Let's try to create the address mapping manually

# First, let's see what address spaces we're working with
puts "DMA Address Spaces:"
puts "  AXI DMA: [get_bd_addr_spaces -of_objects [get_bd_cells axi_dma_0]]"
puts "  AXI CDMA: [get_bd_addr_spaces -of_objects [get_bd_cells axi_cdma_0]]"

# Try to find the target segment for S06_AXI
set target_seg [get_bd_addr_segs axi_noc_0/S06_AXI/C0_DDR_LOW0 -quiet]
if {$target_seg eq ""} {
    set target_seg [get_bd_addr_segs -of_objects [get_bd_intf_pins axi_noc_0/S06_AXI] -quiet]
}
puts "Target segment for S06_AXI: $target_seg"

# If we found a target, assign it to DMA
if {$target_seg ne ""} {
    foreach space {/axi_dma_0/Data_SG /axi_dma_0/Data_MM2S /axi_dma_0/Data_S2MM /axi_cdma_0/Data_SG /axi_cdma_0/Data} {
        set result [assign_bd_address -target_address_space [get_bd_addr_spaces $space] $target_seg -quiet]
        puts "  Assigned to $space: $result"
    }
}

################################################################################
# Step 4: Alternative - Check if NoC needs HBM/DDR config
################################################################################

puts ""
puts "=== NoC Memory Configuration ==="

# Check NoC memory interface properties
set mc_ports [get_bd_intf_pins -of_objects $noc -filter {VLNV =~ "*lpddr*" || NAME =~ "*CH*"} -quiet]
puts "Memory ports: $mc_ports"

# Check if there's a CH0_LPDDR4 or similar
set lpddr_pins [get_bd_intf_pins -of_objects $noc -filter {NAME =~ "*LPDDR*" || NAME =~ "*lpddr*"} -quiet]
puts "LPDDR pins: $lpddr_pins"

################################################################################
# Step 5: Final verification
################################################################################

puts ""
puts "=== Final Address Map ==="

foreach {ip spaces} {axi_dma_0 {Data_SG Data_MM2S Data_S2MM} axi_cdma_0 {Data_SG Data}} {
    foreach space $spaces {
        set segs [get_bd_addr_segs ${ip}/${space}/* -quiet]
        if {$segs ne ""} {
            foreach seg $segs {
                set offset [get_property OFFSET $seg -quiet]
                set range [get_property RANGE $seg -quiet]
                puts "  ${ip}/${space}: [get_property NAME $seg] @ $offset ($range)"
            }
        } else {
            puts "  ${ip}/${space}: NO SEGMENTS"
        }
    }
}

save_bd_design

puts ""
puts "============================================================"
