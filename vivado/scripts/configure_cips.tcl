################################################################################
# Versal DMA Benchmark - CIPS Configuration with DDR4 and UART
# Target: VPK120 (Versal Premium VP1202)
# Vivado 2023.2 - DDR4 enabled for software execution
################################################################################

puts "INFO: Adding and configuring CIPS with DDR4 and UART..."

# Add CIPS IP
create_bd_cell -type ip -vlnv xilinx.com:ip:versal_cips:3.4 versal_cips_0

# Apply board preset for VPK120 which includes DDR4 configuration
apply_bd_automation -rule xilinx.com:bd_rule:versal_cips -config { \
    apply_board_preset {1} \
    boot_config {Custom} \
    configure_noc {Add new AXI NoC} \
    debug_config {JTAG} \
    design_flow {Full System} \
    mc_type {DDR} \
    num_mc_ddr {1} \
    num_mc_lpddr {None} \
    pl_clocks {None} \
    pl_resets {None} \
} [get_bd_cells versal_cips_0]

puts "INFO: Board automation applied, configuring additional settings..."

# Additional configuration for PL clocks, UART, and interrupts
set_property -dict [list \
    CONFIG.PS_PMC_CONFIG { \
        PMC_CRP_PL0_REF_CTRL_FREQMHZ {100} \
        PMC_CRP_PL1_REF_CTRL_FREQMHZ {250} \
        PS_GEN_IPI0_ENABLE {1} \
        PS_GEN_IPI1_ENABLE {1} \
        PS_GEN_IPI2_ENABLE {1} \
        PS_GEN_IPI3_ENABLE {1} \
        PS_GEN_IPI4_ENABLE {1} \
        PS_GEN_IPI5_ENABLE {1} \
        PS_GEN_IPI6_ENABLE {1} \
        PS_IRQ_USAGE {{CH0 1} {CH1 1} {CH10 0} {CH11 0} {CH12 0} {CH13 0} {CH14 0} {CH15 0} {CH2 0} {CH3 0} {CH4 0} {CH5 0} {CH6 0} {CH7 0} {CH8 0} {CH9 0}} \
        PS_NUM_FABRIC_RESETS {1} \
        PS_UART0_PERIPHERAL {{ENABLE 1} {IO {PMC_MIO 42 .. 43}}} \
        PS_USE_M_AXI_FPD {1} \
        PS_USE_S_AXI_FPD {1} \
        PS_USE_PMCPL_CLK0 {1} \
        PS_USE_PMCPL_CLK1 {1} \
        PS_TTC0_PERIPHERAL_ENABLE {1} \
        PS_TTC1_PERIPHERAL_ENABLE {1} \
        PS_TTC2_PERIPHERAL_ENABLE {1} \
        PS_TTC3_PERIPHERAL_ENABLE {1} \
        SMON_ALARMS {Set_Coverage_Alarms} \
    } \
] [get_bd_cells versal_cips_0]

puts "INFO: CIPS configuration with DDR4 and UART complete"
puts "INFO: DDR4 is accessible at 0x00000000 (8GB)"
puts "INFO: PS UART0 enabled on PMC_MIO 42..43"
