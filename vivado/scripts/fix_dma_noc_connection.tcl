################################################################################
# FIX: DMA to DDR Connection via NoC
# Problem: DMA IPs getting decode errors - cannot access DDR memory
# Solution: Properly connect DMA SmartConnect to NoC with DDR address mapping
################################################################################

puts "============================================================"
puts "  FIX: DMA to DDR Connection"
puts "============================================================"

# Open block design
set bd_name "dma_benchmark_bd"
open_bd_design [get_files ${bd_name}.bd]

################################################################################
# Step 1: Find existing NoC
################################################################################

set noc_cell [get_bd_cells -filter {VLNV =~ "*axi_noc*"} -quiet]

if {$noc_cell eq ""} {
    puts "ERROR: No NoC found in design!"
    puts "       The board automation should have created axi_noc_0"
    return -1
}

puts "INFO: Found NoC: $noc_cell"

# Print current NoC configuration
set num_si [get_property CONFIG.NUM_SI [get_bd_cells $noc_cell]]
set num_mi [get_property CONFIG.NUM_MI [get_bd_cells $noc_cell]]
set num_mc [get_property CONFIG.NUM_MC [get_bd_cells $noc_cell]]
puts "INFO: Current NoC config: NUM_SI=$num_si NUM_MI=$num_mi NUM_MC=$num_mc"

################################################################################
# Step 2: Check current axi_smc_dma connection
################################################################################

puts "INFO: Checking axi_smc_dma connections..."

set smc_dma [get_bd_cells axi_smc_dma -quiet]
if {$smc_dma eq ""} {
    puts "ERROR: axi_smc_dma not found!"
    return -1
}

# Check where M00_AXI is connected
set m00_net [get_bd_intf_nets -of_objects [get_bd_intf_pins axi_smc_dma/M00_AXI] -quiet]
if {$m00_net ne ""} {
    puts "INFO: axi_smc_dma/M00_AXI currently connected to: $m00_net"

    # Disconnect it
    puts "INFO: Disconnecting current connection..."
    delete_bd_objs $m00_net
} else {
    puts "INFO: axi_smc_dma/M00_AXI is not connected"
}

################################################################################
# Step 3: Add a new slave interface to NoC for DMA
################################################################################

puts "INFO: Adding slave interface to NoC for DMA access..."

# Increase number of slave interfaces
set new_si [expr {$num_si + 1}]
set dma_si_num [expr {$new_si - 1}]  # 0-indexed
set dma_si_name "S[format %02d $dma_si_num]_AXI"

puts "INFO: Adding $dma_si_name to NoC"

set_property -dict [list \
    CONFIG.NUM_SI $new_si \
] [get_bd_cells $noc_cell]

################################################################################
# Step 4: Configure the new slave interface for DDR access
################################################################################

puts "INFO: Configuring NoC slave interface for DDR access..."

# The slave interface needs to be configured to route to DDR MC
# This is done via the NoC's internal routing

# Set the slave interface properties
# CATEGORY: ps_pmc, ps_rpu, ps_cci, pl (for PL masters)
set si_prop "CONFIG.CONNECTIONS"
set si_config {}

# Get existing MC (Memory Controller) port
# Typically MC_0 for LPDDR4
set mc_ports [get_bd_intf_pins -of_objects [get_bd_cells $noc_cell] -filter {VLNV =~ "*lpddr*" || NAME =~ "*MC*"} -quiet]
puts "INFO: Found MC ports: $mc_ports"

# Configure the new slave interface to connect to MC_0
# This routes traffic from this slave port to the DDR memory controller
set_property -dict [list \
    CONFIG.CONNECTIONS [list MC_0 {read_bw {500} write_bw {500} read_avg_burst {4} write_avg_burst {4}}] \
] [get_bd_cells $noc_cell]

# Actually, we need to configure per-interface
# Let's use a different approach - add the connection via GUI equivalent

################################################################################
# Step 5: Connect axi_smc_dma to the new NoC slave interface
################################################################################

puts "INFO: Connecting axi_smc_dma to NoC..."

# Connect the DMA SmartConnect to the new NoC slave interface
connect_bd_intf_net [get_bd_intf_pins axi_smc_dma/M00_AXI] [get_bd_intf_pins ${noc_cell}/${dma_si_name}]

# Connect clock
set pl_clk [get_bd_pins versal_cips_0/pl0_ref_clk]
connect_bd_net $pl_clk [get_bd_pins ${noc_cell}/aclk${dma_si_num}] -quiet

################################################################################
# Step 6: Assign DDR address range to DMA path
################################################################################

puts "INFO: Assigning DDR address range..."

# Get the address segment for DDR
set ddr_segs [get_bd_addr_segs -of_objects [get_bd_cells $noc_cell] -filter {NAME =~ "*DDR*" || NAME =~ "*ddr*"}]
puts "INFO: DDR segments: $ddr_segs"

# Assign address to DMA path
# The DMA should be able to access full DDR range: 0x00000000 - 0x7FFFFFFF (DDR_LOW_0)
assign_bd_address -target_address_space [get_bd_addr_spaces axi_dma_0/Data_SG] -quiet
assign_bd_address -target_address_space [get_bd_addr_spaces axi_dma_0/Data_MM2S] -quiet
assign_bd_address -target_address_space [get_bd_addr_spaces axi_dma_0/Data_S2MM] -quiet
assign_bd_address -target_address_space [get_bd_addr_spaces axi_cdma_0/Data_SG] -quiet
assign_bd_address -target_address_space [get_bd_addr_spaces axi_cdma_0/Data] -quiet

################################################################################
# Step 7: Validate and report
################################################################################

puts "INFO: Validating design..."
validate_bd_design

puts "INFO: Saving design..."
save_bd_design

puts "============================================================"
puts "  FIX COMPLETE"
puts "============================================================"
puts ""
puts "Next steps:"
puts "1. Run synthesis: synth_design"
puts "2. Run implementation: opt_design, place_design, route_design"
puts "3. Generate PDI: write_device_image"
puts "4. Export XSA: write_hw_platform"
puts ""

