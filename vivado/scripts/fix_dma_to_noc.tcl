################################################################################
# CORRECT FIX: Connect DMA to NoC for DDR access
# Problem: DMA connected to S_AXI_FPD which doesn't provide DDR access
# Solution: Connect axi_smc_dma to NoC with proper DDR address mapping
################################################################################

set project_dir "C:/vdma/versal_dma_benchmark"
set output_dir "C:/Users/mpcukur/claude_code_ws/versal_dma/vivado/output"

puts "============================================================"
puts "  CORRECT FIX: DMA to NoC for DDR Access"
puts "============================================================"

open_project ${project_dir}/versal_dma_benchmark.xpr
open_bd_design [get_files dma_benchmark_bd.bd]

################################################################################
# Step 1: Disconnect axi_smc_dma from S_AXI_FPD
################################################################################

puts "INFO: Disconnecting axi_smc_dma from S_AXI_FPD..."

set smc_m00 [get_bd_intf_pins axi_smc_dma/M00_AXI]
set current_net [get_bd_intf_nets -of_objects $smc_m00 -quiet]

if {$current_net ne ""} {
    puts "  Removing connection: $current_net"
    delete_bd_objs $current_net
}

################################################################################
# Step 2: Configure NoC to add slave interface for DMA
################################################################################

puts "INFO: Configuring NoC for DMA access..."

set noc [get_bd_cells axi_noc_0]
set current_si [get_property CONFIG.NUM_SI $noc]
puts "  Current NUM_SI: $current_si"

# Add new slave interface for DMA
set new_si [expr {$current_si + 1}]
set dma_si_idx [expr {$new_si - 1}]
set dma_si_name "S[format %02d $dma_si_idx]_AXI"

puts "  Adding $dma_si_name to NoC (new NUM_SI: $new_si)"

# Get current NoC configuration
set current_connections [get_property CONFIG.CONNECTIONS $noc]
puts "  Current CONNECTIONS: $current_connections"

# Configure NoC with new SI and connection to MC
set_property -dict [list \
    CONFIG.NUM_SI $new_si \
] $noc

# Configure the new slave interface to connect to Memory Controller
# SI[n]_CONNECTIONS format: { MC_0 { read_bw {500} write_bw {500} } }
set si_conn_prop "CONFIG.SI[${dma_si_idx}]_CONNECTIONS"

# Set connection from new SI to MC_0
set_property CONFIG.SI[${dma_si_idx}]_CONNECTIONS {MC_0 { read_bw {1000} write_bw {1000} read_avg_burst {4} write_avg_burst {4}}} $noc

# Set physical location strategy for new SI
set_property CONFIG.SI[${dma_si_idx}]_R_TRAFFIC {BEST_EFFORT} $noc
set_property CONFIG.SI[${dma_si_idx}]_W_TRAFFIC {BEST_EFFORT} $noc

################################################################################
# Step 3: Connect axi_smc_dma to NoC
################################################################################

puts "INFO: Connecting axi_smc_dma to NoC/$dma_si_name..."

# Connect the interface
connect_bd_intf_net [get_bd_intf_pins axi_smc_dma/M00_AXI] [get_bd_intf_pins ${noc}/${dma_si_name}]

# Connect clock for the new SI
set pl_clk [get_bd_pins versal_cips_0/pl0_ref_clk]
set noc_clk_pin [get_bd_pins ${noc}/aclk${dma_si_idx}]
if {$noc_clk_pin ne ""} {
    connect_bd_net $pl_clk $noc_clk_pin
    puts "  Connected clock to aclk${dma_si_idx}"
}

################################################################################
# Step 4: Assign DDR addresses to DMA
################################################################################

puts "INFO: Assigning DDR addresses to DMA..."

# First, exclude any unwanted segments
# Include all available segments first
set excluded [get_bd_addr_segs -excluded -quiet]
if {$excluded ne ""} {
    puts "  Including previously excluded segments..."
    include_bd_addr_seg $excluded -quiet
}

# Auto-assign addresses
assign_bd_address -quiet

# Verify DDR segments are assigned
puts "INFO: Verifying DDR address assignments..."

# Check each DMA address space
foreach {ip spaces} {axi_dma_0 {Data_SG Data_MM2S Data_S2MM} axi_cdma_0 {Data_SG Data}} {
    foreach space $spaces {
        set addr_space [get_bd_addr_spaces ${ip}/${space} -quiet]
        if {$addr_space ne ""} {
            set segs [get_bd_addr_segs -of_objects $addr_space -quiet]
            set has_ddr 0
            foreach seg $segs {
                set seg_name [get_property NAME $seg]
                if {[string match "*lpddr*" $seg_name] || [string match "*DDR*" $seg_name] || [string match "*MC*" $seg_name]} {
                    set offset [get_property OFFSET $seg]
                    set range [get_property RANGE $seg]
                    puts "  ${ip}/${space}: $seg_name offset=$offset range=$range"
                    set has_ddr 1
                }
            }
            if {!$has_ddr} {
                puts "  WARNING: ${ip}/${space} has no DDR segment!"
            }
        }
    }
}

################################################################################
# Step 5: Validate and Save
################################################################################

puts "INFO: Validating block design..."
set val_result [validate_bd_design -quiet]
if {$val_result ne ""} {
    puts "  Validation issues: $val_result"
}

puts "INFO: Saving block design..."
save_bd_design

puts "INFO: Regenerating output products..."
generate_target all [get_files dma_benchmark_bd.bd]

################################################################################
# Step 6: Print summary
################################################################################

puts ""
puts "============================================================"
puts "  CONFIGURATION SUMMARY"
puts "============================================================"

# Print NoC configuration
puts "NoC Configuration:"
puts "  NUM_SI: [get_property CONFIG.NUM_SI $noc]"
puts "  NUM_MI: [get_property CONFIG.NUM_MI $noc]"
puts "  NUM_MC: [get_property CONFIG.NUM_MC $noc]"

# Print axi_smc_dma connection
set smc_net [get_bd_intf_nets -of_objects [get_bd_intf_pins axi_smc_dma/M00_AXI] -quiet]
set dest [get_bd_intf_pins -of_objects $smc_net -filter {MODE == Slave} -quiet]
puts "  axi_smc_dma/M00_AXI -> $dest"

puts ""
puts "============================================================"
puts "  Next: Run synthesis and implementation"
puts "============================================================"
