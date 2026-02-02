################################################################################
# FIX: Configure NoC S06_AXI connectivity to DDR
# Copy the configuration pattern from S00_AXI/S01_AXI
################################################################################

set project_dir "C:/vdma/versal_dma_benchmark"
set output_dir "C:/Users/mpcukur/claude_code_ws/versal_dma/vivado/output"

puts "============================================================"
puts "  CONFIGURING NoC S06_AXI DDR CONNECTIVITY"
puts "============================================================"

open_project ${project_dir}/versal_dma_benchmark.xpr
open_bd_design [get_files dma_benchmark_bd.bd]

set noc [get_bd_cells axi_noc_0]

################################################################################
# The NoC connectivity needs to be configured through the CONNECTIONS property
# S00 uses C3_DDR, S01 uses C2_DDR
# We need to configure S06 to use one of the MC connections
################################################################################

puts ""
puts "=== Configuring NoC Connectivity ==="

# Get current NUM_SI
set num_si [get_property CONFIG.NUM_SI $noc]
puts "Current NUM_SI: $num_si"

# For S06_AXI, we need to add it to the NoC's connectivity
# The key is to use the proper NoC configuration format

# First, let's try setting the MC_DESTINATIONS for S06
# This is the proper way to configure which MC a given SI can access

# Configure S06 to route to MC_0 (Memory Controller 0)
# Format: {MC_0 {read_bw {500} write_bw {500}}}

puts "Setting S06 connections to MC..."

# Method 1: Try via ASSOCIATED_BUSIF (often used for NoC)
set_property -dict [list \
    CONFIG.CONNECTIONS {MC_0 {read_bw {5000} write_bw {5000} read_avg_burst {4} write_avg_burst {4}}} \
] [get_bd_intf_pins axi_noc_0/S06_AXI] -quiet

# Method 2: Try via NoC cell properties
set_property CONFIG.CH0_LPDDR4_0_BOARD_INTERFACE {ch0_lpddr4_trip1} $noc -quiet
set_property CONFIG.CH1_LPDDR4_0_BOARD_INTERFACE {ch1_lpddr4_trip1} $noc -quiet

# Method 3: Configure through NoC Connectivity using SI6 pattern
# The NoC uses an internal NMU/NSU structure - we need proper configuration

# Check what we can set
puts "Checking available SI6 properties..."
set si6_props [list_property [get_bd_intf_pins axi_noc_0/S06_AXI]]
foreach prop $si6_props {
    if {[string match "*CONN*" $prop] || [string match "*MC*" $prop] || [string match "*MEM*" $prop]} {
        puts "  $prop = [get_property $prop [get_bd_intf_pins axi_noc_0/S06_AXI] -quiet]"
    }
}

################################################################################
# Alternative: Reconfigure entire NoC with proper connectivity
################################################################################

puts ""
puts "=== Reconfiguring NoC with complete connectivity ==="

# The best way is to reconfigure NUM_SI with proper MC connection patterns
# Let's use the apply_bd_automation approach which handles this automatically

# First delete S06_AXI connection
set net [get_bd_intf_nets -of_objects [get_bd_intf_pins axi_smc_dma/M00_AXI] -quiet]
if {$net ne ""} {
    delete_bd_objs $net
}

# Reduce NUM_SI back and reconfigure properly
set_property CONFIG.NUM_SI 6 $noc

# Now add S06 with proper MC connectivity using the right format
# The key property is CONNECTIONS for the SI

# Set the NoC configuration to include the DMA path
set_property -dict [list \
    CONFIG.NUM_SI {7} \
] $noc

# Now we need to define the connection from S06 to MC0
# Try using the connectivity configuration property
set_property -dict [list \
    CONFIG.SI6_CONNECTIONS {MC_0 {read_bw {5000} write_bw {5000} read_avg_burst {4} write_avg_burst {4}}} \
] $noc -quiet

# Reconnect DMA
connect_bd_intf_net [get_bd_intf_pins axi_smc_dma/M00_AXI] [get_bd_intf_pins axi_noc_0/S06_AXI]

# Connect clock
set pl_clk [get_bd_pins versal_cips_0/pl0_ref_clk]
connect_bd_net $pl_clk [get_bd_pins axi_noc_0/aclk6] -quiet

################################################################################
# Check result
################################################################################

puts ""
puts "=== Checking Configuration ==="

set s06_segs [get_bd_addr_segs axi_noc_0/S06_AXI/* -quiet]
puts "S06_AXI segments after config: $s06_segs"

# Try assign address
assign_bd_address -quiet

# Check DMA segments again
foreach {ip spaces} {axi_dma_0 {Data_SG Data_MM2S Data_S2MM} axi_cdma_0 {Data_SG Data}} {
    foreach space $spaces {
        set segs [get_bd_addr_segs ${ip}/${space}/* -quiet]
        if {$segs ne ""} {
            foreach seg $segs {
                set name [get_property NAME $seg]
                if {[string match "*DDR*" $name] || [string match "*C*_*" $name]} {
                    set offset [get_property OFFSET $seg]
                    puts "  ${ip}/${space}: $name @ $offset"
                }
            }
        }
    }
}

################################################################################
# Validate
################################################################################

puts ""
validate_bd_design -quiet -force
save_bd_design

puts ""
puts "============================================================"
puts "Configuration saved. Check if DDR segments are now available."
puts "============================================================"
