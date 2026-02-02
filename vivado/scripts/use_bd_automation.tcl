################################################################################
# Use apply_bd_automation to properly connect DMA to DDR via NoC
################################################################################

set project_dir "C:/vdma/versal_dma_benchmark"
set output_dir "C:/Users/mpcukur/claude_code_ws/versal_dma/vivado/output"

puts "============================================================"
puts "  USING BD AUTOMATION FOR DMA-DDR CONNECTION"
puts "============================================================"

# Restore from original first
cd C:/vdma
file delete -force versal_dma_benchmark
file copy -force "C:/Users/mpcukur/claude_code_ws/versal_dma/vivado/output/versal_dma_benchmark" "C:/vdma/"

open_project ${project_dir}/versal_dma_benchmark.xpr
open_bd_design [get_files dma_benchmark_bd.bd]

################################################################################
# Step 1: Check current state
################################################################################

puts ""
puts "=== Current axi_smc_dma connection ==="

set smc_net [get_bd_intf_nets -of_objects [get_bd_intf_pins axi_smc_dma/M00_AXI] -quiet]
if {$smc_net ne ""} {
    set dest [get_bd_intf_pins -of_objects $smc_net -filter {MODE == Slave} -quiet]
    puts "Connected to: $dest"

    # Disconnect
    puts "Disconnecting..."
    delete_bd_objs $smc_net
} else {
    puts "Not connected"
}

# Clear DMA address segments
puts "Clearing DMA address segments..."
foreach cell {axi_dma_0 axi_cdma_0} {
    set segs [get_bd_addr_segs -of_objects [get_bd_addr_spaces -of_objects [get_bd_cells $cell]] -quiet]
    foreach seg $segs {
        delete_bd_objs $seg -quiet
    }
}

################################################################################
# Step 2: Use apply_bd_automation
################################################################################

puts ""
puts "=== Applying BD Automation ==="

# Connect axi_smc_dma to DDR using automation
# The automation should properly configure NoC connectivity

set result [apply_bd_automation -rule xilinx.com:bd_rule:axi4 \
    -config {Clk_master {Auto} Clk_slave {Auto} Clk_xbar {Auto} Master /axi_smc_dma/M00_AXI Slave {/axi_noc_0/S00_INI} ddr_seg {Auto} intc_ip {/axi_noc_0} master_apm {0}} \
    [get_bd_intf_pins axi_smc_dma/M00_AXI] -quiet]

puts "Automation result: $result"

# If that didn't work, try connecting to the NoC directly with auto rules
if {$result eq ""} {
    puts "Trying alternative automation..."

    # Try connecting with memory segment automation
    set result2 [apply_bd_automation -rule xilinx.com:bd_rule:axi4 \
        -config {Master /axi_smc_dma/M00_AXI intc_ip {New AXI SmartConnect} Clk_xbar {Auto} Clk_master {Auto} Clk_slave {Auto} Slave {/axi_noc_0/S00_AXI} ddr_seg {Auto}} \
        [get_bd_intf_pins axi_smc_dma/M00_AXI] -quiet]
    puts "Alternative result: $result2"
}

################################################################################
# Step 3: Check what happened
################################################################################

puts ""
puts "=== Checking Result ==="

set new_net [get_bd_intf_nets -of_objects [get_bd_intf_pins axi_smc_dma/M00_AXI] -quiet]
if {$new_net ne ""} {
    set new_dest [get_bd_intf_pins -of_objects $new_net -filter {MODE == Slave} -quiet]
    puts "Now connected to: $new_dest"
} else {
    puts "Still not connected"
}

# Check NoC NUM_SI
puts "NoC NUM_SI: [get_property CONFIG.NUM_SI [get_bd_cells axi_noc_0]]"

# Check DMA address segments
puts ""
puts "DMA Address Segments:"
foreach {ip spaces} {axi_dma_0 {Data_SG Data_MM2S Data_S2MM} axi_cdma_0 {Data_SG Data}} {
    foreach space $spaces {
        set segs [get_bd_addr_segs ${ip}/${space}/* -quiet]
        if {$segs ne ""} {
            foreach seg $segs {
                set name [get_property NAME $seg]
                set offset [get_property OFFSET $seg]
                puts "  ${ip}/${space}: $name @ $offset"
            }
        } else {
            puts "  ${ip}/${space}: NO SEGMENTS"
        }
    }
}

################################################################################
# Step 4: Manual connection to existing NoC SI if automation failed
################################################################################

# If still no connection, try connecting to S00_AXI or S01_AXI
# which are already configured for DDR access

set current_net [get_bd_intf_nets -of_objects [get_bd_intf_pins axi_smc_dma/M00_AXI] -quiet]
if {$current_net eq ""} {
    puts ""
    puts "=== Trying manual connection to existing DDR path ==="

    # S01_AXI is connected from axi_smc_ctrl and has DDR access
    # We could add another input to axi_smc_ctrl and route DMA through it

    # Check what's connected to axi_smc_ctrl
    set ctrl_si [get_bd_intf_pins axi_smc_ctrl/S* -filter {MODE == Slave} -quiet]
    puts "axi_smc_ctrl slave interfaces: $ctrl_si"

    set ctrl_num_si [get_property CONFIG.NUM_SI [get_bd_cells axi_smc_ctrl]]
    puts "axi_smc_ctrl NUM_SI: $ctrl_num_si"
}

################################################################################
# Save
################################################################################

puts ""
validate_bd_design -quiet -force
save_bd_design

puts "============================================================"
