################################################################################
# Versal DMA Benchmark - NoC Configuration
# Target: VPK120 (Versal Premium VP1202)
# Vivado 2023.2 - Board automation creates DDR NoC
################################################################################

puts "INFO: Configuring NoC for DMA access..."

# Board automation created axi_noc_0 for DDR
# Get the existing NoC created by board automation
set noc_cell [get_bd_cells -filter {VLNV =~ "*axi_noc*"} -quiet]

if {$noc_cell ne ""} {
    puts "INFO: Found NoC: $noc_cell"

    # Get current number of slave interfaces
    set current_si [get_property CONFIG.NUM_SI [get_bd_cells $noc_cell]]
    puts "INFO: NoC has $current_si slave interfaces"

    # Add more slave interfaces for DMA engines (need 4 more: DMA MM2S, S2MM, CDMA, MCDMA)
    set new_si [expr {$current_si + 4}]

    set_property -dict [list \
        CONFIG.NUM_SI $new_si \
    ] [get_bd_cells $noc_cell]

    puts "INFO: NoC updated to $new_si slave interfaces for DMA access"
} else {
    puts "WARNING: NoC not found - creating SmartConnect for memory access"
}

################################################################################
# AXI SmartConnect for PL Peripherals (Control path)
################################################################################

puts "INFO: Creating SmartConnect for PL peripherals..."

create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:1.0 axi_smc_pl

set_property -dict [list \
    CONFIG.NUM_SI {1} \
    CONFIG.NUM_MI {5} \
    CONFIG.NUM_CLKS {1} \
] [get_bd_cells axi_smc_pl]

# Connect CIPS M_AXI_FPD to SmartConnect
connect_bd_intf_net [get_bd_intf_pins versal_cips_0/M_AXI_FPD] [get_bd_intf_pins axi_smc_pl/S00_AXI]

# Connect clock to SmartConnect
connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins axi_smc_pl/aclk]

# Connect clocks to CIPS AXI interfaces
connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins versal_cips_0/m_axi_fpd_aclk]
connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins versal_cips_0/s_axi_fpd_aclk]

################################################################################
# AXI SmartConnect for DMA Data Path
################################################################################

puts "INFO: Creating SmartConnect for DMA data path..."

create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:1.0 axi_smc_dma

set_property -dict [list \
    CONFIG.NUM_SI {4} \
    CONFIG.NUM_MI {1} \
    CONFIG.NUM_CLKS {1} \
] [get_bd_cells axi_smc_dma]

connect_bd_net [get_bd_pins versal_cips_0/pl0_ref_clk] [get_bd_pins axi_smc_dma/aclk]

# Try to connect to NoC for DDR access, otherwise use S_AXI_FPD
if {$noc_cell ne ""} {
    # Get available slave interface on NoC
    set noc_si_pins [get_bd_intf_pins -of_objects [get_bd_cells $noc_cell] -filter {MODE == Slave && VLNV =~ "*axi*"} -quiet]
    set connected_pins [get_bd_intf_nets -of_objects $noc_si_pins -quiet]

    # Find first unconnected slave interface
    foreach pin $noc_si_pins {
        set net [get_bd_intf_nets -of_objects $pin -quiet]
        if {$net eq ""} {
            puts "INFO: Connecting DMA SmartConnect to NoC via $pin"
            connect_bd_intf_net [get_bd_intf_pins axi_smc_dma/M00_AXI] $pin
            break
        }
    }
} else {
    # Fallback: Connect to CIPS S_AXI_FPD
    puts "INFO: Connecting DMA SmartConnect to CIPS S_AXI_FPD"
    connect_bd_intf_net [get_bd_intf_pins axi_smc_dma/M00_AXI] [get_bd_intf_pins versal_cips_0/S_AXI_FPD]
}

puts "INFO: NoC/SmartConnect configuration complete"
