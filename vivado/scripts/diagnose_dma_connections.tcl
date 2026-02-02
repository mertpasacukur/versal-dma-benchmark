################################################################################
# DIAGNOSE: DMA to DDR Connection Analysis
# Run this in Vivado to see current connection status
################################################################################

puts ""
puts "============================================================"
puts "  DMA to DDR Connection Diagnostics"
puts "============================================================"
puts ""

# Open block design
set bd_name "dma_benchmark_bd"
open_bd_design [get_files ${bd_name}.bd] -quiet

################################################################################
# 1. Check NoC Configuration
################################################################################

puts "=== NoC Configuration ==="
set noc_cells [get_bd_cells -filter {VLNV =~ "*axi_noc*"} -quiet]
if {$noc_cells eq ""} {
    puts "  ERROR: No NoC found!"
} else {
    foreach noc $noc_cells {
        puts "  NoC: $noc"
        puts "    NUM_SI: [get_property CONFIG.NUM_SI [get_bd_cells $noc]]"
        puts "    NUM_MI: [get_property CONFIG.NUM_MI [get_bd_cells $noc]]"
        puts "    NUM_MC: [get_property CONFIG.NUM_MC [get_bd_cells $noc]]"
        puts "    NUM_MCP: [get_property CONFIG.NUM_MCP [get_bd_cells $noc]]"

        puts "  NoC Slave Interfaces:"
        set si_pins [get_bd_intf_pins -of_objects [get_bd_cells $noc] -filter {MODE == Slave} -quiet]
        foreach pin $si_pins {
            set net [get_bd_intf_nets -of_objects $pin -quiet]
            set source [get_bd_intf_pins -of_objects $net -filter {MODE == Master} -quiet]
            puts "    $pin -> connected to: $source"
        }
    }
}
puts ""

################################################################################
# 2. Check SmartConnect Connections
################################################################################

puts "=== SmartConnect Connections ==="
set smc_cells [get_bd_cells -filter {VLNV =~ "*smartconnect*"} -quiet]
foreach smc $smc_cells {
    puts "  $smc:"
    puts "    NUM_SI: [get_property CONFIG.NUM_SI [get_bd_cells $smc]]"
    puts "    NUM_MI: [get_property CONFIG.NUM_MI [get_bd_cells $smc]]"

    # Check master interfaces
    set mi_pins [get_bd_intf_pins -of_objects [get_bd_cells $smc] -filter {MODE == Master} -quiet]
    foreach pin $mi_pins {
        set net [get_bd_intf_nets -of_objects $pin -quiet]
        set dest [get_bd_intf_pins -of_objects $net -filter {MODE == Slave} -quiet]
        puts "    $pin -> $dest"
    }
}
puts ""

################################################################################
# 3. Check DMA IP Connections
################################################################################

puts "=== AXI DMA Connections ==="
set dma [get_bd_cells axi_dma_0 -quiet]
if {$dma ne ""} {
    puts "  Base address: [get_property CONFIG.C_BASEADDR [get_bd_cells $dma] -quiet]"
    puts "  Data width: [get_property CONFIG.c_m_axi_mm2s_data_width [get_bd_cells $dma]]"
    puts "  Addr width: [get_property CONFIG.c_addr_width [get_bd_cells $dma]]"
    puts "  Include SG: [get_property CONFIG.c_include_sg [get_bd_cells $dma]]"

    set dma_pins [get_bd_intf_pins -of_objects [get_bd_cells $dma] -filter {VLNV =~ "*aximm*"} -quiet]
    foreach pin $dma_pins {
        set net [get_bd_intf_nets -of_objects $pin -quiet]
        set dest [get_bd_intf_pins -of_objects $net -filter {MODE == Slave} -quiet]
        puts "  $pin -> $dest"
    }
}
puts ""

puts "=== AXI CDMA Connections ==="
set cdma [get_bd_cells axi_cdma_0 -quiet]
if {$cdma ne ""} {
    puts "  Data width: [get_property CONFIG.C_M_AXI_DATA_WIDTH [get_bd_cells $cdma]]"
    puts "  Addr width: [get_property CONFIG.C_ADDR_WIDTH [get_bd_cells $cdma]]"
    puts "  Include SG: [get_property CONFIG.C_INCLUDE_SG [get_bd_cells $cdma]]"

    set cdma_pins [get_bd_intf_pins -of_objects [get_bd_cells $cdma] -filter {VLNV =~ "*aximm*"} -quiet]
    foreach pin $cdma_pins {
        set net [get_bd_intf_nets -of_objects $pin -quiet]
        set dest [get_bd_intf_pins -of_objects $net -filter {MODE == Slave} -quiet]
        puts "  $pin -> $dest"
    }
}
puts ""

################################################################################
# 4. Check CIPS Configuration
################################################################################

puts "=== CIPS Configuration ==="
set cips [get_bd_cells versal_cips_0 -quiet]
if {$cips ne ""} {
    # Check S_AXI_FPD
    set s_axi_fpd [get_bd_intf_pins versal_cips_0/S_AXI_FPD -quiet]
    if {$s_axi_fpd ne ""} {
        set net [get_bd_intf_nets -of_objects $s_axi_fpd -quiet]
        set source [get_bd_intf_pins -of_objects $net -filter {MODE == Master} -quiet]
        puts "  S_AXI_FPD <- $source"
    } else {
        puts "  S_AXI_FPD: NOT ENABLED!"
    }

    # Check M_AXI_FPD
    set m_axi_fpd [get_bd_intf_pins versal_cips_0/M_AXI_FPD -quiet]
    if {$m_axi_fpd ne ""} {
        set net [get_bd_intf_nets -of_objects $m_axi_fpd -quiet]
        set dest [get_bd_intf_pins -of_objects $net -filter {MODE == Slave} -quiet]
        puts "  M_AXI_FPD -> $dest"
    }
}
puts ""

################################################################################
# 5. Check Address Map
################################################################################

puts "=== Address Map for DMA ==="

# Check address segments assigned to DMA
set dma_spaces [get_bd_addr_spaces -of_objects [get_bd_cells axi_dma_0] -quiet]
foreach space $dma_spaces {
    puts "  Address space: $space"
    set segs [get_bd_addr_segs -of_objects $space -quiet]
    foreach seg $segs {
        set offset [get_property OFFSET $seg]
        set range [get_property RANGE $seg]
        puts "    $seg: offset=$offset range=$range"
    }
}

set cdma_spaces [get_bd_addr_spaces -of_objects [get_bd_cells axi_cdma_0] -quiet]
foreach space $cdma_spaces {
    puts "  Address space: $space"
    set segs [get_bd_addr_segs -of_objects $space -quiet]
    foreach seg $segs {
        set offset [get_property OFFSET $seg]
        set range [get_property RANGE $seg]
        puts "    $seg: offset=$offset range=$range"
    }
}
puts ""

################################################################################
# 6. Summary and Recommendations
################################################################################

puts "=== Summary ==="
puts ""
puts "If DMA is getting DECODE ERRORS, check:"
puts "1. Is axi_smc_dma connected to NoC or S_AXI_FPD?"
puts "2. Does the DMA have DDR address segments assigned?"
puts "3. Is S_AXI_FPD enabled in CIPS configuration?"
puts ""
puts "For NoC-based DDR access, the path should be:"
puts "  DMA -> axi_smc_dma -> NoC Slave Port -> DDR MC"
puts ""
puts "For S_AXI_FPD-based access, the path should be:"
puts "  DMA -> axi_smc_dma -> S_AXI_FPD -> PS DDR Controller"
puts ""
puts "============================================================"
