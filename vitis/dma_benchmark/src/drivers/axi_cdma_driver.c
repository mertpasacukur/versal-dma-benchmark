/**
 * @file axi_cdma_driver.c
 * @brief AXI CDMA Driver Wrapper Implementation
 *
 * Wrapper for Xilinx AXI Central DMA IP for memory-to-memory transfers.
 */

#include <string.h>
#include "xil_io.h"
#include "xil_cache.h"
#include "sleep.h"
#include "axi_cdma_driver.h"
#include "../platform_config.h"
#include "../utils/timer_utils.h"
#include "../utils/debug_print.h"

/*******************************************************************************
 * Local Variables
 ******************************************************************************/

static AxiCdmaInst_t g_AxiCdma = {0};

/* Descriptor memory (aligned to 64 bytes) */
static AxiCdmaSgDesc_t g_DescRing[MAX_SG_DESCRIPTORS] __attribute__((aligned(64)));

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

static inline void axi_cdma_write_reg(uint32_t offset, uint32_t value)
{
    Xil_Out32(g_AxiCdma.base_addr + offset, value);
}

static inline uint32_t axi_cdma_read_reg(uint32_t offset)
{
    return Xil_In32(g_AxiCdma.base_addr + offset);
}

/*******************************************************************************
 * Initialization Functions
 ******************************************************************************/

int axi_cdma_init(void)
{
    uint32_t status;

    LOG_DEBUG("AXI CDMA: Initializing at 0x%08llX\r\n", AXI_CDMA_BASE_ADDR);

    /* Initialize instance structure */
    memset(&g_AxiCdma, 0, sizeof(g_AxiCdma));
    g_AxiCdma.base_addr = AXI_CDMA_BASE_ADDR;
    g_AxiCdma.data_width = AXI_CDMA_DATA_WIDTH;
    g_AxiCdma.addr_width = AXI_CDMA_ADDR_WIDTH;
    g_AxiCdma.max_burst_len = AXI_CDMA_MAX_BURST_LEN;

    /* Check SG mode support */
    status = axi_cdma_read_reg(XAXICDMA_SR_OFFSET);
    g_AxiCdma.sg_mode = (status & XAXICDMA_SR_SGINCL_MASK) ? true : false;

    LOG_DEBUG("AXI CDMA: SG Mode = %s\r\n", g_AxiCdma.sg_mode ? "Yes" : "No");

    /* Reset CDMA */
    if (axi_cdma_reset() != 0) {
        LOG_ERROR("AXI CDMA: Reset failed\r\n");
        return DMA_ERROR_DMA_FAIL;
    }

    /* Setup SG descriptor ring if SG mode */
    if (g_AxiCdma.sg_mode) {
        if (axi_cdma_setup_sg_ring(g_DescRing, MAX_SG_DESCRIPTORS) != 0) {
            LOG_ERROR("AXI CDMA: SG ring setup failed\r\n");
            return DMA_ERROR_DMA_FAIL;
        }
    }

    g_AxiCdma.initialized = true;
    LOG_DEBUG("AXI CDMA: Initialization complete\r\n");
    return DMA_SUCCESS;
}

void axi_cdma_cleanup(void)
{
    if (!g_AxiCdma.initialized) {
        return;
    }

    /* Reset CDMA */
    axi_cdma_reset();

    g_AxiCdma.initialized = false;
    LOG_DEBUG("AXI CDMA: Cleanup complete\r\n");
}

int axi_cdma_reset(void)
{
    uint32_t timeout = 1000;
    uint32_t status;

    /* Issue reset */
    axi_cdma_write_reg(XAXICDMA_CR_OFFSET, XAXICDMA_CR_RESET_MASK);

    /* Wait for reset to complete */
    while (timeout > 0) {
        status = axi_cdma_read_reg(XAXICDMA_CR_OFFSET);
        if (!(status & XAXICDMA_CR_RESET_MASK)) {
            break;
        }
        usleep(1);
        timeout--;
    }

    if (timeout == 0) {
        LOG_ERROR("AXI CDMA: Reset timeout\r\n");
        return DMA_ERROR_TIMEOUT;
    }

    /* Clear completion flag */
    g_AxiCdma.transfer_complete = false;
    g_AxiCdma.transfer_error = 0;

    return DMA_SUCCESS;
}

int axi_cdma_configure(bool use_sg, bool use_irq)
{
    uint32_t cr_value = 0;

    if (!g_AxiCdma.initialized) {
        return DMA_ERROR_NOT_INIT;
    }

    /* Configure control register */
    if (use_irq) {
        cr_value |= XAXICDMA_CR_IOC_IRQ_EN | XAXICDMA_CR_ERR_IRQ_EN;
    }

    axi_cdma_write_reg(XAXICDMA_CR_OFFSET, cr_value);

    return DMA_SUCCESS;
}

int axi_cdma_setup_sg_ring(AxiCdmaSgDesc_t* descs, uint32_t num_descs)
{
    uint32_t i;
    uint64_t desc_addr;

    if (!descs || num_descs == 0) {
        return DMA_ERROR_INVALID_PARAM;
    }

    g_AxiCdma.desc_ring = descs;
    g_AxiCdma.ring_size = num_descs;

    /* Initialize descriptor ring */
    memset(descs, 0, num_descs * sizeof(AxiCdmaSgDesc_t));
    for (i = 0; i < num_descs; i++) {
        desc_addr = (uint64_t)&descs[(i + 1) % num_descs];
        descs[i].next_desc = (uint32_t)(desc_addr & 0xFFFFFFFF);
        descs[i].next_desc_msb = (uint32_t)(desc_addr >> 32);
    }

    /* Flush descriptor memory */
    Xil_DCacheFlushRange((UINTPTR)descs, num_descs * sizeof(AxiCdmaSgDesc_t));

    g_AxiCdma.desc_head = 0;
    g_AxiCdma.desc_tail = 0;

    return DMA_SUCCESS;
}

/*******************************************************************************
 * Transfer Functions
 ******************************************************************************/

int axi_cdma_simple_transfer(uint64_t src_addr, uint64_t dst_addr, uint32_t length)
{
    if (!g_AxiCdma.initialized) {
        return DMA_ERROR_NOT_INIT;
    }

    /* Check if CDMA is busy */
    if (axi_cdma_is_busy()) {
        return DMA_ERROR_BUSY;
    }

    /* Flush source buffer from cache */
    Xil_DCacheFlushRange(src_addr, length);

    /* Invalidate destination buffer */
    Xil_DCacheInvalidateRange(dst_addr, length);

    /* Clear completion flag */
    g_AxiCdma.transfer_complete = false;

    /* Set source address */
    axi_cdma_write_reg(XAXICDMA_SA_OFFSET, (uint32_t)(src_addr & 0xFFFFFFFF));
    axi_cdma_write_reg(XAXICDMA_SA_MSB_OFFSET, (uint32_t)(src_addr >> 32));

    /* Set destination address */
    axi_cdma_write_reg(XAXICDMA_DA_OFFSET, (uint32_t)(dst_addr & 0xFFFFFFFF));
    axi_cdma_write_reg(XAXICDMA_DA_MSB_OFFSET, (uint32_t)(dst_addr >> 32));

    /* Set bytes to transfer (starts the transfer) */
    axi_cdma_write_reg(XAXICDMA_BTT_OFFSET, length);

    return DMA_SUCCESS;
}

int axi_cdma_sg_transfer(uint64_t src_addr, uint64_t dst_addr, uint32_t length)
{
    uint64_t desc_addr;
    AxiCdmaSgDesc_t* desc;

    if (!g_AxiCdma.initialized || !g_AxiCdma.sg_mode) {
        return DMA_ERROR_NOT_INIT;
    }

    /* Check if CDMA is busy */
    if (axi_cdma_is_busy()) {
        return DMA_ERROR_BUSY;
    }

    /* Setup descriptor */
    desc = &g_AxiCdma.desc_ring[g_AxiCdma.desc_head];
    desc->src_addr = (uint32_t)(src_addr & 0xFFFFFFFF);
    desc->src_addr_msb = (uint32_t)(src_addr >> 32);
    desc->dst_addr = (uint32_t)(dst_addr & 0xFFFFFFFF);
    desc->dst_addr_msb = (uint32_t)(dst_addr >> 32);
    desc->control = length;
    desc->status = 0;

    /* Flush descriptor and buffers */
    Xil_DCacheFlushRange((UINTPTR)desc, sizeof(AxiCdmaSgDesc_t));
    Xil_DCacheFlushRange(src_addr, length);
    Xil_DCacheInvalidateRange(dst_addr, length);

    /* Clear completion flag */
    g_AxiCdma.transfer_complete = false;

    /* Set current descriptor pointer */
    desc_addr = (uint64_t)desc;
    axi_cdma_write_reg(XAXICDMA_CDESC_OFFSET, (uint32_t)(desc_addr & 0xFFFFFFFF));
    axi_cdma_write_reg(XAXICDMA_CDESC_MSB_OFFSET, (uint32_t)(desc_addr >> 32));

    /* Set tail descriptor pointer (starts the transfer) */
    axi_cdma_write_reg(XAXICDMA_TDESC_OFFSET, (uint32_t)(desc_addr & 0xFFFFFFFF));
    axi_cdma_write_reg(XAXICDMA_TDESC_MSB_OFFSET, (uint32_t)(desc_addr >> 32));

    /* Update ring index */
    g_AxiCdma.desc_head = (g_AxiCdma.desc_head + 1) % g_AxiCdma.ring_size;

    return DMA_SUCCESS;
}

/*******************************************************************************
 * Wait Functions
 ******************************************************************************/

int axi_cdma_wait_complete(uint32_t timeout_us)
{
    uint64_t start_time = timer_get_us();
    uint32_t status;

    while ((timer_get_us() - start_time) < timeout_us) {
        status = axi_cdma_read_reg(XAXICDMA_SR_OFFSET);

        /* Check for errors */
        if (status & XAXICDMA_SR_ALL_ERR_MASK) {
            g_AxiCdma.transfer_error = status & XAXICDMA_SR_ALL_ERR_MASK;
            g_AxiCdma.errors++;
            /* Clear error bits by writing 1 */
            axi_cdma_write_reg(XAXICDMA_SR_OFFSET, status & XAXICDMA_SR_ALL_ERR_MASK);
            return DMA_ERROR_DMA_FAIL;
        }

        /* Check for completion (idle) */
        if (status & XAXICDMA_SR_IDLE_MASK) {
            g_AxiCdma.transfer_complete = true;
            g_AxiCdma.num_transfers++;
            return DMA_SUCCESS;
        }

        /* Check IOC interrupt status */
        if (status & XAXICDMA_SR_IOC_IRQ_MASK) {
            /* Clear interrupt */
            axi_cdma_write_reg(XAXICDMA_SR_OFFSET, XAXICDMA_SR_IOC_IRQ_MASK);
            g_AxiCdma.transfer_complete = true;
            g_AxiCdma.num_transfers++;
            return DMA_SUCCESS;
        }
    }

    return DMA_ERROR_TIMEOUT;
}

/*******************************************************************************
 * Status Functions
 ******************************************************************************/

bool axi_cdma_is_busy(void)
{
    uint32_t status = axi_cdma_read_reg(XAXICDMA_SR_OFFSET);
    return !(status & XAXICDMA_SR_IDLE_MASK);
}

uint32_t axi_cdma_get_status(void)
{
    return axi_cdma_read_reg(XAXICDMA_SR_OFFSET);
}

AxiCdmaInst_t* axi_cdma_get_instance(void)
{
    return &g_AxiCdma;
}

/*******************************************************************************
 * Statistics Functions
 ******************************************************************************/

void axi_cdma_print_stats(void)
{
    LOG_ALWAYS("\r\n=== AXI CDMA Statistics ===\r\n");
    LOG_ALWAYS("Transfers:    %lu\r\n", g_AxiCdma.num_transfers);
    LOG_ALWAYS("Bytes:        %llu\r\n", g_AxiCdma.bytes_transferred);
    LOG_ALWAYS("Errors:       %lu\r\n", g_AxiCdma.errors);
    LOG_ALWAYS("===========================\r\n");
}

void axi_cdma_clear_stats(void)
{
    g_AxiCdma.bytes_transferred = 0;
    g_AxiCdma.num_transfers = 0;
    g_AxiCdma.errors = 0;
}

/*******************************************************************************
 * Interrupt Handler
 ******************************************************************************/

void axi_cdma_irq_handler(void)
{
    uint32_t status;

    /* Read status register */
    status = axi_cdma_read_reg(XAXICDMA_SR_OFFSET);

    /* Handle interrupts */
    if (status & XAXICDMA_SR_ALL_IRQ_MASK) {
        /* Clear interrupt bits */
        axi_cdma_write_reg(XAXICDMA_SR_OFFSET, status & XAXICDMA_SR_ALL_IRQ_MASK);

        if (status & XAXICDMA_SR_ERR_IRQ_MASK) {
            g_AxiCdma.transfer_error = status & XAXICDMA_SR_ALL_ERR_MASK;
            g_AxiCdma.errors++;
        }

        if (status & XAXICDMA_SR_IOC_IRQ_MASK) {
            g_AxiCdma.transfer_complete = true;
            g_AxiCdma.num_transfers++;
        }
    }
}
