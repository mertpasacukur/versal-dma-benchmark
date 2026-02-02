################################################################################
# Check NoC internal configuration to understand how to add DMA port
################################################################################

set project_dir "C:/vdma/versal_dma_benchmark"

puts "============================================================"
puts "  NOC CONFIGURATION ANALYSIS"
puts "============================================================"

open_project ${project_dir}/versal_dma_benchmark.xpr
open_bd_design [get_files dma_benchmark_bd.bd]

set noc [get_bd_cells axi_noc_0]

puts ""
puts "=== Basic NoC Config ==="
foreach prop {NUM_SI NUM_MI NUM_MC NUM_MCP NUM_NSU NUM_NMU NUM_CLKS} {
    set val [get_property CONFIG.${prop} $noc -quiet]
    puts "  $prop: $val"
}

puts ""
puts "=== SI Configurations ==="
for {set i 0} {$i < 7} {incr i} {
    puts "SI${i}:"
    foreach prop {CONNECTIONS R_TRAFFIC W_TRAFFIC} {
        set val [get_property CONFIG.SI${i}_${prop} $noc -quiet]
        if {$val ne ""} {
            puts "  ${prop}: $val"
        }
    }
}

puts ""
puts "=== MC Configuration ==="
foreach prop {MC_ADDR_WIDTH MC_BURST_LENGTH MC_DATA_WIDTH MC_INTERLEAVE_SIZE} {
    set val [get_property CONFIG.${prop} $noc -quiet]
    puts "  $prop: $val"
}

puts ""
puts "=== MC Port Configs ==="
for {set i 0} {$i < 2} {incr i} {
    puts "MC${i}:"
    foreach prop {CONNECTIONS} {
        set val [get_property CONFIG.MC${i}_${prop} $noc -quiet]
        if {$val ne ""} {
            puts "  ${prop}: $val"
        }
    }
}

puts ""
puts "=== All CONFIG properties containing CONNECTION ==="
set all_props [list_property $noc]
foreach prop $all_props {
    if {[string match "*CONNECTION*" $prop] || [string match "*ROUTE*" $prop]} {
        set val [get_property $prop $noc -quiet]
        if {$val ne ""} {
            puts "  $prop: $val"
        }
    }
}

puts ""
puts "=== Checking Address Segments ==="
# Check S00_AXI segments (this should work if PS can access LPDDR)
set s00_segs [get_bd_addr_segs axi_noc_0/S00_AXI/* -quiet]
puts "S00_AXI segments: $s00_segs"

set s01_segs [get_bd_addr_segs axi_noc_0/S01_AXI/* -quiet]
puts "S01_AXI segments: $s01_segs"

set s06_segs [get_bd_addr_segs axi_noc_0/S06_AXI/* -quiet]
puts "S06_AXI segments: $s06_segs"

puts ""
puts "============================================================"
