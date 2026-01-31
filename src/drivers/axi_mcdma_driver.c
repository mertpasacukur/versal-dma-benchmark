/**
 * @file axi_mcdma_driver.c
 * @brief AXI MCDMA Driver Wrapper Implementation
 *
 * Wrapper for Xilinx AXI Multi-Channel DMA IP.
 */

#include <string.h>
#include "xil_io.h"
#include "xil_cache.h"
#include "sleep.h"
#include "axi_mcdma_driver.h"
#include "../platform_config.h"
#include "../utils/timer_utils.h"
#include "../utils/debug_print.h"

/*******************************************************************************
 * Local Variables
 ******************************************************************************/

static AxiMcdmaInst_t g_AxiMcdma = {0};

/* Descriptor memory for all channels */
static McdmaSgDesc_t g_Mm2sDescRing[MCDMA_MAX_CHANNELS][MAX_SG_DESCRIPTORS] __attribute__((aligned(64)));
static McdmaSgDesc_t g_S2mmDescRing[MCDMA_MAX_CHANNELS][MAX_SG_DESCRIPTORS] __attribute__((aligned(64)));

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

static inline void mcdma_write_reg(uint32_t offset, uint32_t value)
{
    Xil_Out32(g_AxiMcdma.base_addr + offset, value);
}

static inline uint32_t mcdma_read_reg(uint32_t offset)
{
    return Xil_In32(g_AxiMcdma.base_addr + offset);
}

static inline void mcdma_write_mm2s_ch_reg(uint32_t channel, uint32_t offset, uint32_t value)
{
    uint32_t ch_base = MCDMA_MM2S_BASE_OFFSET + (channel * MCDMA_CHANNEL_OFFSET);
    Xil_Out32(g_AxiMcdma.base_addr + ch_base + offset, value);
}

static inline uint32_t mcdma_read_mm2s_ch_reg(uint32_t channel, uint32_t offset)
{
    uint32_t ch_base = MCDMA_MM2S_BASE_OFFSET + (channel * MCDMA_CHANNEL_OFFSET);
    return Xil_In32(g_AxiMcdma.base_addr + ch_base + offset);
}

static inline void mcdma_write_s2mm_ch_reg(uint32_t channel, uint32_t offset, uint32_t value)
{
    uint32_t ch_base = MCDMA_S2MM_BASE_OFFSET + (channel * MCDMA_CHANNEL_OFFSET);
    Xil_Out32(g_AxiMcdma.base_addr + ch_base + offset, value);
}

static inline uint32_t mcdma_read_s2mm_ch_reg(uint32_t channel, uint32_t offset)
{
    uint32_t ch_base = MCDMA_S2MM_BASE_OFFSET + (channel * MCDMA_CHANNEL_OFFSET);
    return Xil_In32(g_AxiMcdma.base_addr + ch_base + offset);
}

/*******************************************************************************
 * Initialization Functions
 ******************************************************************************/

int axi_mcdma_init(void)
{
    uint32_t i;

    LOG_DEBUG("AXI MCDMA: Initializing at 0x%08llX\r\n", AXI_MCDMA_BASE_ADDR);

    /* Initialize instance structure */
    memset(&g_AxiMcdma, 0, sizeof(g_AxiMcdma));
    g_AxiMcdma.base_addr = AXI_MCDMA_BASE_ADDR;
    g_AxiMcdma.num_mm2s_channels = AXI_MCDMA_NUM_MM2S_CHANNELS;
    g_AxiMcdma.num_s2mm_channels = AXI_MCDMA_NUM_S2MM_CHANNELS;
    g_AxiMcdma.data_width = AXI_MCDMA_DATA_WIDTH;
    g_AxiMcdma.sched_mode = MCDMA_SCHED_ROUND_ROBIN;

    /* Initialize channel structures */
    for (i = 0; i < MCDMA_MAX_CHANNELS; i++) {
        g_AxiMcdma.mm2s_channels[i].channel_id = i;
        g_AxiMcdma.mm2s_channels[i].enabled = false;
        g_AxiMcdma.mm2s_channels[i].busy = false;
        g_AxiMcdma.mm2s_channels[i].desc_ring = g_Mm2sDescRing[i];
        g_AxiMcdma.mm2s_channels[i].ring_size = MAX_SG_DESCRIPTORS;

        g_AxiMcdma.s2mm_channels[i].channel_id = i;
        g_AxiMcdma.s2mm_channels[i].enabled = false;
        g_AxiMcdma.s2mm_channels[i].busy = false;
        g_AxiMcdma.s2mm_channels[i].desc_ring = g_S2mmDescRing[i];
        g_AxiMcdma.s2mm_channels[i].ring_size = MAX_SG_DESCRIPTORS;
    }

    /* Reset MCDMA */
    if (axi_mcdma_reset() != 0) {
        LOG_ERROR("AXI MCDMA: Reset failed\r\n");
        return DMA_ERROR_DMA_FAIL;
    }

    g_AxiMcdma.initialized = true;
    LOG_DEBUG("AXI MCDMA: Initialization complete (%d MM2S, %d S2MM channels)\r\n",
              g_AxiMcdma.num_mm2s_channels, g_AxiMcdma.num_s2mm_channels);

    return DMA_SUCCESS;
}

void axi_mcdma_cleanup(void)
{
    uint32_t i;

    if (!g_AxiMcdma.initialized) {
        return;
    }

    /* Disable all channels */
    for (i = 0; i < g_AxiMcdma.num_mm2s_channels; i++) {
        axi_mcdma_disable_mm2s_channel(i);
    }
    for (i = 0; i < g_AxiMcdma.num_s2mm_channels; i++) {
        axi_mcdma_disable_s2mm_channel(i);
    }

    /* Reset MCDMA */
    axi_mcdma_reset();

    g_AxiMcdma.initialized = false;
    LOG_DEBUG("AXI MCDMA: Cleanup complete\r\n");
}

int axi_mcdma_reset(void)
{
    uint32_t timeout = 1000;
    uint32_t status;

    /* Reset MM2S */
    mcdma_write_reg(MCDMA_MM2S_BASE_OFFSET + XMCDMA_CCR_OFFSET, XMCDMA_CCR_RESET_MASK);

    while (timeout > 0) {
        status = mcdma_read_reg(MCDMA_MM2S_BASE_OFFSET + XMCDMA_CCR_OFFSET);
        if (!(status & XMCDMA_CCR_RESET_MASK)) {
            break;
        }
        usleep(1);
        timeout--;
    }

    if (timeout == 0) {
        return DMA_ERROR_TIMEOUT;
    }

    /* Reset S2MM */
    timeout = 1000;
    mcdma_write_reg(MCDMA_S2MM_BASE_OFFSET + XMCDMA_CCR_OFFSET, XMCDMA_CCR_RESET_MASK);

    while (timeout > 0) {
        status = mcdma_read_reg(MCDMA_S2MM_BASE_OFFSET + XMCDMA_CCR_OFFSET);
        if (!(status & XMCDMA_CCR_RESET_MASK)) {
            break;
        }
        usleep(1);
        timeout--;
    }

    if (timeout == 0) {
        return DMA_ERROR_TIMEOUT;
    }

    return DMA_SUCCESS;
}

int axi_mcdma_set_scheduler(McdmaSchedMode_t mode)
{
    if (!g_AxiMcdma.initialized) {
        return DMA_ERROR_NOT_INIT;
    }

    g_AxiMcdma.sched_mode = mode;
    return DMA_SUCCESS;
}

/*******************************************************************************
 * Channel Configuration Functions
 ******************************************************************************/

int axi_mcdma_enable_mm2s_channel(uint32_t channel, bool use_irq)
{
    uint32_t cr_value;
    uint32_t chen;

    if (!g_AxiMcdma.initialized || channel >= g_AxiMcdma.num_mm2s_channels) {
        return DMA_ERROR_INVALID_PARAM;
    }

    /* Setup descriptor ring */
    axi_mcdma_setup_mm2s_ring(channel, g_Mm2sDescRing[channel], MAX_SG_DESCRIPTORS);

    /* Configure channel */
    cr_value = 0;
    if (use_irq) {
        cr_value |= XMCDMA_CH_CR_ALL_IRQ_EN;
    }
    mcdma_write_mm2s_ch_reg(channel, XMCDMA_CH_CR_OFFSET, cr_value);

    /* Enable channel in channel enable register */
    chen = mcdma_read_reg(MCDMA_MM2S_BASE_OFFSET + XMCDMA_CHEN_OFFSET);
    chen |= (1 << channel);
    mcdma_write_reg(MCDMA_MM2S_BASE_OFFSET + XMCDMA_CHEN_OFFSET, chen);

    g_AxiMcdma.mm2s_channels[channel].enabled = true;

    return DMA_SUCCESS;
}

int axi_mcdma_enable_s2mm_channel(uint32_t channel, bool use_irq)
{
    uint32_t cr_value;
    uint32_t chen;

    if (!g_AxiMcdma.initialized || channel >= g_AxiMcdma.num_s2mm_channels) {
        return DMA_ERROR_INVALID_PARAM;
    }

    /* Setup descriptor ring */
    axi_mcdma_setup_s2mm_ring(channel, g_S2mmDescRing[channel], MAX_SG_DESCRIPTORS);

    /* Configure channel */
    cr_value = 0;
    if (use_irq) {
        cr_value |= XMCDMA_CH_CR_ALL_IRQ_EN;
    }
    mcdma_write_s2mm_ch_reg(channel, XMCDMA_CH_CR_OFFSET, cr_value);

    /* Enable channel in channel enable register */
    chen = mcdma_read_reg(MCDMA_S2MM_BASE_OFFSET + XMCDMA_CHEN_OFFSET);
    chen |= (1 << channel);
    mcdma_write_reg(MCDMA_S2MM_BASE_OFFSET + XMCDMA_CHEN_OFFSET, chen);

    g_AxiMcdma.s2mm_channels[channel].enabled = true;

    return DMA_SUCCESS;
}

int axi_mcdma_disable_mm2s_channel(uint32_t channel)
{
    uint32_t chen;

    if (channel >= MCDMA_MAX_CHANNELS) {
        return DMA_ERROR_INVALID_PARAM;
    }

    /* Clear channel run bit */
    mcdma_write_mm2s_ch_reg(channel, XMCDMA_CH_CR_OFFSET, 0);

    /* Disable channel */
    chen = mcdma_read_reg(MCDMA_MM2S_BASE_OFFSET + XMCDMA_CHEN_OFFSET);
    chen &= ~(1 << channel);
    mcdma_write_reg(MCDMA_MM2S_BASE_OFFSET + XMCDMA_CHEN_OFFSET, chen);

    g_AxiMcdma.mm2s_channels[channel].enabled = false;

    return DMA_SUCCESS;
}

int axi_mcdma_disable_s2mm_channel(uint32_t channel)
{
    uint32_t chen;

    if (channel >= MCDMA_MAX_CHANNELS) {
        return DMA_ERROR_INVALID_PARAM;
    }

    /* Clear channel run bit */
    mcdma_write_s2mm_ch_reg(channel, XMCDMA_CH_CR_OFFSET, 0);

    /* Disable channel */
    chen = mcdma_read_reg(MCDMA_S2MM_BASE_OFFSET + XMCDMA_CHEN_OFFSET);
    chen &= ~(1 << channel);
    mcdma_write_reg(MCDMA_S2MM_BASE_OFFSET + XMCDMA_CHEN_OFFSET, chen);

    g_AxiMcdma.s2mm_channels[channel].enabled = false;

    return DMA_SUCCESS;
}

int axi_mcdma_setup_mm2s_ring(uint32_t channel, McdmaSgDesc_t* descs, uint32_t num_descs)
{
    uint32_t i;
    uint64_t desc_addr;

    if (channel >= MCDMA_MAX_CHANNELS || !descs || num_descs == 0) {
        return DMA_ERROR_INVALID_PARAM;
    }

    /* Initialize descriptor ring */
    memset(descs, 0, num_descs * sizeof(McdmaSgDesc_t));
    for (i = 0; i < num_descs; i++) {
        desc_addr = (uint64_t)&descs[(i + 1) % num_descs];
        descs[i].next_desc = (uint32_t)(desc_addr & 0xFFFFFFFF);
        descs[i].next_desc_msb = (uint32_t)(desc_addr >> 32);
    }

    Xil_DCacheFlushRange((UINTPTR)descs, num_descs * sizeof(McdmaSgDesc_t));

    g_AxiMcdma.mm2s_channels[channel].desc_ring = descs;
    g_AxiMcdma.mm2s_channels[channel].ring_size = num_descs;
    g_AxiMcdma.mm2s_channels[channel].desc_head = 0;
    g_AxiMcdma.mm2s_channels[channel].desc_tail = 0;

    return DMA_SUCCESS;
}

int axi_mcdma_setup_s2mm_ring(uint32_t channel, McdmaSgDesc_t* descs, uint32_t num_descs)
{
    uint32_t i;
    uint64_t desc_addr;

    if (channel >= MCDMA_MAX_CHANNELS || !descs || num_descs == 0) {
        return DMA_ERROR_INVALID_PARAM;
    }

    /* Initialize descriptor ring */
    memset(descs, 0, num_descs * sizeof(McdmaSgDesc_t));
    for (i = 0; i < num_descs; i++) {
        desc_addr = (uint64_t)&descs[(i + 1) % num_descs];
        descs[i].next_desc = (uint32_t)(desc_addr & 0xFFFFFFFF);
        descs[i].next_desc_msb = (uint32_t)(desc_addr >> 32);
    }

    Xil_DCacheFlushRange((UINTPTR)descs, num_descs * sizeof(McdmaSgDesc_t));

    g_AxiMcdma.s2mm_channels[channel].desc_ring = descs;
    g_AxiMcdma.s2mm_channels[channel].ring_size = num_descs;
    g_AxiMcdma.s2mm_channels[channel].desc_head = 0;
    g_AxiMcdma.s2mm_channels[channel].desc_tail = 0;

    return DMA_SUCCESS;
}

/*******************************************************************************
 * Transfer Functions
 ******************************************************************************/

int axi_mcdma_transfer(uint32_t channel, uint64_t src_addr, uint64_t dst_addr, uint32_t length)
{
    int status;

    /* Start S2MM (RX) first */
    status = axi_mcdma_start_s2mm(channel, dst_addr, length);
    if (status != DMA_SUCCESS) {
        return status;
    }

    /* Start MM2S (TX) */
    status = axi_mcdma_start_mm2s(channel, src_addr, length);
    if (status != DMA_SUCCESS) {
        return status;
    }

    return DMA_SUCCESS;
}

int axi_mcdma_start_mm2s(uint32_t channel, uint64_t buffer_addr, uint32_t length)
{
    McdmaChannel_t* ch;
    McdmaSgDesc_t* desc;
    uint64_t desc_addr;
    uint32_t cr_value;

    if (!g_AxiMcdma.initialized || channel >= g_AxiMcdma.num_mm2s_channels) {
        return DMA_ERROR_INVALID_PARAM;
    }

    ch = &g_AxiMcdma.mm2s_channels[channel];
    if (!ch->enabled) {
        return DMA_ERROR_NOT_INIT;
    }

    /* Setup descriptor */
    desc = &ch->desc_ring[ch->desc_head];
    desc->buffer_addr = (uint32_t)(buffer_addr & 0xFFFFFFFF);
    desc->buffer_addr_msb = (uint32_t)(buffer_addr >> 32);
    desc->control = XMCDMA_BD_CTRL_SOF_MASK | XMCDMA_BD_CTRL_EOF_MASK | length;
    desc->status = 0;

    Xil_DCacheFlushRange((UINTPTR)desc, sizeof(McdmaSgDesc_t));
    Xil_DCacheFlushRange(buffer_addr, length);

    ch->transfer_complete = false;
    ch->busy = true;

    /* Set current descriptor */
    desc_addr = (uint64_t)desc;
    mcdma_write_mm2s_ch_reg(channel, XMCDMA_CH_CDESC_OFFSET, (uint32_t)(desc_addr & 0xFFFFFFFF));
    mcdma_write_mm2s_ch_reg(channel, XMCDMA_CH_CDESC_MSB_OFFSET, (uint32_t)(desc_addr >> 32));

    /* Start channel */
    cr_value = mcdma_read_mm2s_ch_reg(channel, XMCDMA_CH_CR_OFFSET);
    mcdma_write_mm2s_ch_reg(channel, XMCDMA_CH_CR_OFFSET, cr_value | XMCDMA_CH_CR_RS_MASK);

    /* Set tail descriptor (starts transfer) */
    mcdma_write_mm2s_ch_reg(channel, XMCDMA_CH_TDESC_OFFSET, (uint32_t)(desc_addr & 0xFFFFFFFF));
    mcdma_write_mm2s_ch_reg(channel, XMCDMA_CH_TDESC_MSB_OFFSET, (uint32_t)(desc_addr >> 32));

    ch->desc_head = (ch->desc_head + 1) % ch->ring_size;

    return DMA_SUCCESS;
}

int axi_mcdma_start_s2mm(uint32_t channel, uint64_t buffer_addr, uint32_t length)
{
    McdmaChannel_t* ch;
    McdmaSgDesc_t* desc;
    uint64_t desc_addr;
    uint32_t cr_value;

    if (!g_AxiMcdma.initialized || channel >= g_AxiMcdma.num_s2mm_channels) {
        return DMA_ERROR_INVALID_PARAM;
    }

    ch = &g_AxiMcdma.s2mm_channels[channel];
    if (!ch->enabled) {
        return DMA_ERROR_NOT_INIT;
    }

    /* Setup descriptor */
    desc = &ch->desc_ring[ch->desc_head];
    desc->buffer_addr = (uint32_t)(buffer_addr & 0xFFFFFFFF);
    desc->buffer_addr_msb = (uint32_t)(buffer_addr >> 32);
    desc->control = length;
    desc->status = 0;

    Xil_DCacheFlushRange((UINTPTR)desc, sizeof(McdmaSgDesc_t));
    Xil_DCacheInvalidateRange(buffer_addr, length);

    ch->transfer_complete = false;
    ch->busy = true;

    /* Set current descriptor */
    desc_addr = (uint64_t)desc;
    mcdma_write_s2mm_ch_reg(channel, XMCDMA_CH_CDESC_OFFSET, (uint32_t)(desc_addr & 0xFFFFFFFF));
    mcdma_write_s2mm_ch_reg(channel, XMCDMA_CH_CDESC_MSB_OFFSET, (uint32_t)(desc_addr >> 32));

    /* Start channel */
    cr_value = mcdma_read_s2mm_ch_reg(channel, XMCDMA_CH_CR_OFFSET);
    mcdma_write_s2mm_ch_reg(channel, XMCDMA_CH_CR_OFFSET, cr_value | XMCDMA_CH_CR_RS_MASK);

    /* Set tail descriptor (starts transfer) */
    mcdma_write_s2mm_ch_reg(channel, XMCDMA_CH_TDESC_OFFSET, (uint32_t)(desc_addr & 0xFFFFFFFF));
    mcdma_write_s2mm_ch_reg(channel, XMCDMA_CH_TDESC_MSB_OFFSET, (uint32_t)(desc_addr >> 32));

    ch->desc_head = (ch->desc_head + 1) % ch->ring_size;

    return DMA_SUCCESS;
}

/*******************************************************************************
 * Wait Functions
 ******************************************************************************/

int axi_mcdma_wait_mm2s(uint32_t channel, uint32_t timeout_us)
{
    McdmaChannel_t* ch;
    uint32_t status;
    uint32_t elapsed_us = 0;
    uint32_t poll_interval_us = 10;

    if (channel >= g_AxiMcdma.num_mm2s_channels) {
        return DMA_ERROR_INVALID_PARAM;
    }

    ch = &g_AxiMcdma.mm2s_channels[channel];

    while (elapsed_us < timeout_us) {
        status = mcdma_read_mm2s_ch_reg(channel, XMCDMA_CH_SR_OFFSET);

        if (status & XMCDMA_CH_SR_ERR_MASK) {
            ch->transfer_error = status;
            ch->errors++;
            ch->busy = false;
            LOG_ERROR("MCDMA MM2S ch%lu Error: status=0x%08lX\r\n",
                      (unsigned long)channel, (unsigned long)status);
            mcdma_write_mm2s_ch_reg(channel, XMCDMA_CH_SR_OFFSET, status);
            return DMA_ERROR_DMA_FAIL;
        }

        if (status & XMCDMA_CH_SR_IDLE_MASK) {
            ch->transfer_complete = true;
            ch->num_transfers++;
            ch->busy = false;
            return DMA_SUCCESS;
        }

        if (status & XMCDMA_CH_SR_IOC_IRQ_MASK) {
            mcdma_write_mm2s_ch_reg(channel, XMCDMA_CH_SR_OFFSET, XMCDMA_CH_SR_IOC_IRQ_MASK);
            ch->transfer_complete = true;
            ch->num_transfers++;
            ch->busy = false;
            return DMA_SUCCESS;
        }

        usleep(poll_interval_us);
        elapsed_us += poll_interval_us;
    }

    LOG_ERROR("MCDMA MM2S ch%lu Timeout: status=0x%08lX\r\n",
              (unsigned long)channel, (unsigned long)mcdma_read_mm2s_ch_reg(channel, XMCDMA_CH_SR_OFFSET));
    return DMA_ERROR_TIMEOUT;
}

int axi_mcdma_wait_s2mm(uint32_t channel, uint32_t timeout_us)
{
    McdmaChannel_t* ch;
    uint32_t status;
    uint32_t elapsed_us = 0;
    uint32_t poll_interval_us = 10;

    if (channel >= g_AxiMcdma.num_s2mm_channels) {
        return DMA_ERROR_INVALID_PARAM;
    }

    ch = &g_AxiMcdma.s2mm_channels[channel];

    while (elapsed_us < timeout_us) {
        status = mcdma_read_s2mm_ch_reg(channel, XMCDMA_CH_SR_OFFSET);

        if (status & XMCDMA_CH_SR_ERR_MASK) {
            ch->transfer_error = status;
            ch->errors++;
            ch->busy = false;
            LOG_ERROR("MCDMA S2MM ch%lu Error: status=0x%08lX\r\n",
                      (unsigned long)channel, (unsigned long)status);
            mcdma_write_s2mm_ch_reg(channel, XMCDMA_CH_SR_OFFSET, status);
            return DMA_ERROR_DMA_FAIL;
        }

        if (status & XMCDMA_CH_SR_IDLE_MASK) {
            ch->transfer_complete = true;
            ch->num_transfers++;
            ch->busy = false;
            return DMA_SUCCESS;
        }

        if (status & XMCDMA_CH_SR_IOC_IRQ_MASK) {
            mcdma_write_s2mm_ch_reg(channel, XMCDMA_CH_SR_OFFSET, XMCDMA_CH_SR_IOC_IRQ_MASK);
            ch->transfer_complete = true;
            ch->num_transfers++;
            ch->busy = false;
            return DMA_SUCCESS;
        }

        usleep(poll_interval_us);
        elapsed_us += poll_interval_us;
    }

    LOG_ERROR("MCDMA S2MM ch%lu Timeout: status=0x%08lX\r\n",
              (unsigned long)channel, (unsigned long)mcdma_read_s2mm_ch_reg(channel, XMCDMA_CH_SR_OFFSET));
    return DMA_ERROR_TIMEOUT;
}

int axi_mcdma_wait_complete(uint32_t channel, uint32_t timeout_us)
{
    int status;

    status = axi_mcdma_wait_mm2s(channel, timeout_us);
    if (status != DMA_SUCCESS) {
        return status;
    }

    status = axi_mcdma_wait_s2mm(channel, timeout_us);
    return status;
}

/*******************************************************************************
 * Status Functions
 ******************************************************************************/

bool axi_mcdma_mm2s_busy(uint32_t channel)
{
    if (channel >= MCDMA_MAX_CHANNELS) {
        return false;
    }
    uint32_t status = mcdma_read_mm2s_ch_reg(channel, XMCDMA_CH_SR_OFFSET);
    return !(status & XMCDMA_CH_SR_IDLE_MASK);
}

bool axi_mcdma_s2mm_busy(uint32_t channel)
{
    if (channel >= MCDMA_MAX_CHANNELS) {
        return false;
    }
    uint32_t status = mcdma_read_s2mm_ch_reg(channel, XMCDMA_CH_SR_OFFSET);
    return !(status & XMCDMA_CH_SR_IDLE_MASK);
}

uint32_t axi_mcdma_get_mm2s_channel_count(void)
{
    return g_AxiMcdma.num_mm2s_channels;
}

uint32_t axi_mcdma_get_s2mm_channel_count(void)
{
    return g_AxiMcdma.num_s2mm_channels;
}

AxiMcdmaInst_t* axi_mcdma_get_instance(void)
{
    return &g_AxiMcdma;
}

/*******************************************************************************
 * Statistics Functions
 ******************************************************************************/

void axi_mcdma_print_stats(void)
{
    uint32_t i;

    LOG_ALWAYS("\r\n=== AXI MCDMA Statistics ===\r\n");
    LOG_ALWAYS("Total Transfers: %lu\r\n", g_AxiMcdma.total_transfers);
    LOG_ALWAYS("Total Bytes:     %llu\r\n", g_AxiMcdma.total_bytes);
    LOG_ALWAYS("Total Errors:    %lu\r\n", g_AxiMcdma.total_errors);

    LOG_ALWAYS("\r\nMM2S Channels:\r\n");
    for (i = 0; i < g_AxiMcdma.num_mm2s_channels; i++) {
        if (g_AxiMcdma.mm2s_channels[i].enabled) {
            LOG_ALWAYS("  CH%d: %lu transfers, %lu errors\r\n",
                       i, g_AxiMcdma.mm2s_channels[i].num_transfers,
                       g_AxiMcdma.mm2s_channels[i].errors);
        }
    }

    LOG_ALWAYS("\r\nS2MM Channels:\r\n");
    for (i = 0; i < g_AxiMcdma.num_s2mm_channels; i++) {
        if (g_AxiMcdma.s2mm_channels[i].enabled) {
            LOG_ALWAYS("  CH%d: %lu transfers, %lu errors\r\n",
                       i, g_AxiMcdma.s2mm_channels[i].num_transfers,
                       g_AxiMcdma.s2mm_channels[i].errors);
        }
    }
    LOG_ALWAYS("============================\r\n");
}

void axi_mcdma_clear_stats(void)
{
    uint32_t i;

    g_AxiMcdma.total_bytes = 0;
    g_AxiMcdma.total_transfers = 0;
    g_AxiMcdma.total_errors = 0;

    for (i = 0; i < MCDMA_MAX_CHANNELS; i++) {
        g_AxiMcdma.mm2s_channels[i].bytes_transferred = 0;
        g_AxiMcdma.mm2s_channels[i].num_transfers = 0;
        g_AxiMcdma.mm2s_channels[i].errors = 0;

        g_AxiMcdma.s2mm_channels[i].bytes_transferred = 0;
        g_AxiMcdma.s2mm_channels[i].num_transfers = 0;
        g_AxiMcdma.s2mm_channels[i].errors = 0;
    }
}

/*******************************************************************************
 * Interrupt Handler
 ******************************************************************************/

void axi_mcdma_irq_handler(void)
{
    uint32_t i, status;

    /* Handle MM2S interrupts */
    for (i = 0; i < g_AxiMcdma.num_mm2s_channels; i++) {
        if (g_AxiMcdma.mm2s_channels[i].enabled) {
            status = mcdma_read_mm2s_ch_reg(i, XMCDMA_CH_SR_OFFSET);
            if (status & XMCDMA_CH_SR_ALL_IRQ_MASK) {
                mcdma_write_mm2s_ch_reg(i, XMCDMA_CH_SR_OFFSET, status & XMCDMA_CH_SR_ALL_IRQ_MASK);

                if (status & XMCDMA_CH_SR_ERR_IRQ_MASK) {
                    g_AxiMcdma.mm2s_channels[i].transfer_error = status;
                    g_AxiMcdma.mm2s_channels[i].errors++;
                    g_AxiMcdma.total_errors++;
                }

                if (status & XMCDMA_CH_SR_IOC_IRQ_MASK) {
                    g_AxiMcdma.mm2s_channels[i].transfer_complete = true;
                    g_AxiMcdma.mm2s_channels[i].num_transfers++;
                    g_AxiMcdma.total_transfers++;
                }

                g_AxiMcdma.mm2s_channels[i].busy = false;
            }
        }
    }

    /* Handle S2MM interrupts */
    for (i = 0; i < g_AxiMcdma.num_s2mm_channels; i++) {
        if (g_AxiMcdma.s2mm_channels[i].enabled) {
            status = mcdma_read_s2mm_ch_reg(i, XMCDMA_CH_SR_OFFSET);
            if (status & XMCDMA_CH_SR_ALL_IRQ_MASK) {
                mcdma_write_s2mm_ch_reg(i, XMCDMA_CH_SR_OFFSET, status & XMCDMA_CH_SR_ALL_IRQ_MASK);

                if (status & XMCDMA_CH_SR_ERR_IRQ_MASK) {
                    g_AxiMcdma.s2mm_channels[i].transfer_error = status;
                    g_AxiMcdma.s2mm_channels[i].errors++;
                    g_AxiMcdma.total_errors++;
                }

                if (status & XMCDMA_CH_SR_IOC_IRQ_MASK) {
                    g_AxiMcdma.s2mm_channels[i].transfer_complete = true;
                    g_AxiMcdma.s2mm_channels[i].num_transfers++;
                    g_AxiMcdma.total_transfers++;
                }

                g_AxiMcdma.s2mm_channels[i].busy = false;
            }
        }
    }
}
