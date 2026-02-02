################################################################################
# Versal DMA Benchmark - Block Design Creation (Main Script)
# Target: VPK120 (Versal Premium VP1202)
# Vivado 2023.2 Compatible
################################################################################

set project_dir [file normalize [file dirname [info script]]/../..]
set bd_name "dma_benchmark_bd"

# Create block design
puts "INFO: Creating block design: ${bd_name}"
create_bd_design ${bd_name}

################################################################################
# Source configuration scripts in order
################################################################################

puts "INFO: Configuring CIPS..."
source ${project_dir}/vivado/scripts/configure_cips.tcl

puts "INFO: Configuring NoC/SmartConnect..."
source ${project_dir}/vivado/scripts/configure_noc.tcl

puts "INFO: Adding DMA IPs..."
source ${project_dir}/vivado/scripts/add_dma_ips.tcl

puts "INFO: Adding Memory IPs..."
source ${project_dir}/vivado/scripts/add_memory_ips.tcl

################################################################################
# Create Processor System Reset
################################################################################

puts "INFO: Creating processor system reset..."

create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 proc_sys_reset_0

set pl_clk [get_bd_pins versal_cips_0/pl0_ref_clk]
set pl_resetn [get_bd_pins versal_cips_0/pl0_resetn]

connect_bd_net ${pl_clk} [get_bd_pins proc_sys_reset_0/slowest_sync_clk]
connect_bd_net ${pl_resetn} [get_bd_pins proc_sys_reset_0/ext_reset_in]

# Connect constant for aux_reset and dcm_locked
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 const_vcc
set_property CONFIG.CONST_VAL {1} [get_bd_cells const_vcc]
connect_bd_net [get_bd_pins const_vcc/dout] [get_bd_pins proc_sys_reset_0/aux_reset_in]
connect_bd_net [get_bd_pins const_vcc/dout] [get_bd_pins proc_sys_reset_0/dcm_locked]

################################################################################
# Connect Resets to all IPs
################################################################################

puts "INFO: Connecting resets..."

set periph_resetn [get_bd_pins proc_sys_reset_0/peripheral_aresetn]

# SmartConnects
connect_bd_net ${periph_resetn} [get_bd_pins axi_smc_pl/aresetn]
connect_bd_net ${periph_resetn} [get_bd_pins axi_smc_dma/aresetn]
connect_bd_net ${periph_resetn} [get_bd_pins axi_smc_cdma/aresetn]
connect_bd_net ${periph_resetn} [get_bd_pins axi_smc_mcdma/aresetn]

# DMAs
connect_bd_net ${periph_resetn} [get_bd_pins axi_dma_0/axi_resetn]
connect_bd_net ${periph_resetn} [get_bd_pins axi_cdma_0/s_axi_lite_aresetn]
connect_bd_net ${periph_resetn} [get_bd_pins axi_mcdma_0/axi_resetn]

# Stream FIFOs
connect_bd_net ${periph_resetn} [get_bd_pins axis_data_fifo_0/s_axis_aresetn]
connect_bd_net ${periph_resetn} [get_bd_pins axis_data_fifo_mcdma/s_axis_aresetn]

# BRAM Controller
connect_bd_net ${periph_resetn} [get_bd_pins axi_bram_ctrl_0/s_axi_aresetn]

################################################################################
# Connect Interrupts
################################################################################

puts "INFO: Connecting interrupts..."

# Create concat for interrupts
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 xlconcat_irq
set_property CONFIG.NUM_PORTS {5} [get_bd_cells xlconcat_irq]

# Connect DMA interrupts
connect_bd_net [get_bd_pins axi_dma_0/mm2s_introut] [get_bd_pins xlconcat_irq/In0]
connect_bd_net [get_bd_pins axi_dma_0/s2mm_introut] [get_bd_pins xlconcat_irq/In1]
connect_bd_net [get_bd_pins axi_cdma_0/cdma_introut] [get_bd_pins xlconcat_irq/In2]
connect_bd_net [get_bd_pins axi_mcdma_0/mm2s_ch1_introut] [get_bd_pins xlconcat_irq/In3]
connect_bd_net [get_bd_pins axi_mcdma_0/s2mm_ch1_introut] [get_bd_pins xlconcat_irq/In4]

# Connect to CIPS pl_ps_irq
connect_bd_net [get_bd_pins xlconcat_irq/dout] [get_bd_pins versal_cips_0/pl_ps_irq0]

################################################################################
# Address Assignment
################################################################################

puts "INFO: Assigning addresses..."

# Assign addresses automatically
assign_bd_address

################################################################################
# Validation and Save
################################################################################

puts "INFO: Validating block design..."
validate_bd_design

# Save block design
save_bd_design

# Generate HDL wrapper
puts "INFO: Generating HDL wrapper..."
make_wrapper -files [get_files ${bd_name}.bd] -top

# Add wrapper to project
set wrapper_path [file normalize "[get_property DIRECTORY [current_project]]/versal_dma_benchmark.gen/sources_1/bd/${bd_name}/hdl/${bd_name}_wrapper.v"]
add_files -norecurse ${wrapper_path}
set_property top ${bd_name}_wrapper [current_fileset]
update_compile_order -fileset sources_1

puts "INFO: Block design creation complete"
