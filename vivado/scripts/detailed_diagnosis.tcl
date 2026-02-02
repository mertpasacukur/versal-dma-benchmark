################################################################################
# DETAILED DIAGNOSIS: Check actual connections and address mappings
################################################################################

set project_dir "C:/vdma/versal_dma_benchmark"

puts "============================================================"
puts "  DETAILED DMA CONNECTION DIAGNOSIS"
puts "============================================================"

open_project ${project_dir}/versal_dma_benchmark.xpr
open_bd_design [get_files dma_benchmark_bd.bd]

puts ""
puts "=== 1. CIPS S_AXI_FPD Status ==="
set s_axi_fpd [get_bd_intf_pins versal_cips_0/S_AXI_FPD -quiet]
if {$s_axi_fpd eq ""} {
    puts "  ERROR: S_AXI_FPD is NOT ENABLED in CIPS!"
} else {
    puts "  S_AXI_FPD exists: $s_axi_fpd"
    set net [get_bd_intf_nets -of_objects $s_axi_fpd -quiet]
    if {$net ne ""} {
        set source [get_bd_intf_pins -of_objects $net -filter {MODE == Master} -quiet]
        puts "  Connected from: $source"
    } else {
        puts "  WARNING: S_AXI_FPD is NOT CONNECTED!"
    }
}

puts ""
puts "=== 2. axi_smc_dma M00_AXI Connection ==="
set smc_m00 [get_bd_intf_pins axi_smc_dma/M00_AXI -quiet]
if {$smc_m00 ne ""} {
    set net [get_bd_intf_nets -of_objects $smc_m00 -quiet]
    if {$net ne ""} {
        set dest [get_bd_intf_pins -of_objects $net -filter {MODE == Slave} -quiet]
        puts "  axi_smc_dma/M00_AXI -> $dest"
    } else {
        puts "  ERROR: axi_smc_dma/M00_AXI is NOT CONNECTED!"
    }
}

puts ""
puts "=== 3. NoC Slave Interfaces ==="
set noc [get_bd_cells axi_noc_0 -quiet]
if {$noc ne ""} {
    set num_si [get_property CONFIG.NUM_SI $noc]
    set num_mc [get_property CONFIG.NUM_MC $noc]
    puts "  NoC NUM_SI: $num_si"
    puts "  NoC NUM_MC: $num_mc"

    set si_pins [get_bd_intf_pins -of_objects $noc -filter {MODE == Slave && VLNV =~ "*aximm*"} -quiet]
    foreach pin $si_pins {
        set net [get_bd_intf_nets -of_objects $pin -quiet]
        set source [get_bd_intf_pins -of_objects $net -filter {MODE == Master} -quiet]
        puts "  $pin <- $source"
    }
}

puts ""
puts "=== 4. DMA Address Segments ==="

puts "  AXI DMA address spaces:"
foreach space {Data_SG Data_MM2S Data_S2MM} {
    set addr_space [get_bd_addr_spaces axi_dma_0/$space -quiet]
    if {$addr_space ne ""} {
        set segs [get_bd_addr_segs -of_objects $addr_space -quiet]
        if {$segs eq ""} {
            puts "    $space: NO SEGMENTS ASSIGNED!"
        } else {
            foreach seg $segs {
                set offset [get_property OFFSET $seg -quiet]
                set range [get_property RANGE $seg -quiet]
                puts "    $space: $seg offset=$offset range=$range"
            }
        }
    }
}

puts ""
puts "  AXI CDMA address spaces:"
foreach space {Data_SG Data} {
    set addr_space [get_bd_addr_spaces axi_cdma_0/$space -quiet]
    if {$addr_space ne ""} {
        set segs [get_bd_addr_segs -of_objects $addr_space -quiet]
        if {$segs eq ""} {
            puts "    $space: NO SEGMENTS ASSIGNED!"
        } else {
            foreach seg $segs {
                set offset [get_property OFFSET $seg -quiet]
                set range [get_property RANGE $seg -quiet]
                puts "    $space: $seg offset=$offset range=$range"
            }
        }
    }
}

puts ""
puts "=== 5. All Address Segments in Design ==="
report_bd_addr_segments

puts ""
puts "=== 6. Excluded Address Segments ==="
set excluded [get_bd_addr_segs -excluded -quiet]
if {$excluded ne ""} {
    puts "  EXCLUDED segments found:"
    foreach seg $excluded {
        puts "    $seg"
    }
} else {
    puts "  No excluded segments"
}

puts ""
puts "============================================================"
puts "  DIAGNOSIS COMPLETE"
puts "============================================================"
