/**
 * @file lpd_dma_driver.c
 * @brief LPD DMA (ADMA) Driver Wrapper Implementation
 *
 * Wrapper for Versal PS LPD DMA (ADMA) - 8 channel DMA engine.
 */

#include <string.h>
#include "xil_io.h"
#include "xil_cache.h"
#include "sleep.h"
#include "lpd_dma_driver.h"
#include "../platform_config.h"
#include "../utils/timer_utils.h"
#include "../utils/debug_print.h"

/*******************************************************************************
 * Local Variables
 ******************************************************************************/

static LpdDmaInst_t g_LpdDma = {0};

/* Channel base addresses */
static const uint64_t g_ChannelBaseAddrs[LPD_DMA_NUM_CHANNELS] = {
    LPD_DMA_CH0_BASE_ADDR,
    LPD_DMA_CH1_BASE_ADDR,
    LPD_DMA_CH2_BASE_ADDR,
    LPD_DMA_CH3_BASE_ADDR,
    LPD_DMA_CH4_BASE_ADDR,
    LPD_DMA_CH5_BASE_ADDR,
    LPD_DMA_CH6_BASE_ADDR,
    LPD_DMA_CH7_BASE_ADDR
};

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

static inline void lpd_dma_write_reg(uint32_t channel, uint32_t offset, uint32_t value)
{
    if (channel < LPD_DMA_NUM_CHANNELS) {
        Xil_Out32(g_ChannelBaseAddrs[channel] + offset, value);
    }
}

static inline uint32_t lpd_dma_read_reg(uint32_t channel, uint32_t offset)
{
    if (channel < LPD_DMA_NUM_CHANNELS) {
        return Xil_In32(g_ChannelBaseAddrs[channel] + offset);
    }
    return 0;
}

/*******************************************************************************
 * Initialization Functions
 ******************************************************************************/

int lpd_dma_init(void)
{
    uint32_t i;

    LOG_DEBUG("LPD DMA: Initializing %d channels\r\n", LPD_DMA_NUM_CHANNELS);

    /* Initialize instance structure */
    memset(&g_LpdDma, 0, sizeof(g_LpdDma));

    /* Initialize each channel */
    for (i = 0; i < LPD_DMA_NUM_CHANNELS; i++) {
        g_LpdDma.channels[i].channel_id = i;
        g_LpdDma.channels[i].base_addr = g_ChannelBaseAddrs[i];
        g_LpdDma.channels[i].initialized = false;
        g_LpdDma.channels[i].busy = false;

        /* Reset channel */
        if (lpd_dma_reset_channel(i) != 0) {
            LOG_DEBUG("LPD DMA: Channel %d reset failed\r\n", i);
            continue;
        }

        /* Configure channel */
        if (lpd_dma_configure_channel(i, false) != 0) {
            LOG_DEBUG("LPD DMA: Channel %d configure failed\r\n", i);
            continue;
        }

        g_LpdDma.channels[i].initialized = true;
    }

    g_LpdDma.initialized = true;
    LOG_DEBUG("LPD DMA: Initialization complete\r\n");
    return DMA_SUCCESS;
}

void lpd_dma_cleanup(void)
{
    uint32_t i;

    if (!g_LpdDma.initialized) {
        return;
    }

    /* Reset all channels */
    for (i = 0; i < LPD_DMA_NUM_CHANNELS; i++) {
        lpd_dma_reset_channel(i);
    }

    g_LpdDma.initialized = false;
    LOG_DEBUG("LPD DMA: Cleanup complete\r\n");
}

int lpd_dma_reset_channel(uint32_t channel)
{
    uint32_t timeout = 1000;
    uint32_t status;

    if (channel >= LPD_DMA_NUM_CHANNELS) {
        return DMA_ERROR_INVALID_PARAM;
    }

    /* Disable all interrupts */
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_IDS, XLPDDMA_IXR_ALL_MASK);

    /* Clear any pending interrupts */
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_ISR, XLPDDMA_IXR_ALL_MASK);

    /* Wait for channel to be idle */
    while (timeout > 0) {
        status = lpd_dma_read_reg(channel, XLPDDMA_ZDMA_CH_STATUS);
        if ((status & XLPDDMA_STATUS_STATE_MASK) == XLPDDMA_STATUS_STATE_IDLE) {
            break;
        }
        usleep(1);
        timeout--;
    }

    if (timeout == 0) {
        LOG_ERROR("LPD DMA: Channel %d reset timeout\r\n", channel);
        return DMA_ERROR_TIMEOUT;
    }

    /* Clear transfer state */
    g_LpdDma.channels[channel].transfer_complete = false;
    g_LpdDma.channels[channel].transfer_error = 0;
    g_LpdDma.channels[channel].busy = false;

    return DMA_SUCCESS;
}

int lpd_dma_configure_channel(uint32_t channel, bool use_irq)
{
    uint32_t ctrl0;

    if (channel >= LPD_DMA_NUM_CHANNELS) {
        return DMA_ERROR_INVALID_PARAM;
    }

    /* Configure for normal mode, simple pointer type */
    ctrl0 = XLPDDMA_CTRL0_MODE_NORMAL;
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_CTRL0, ctrl0);

    /* Configure data attributes (AXI attributes) */
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_DATA_ATTR, 0x04830483);

    /* Configure descriptor attributes */
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_DSCR_ATTR, 0x04830483);

    if (use_irq) {
        /* Enable done and error interrupts */
        lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_IEN,
                          XLPDDMA_IXR_DMA_DONE | XLPDDMA_IXR_ERR_MASK);
    }

    return DMA_SUCCESS;
}

/*******************************************************************************
 * Transfer Functions
 ******************************************************************************/

int lpd_dma_transfer(uint32_t channel, uint64_t src_addr, uint64_t dst_addr, uint32_t length)
{
    uint32_t ctrl0;

    if (channel >= LPD_DMA_NUM_CHANNELS) {
        return DMA_ERROR_INVALID_PARAM;
    }

    if (!g_LpdDma.channels[channel].initialized) {
        return DMA_ERROR_NOT_INIT;
    }

    /* Check if channel is busy */
    if (lpd_dma_is_busy(channel)) {
        return DMA_ERROR_BUSY;
    }

    /* Flush source, invalidate destination */
    Xil_DCacheFlushRange(src_addr, length);
    Xil_DCacheInvalidateRange(dst_addr, length);

    /* Clear completion flags */
    g_LpdDma.channels[channel].transfer_complete = false;
    g_LpdDma.channels[channel].transfer_error = 0;
    g_LpdDma.channels[channel].busy = true;

    /* Clear any pending interrupts */
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_ISR, XLPDDMA_IXR_ALL_MASK);

    /* Setup source descriptor */
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_SRC_DSCR_WRD0, (uint32_t)(src_addr & 0xFFFFFFFF));
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_SRC_DSCR_WRD1, (uint32_t)(src_addr >> 32));
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_SRC_DSCR_WRD2, length);
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_SRC_DSCR_WRD3, XLPDDMA_DESC_CTRL_INTR_EN);

    /* Setup destination descriptor */
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_DST_DSCR_WRD0, (uint32_t)(dst_addr & 0xFFFFFFFF));
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_DST_DSCR_WRD1, (uint32_t)(dst_addr >> 32));
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_DST_DSCR_WRD2, length);
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_DST_DSCR_WRD3, XLPDDMA_DESC_CTRL_INTR_EN);

    /* Configure for normal mode, simple pointer type */
    ctrl0 = XLPDDMA_CTRL0_MODE_NORMAL;
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_CTRL0, ctrl0);

    /* Start transfer by writing to CTRL1 (enable DMA) */
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_CTRL1, 0);

    /* Memory barrier to ensure all writes are complete */
    __asm__ __volatile__("dsb sy" ::: "memory");

    /* Trigger the transfer by setting start */
    ctrl0 = lpd_dma_read_reg(channel, XLPDDMA_ZDMA_CH_CTRL0);
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_CTRL1, 0);

    return DMA_SUCCESS;
}

int lpd_dma_start_src(uint32_t channel, uint64_t src_addr, uint32_t length)
{
    uint32_t ctrl0;

    if (channel >= LPD_DMA_NUM_CHANNELS) {
        return DMA_ERROR_INVALID_PARAM;
    }

    if (!g_LpdDma.channels[channel].initialized) {
        return DMA_ERROR_NOT_INIT;
    }

    /* Flush source buffer */
    Xil_DCacheFlushRange(src_addr, length);

    /* Clear completion flags */
    g_LpdDma.channels[channel].transfer_complete = false;
    g_LpdDma.channels[channel].busy = true;

    /* Clear interrupts */
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_ISR, XLPDDMA_IXR_ALL_MASK);

    /* Setup source descriptor */
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_SRC_DSCR_WRD0, (uint32_t)(src_addr & 0xFFFFFFFF));
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_SRC_DSCR_WRD1, (uint32_t)(src_addr >> 32));
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_SRC_DSCR_WRD2, length);
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_SRC_DSCR_WRD3, XLPDDMA_DESC_CTRL_INTR_EN);

    /* Configure for read-only mode */
    ctrl0 = XLPDDMA_CTRL0_MODE_RONLY;
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_CTRL0, ctrl0);

    /* Start transfer */
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_CTRL1, 0);

    return DMA_SUCCESS;
}

int lpd_dma_start_dst(uint32_t channel, uint64_t dst_addr, uint32_t length, uint32_t data)
{
    uint32_t ctrl0;

    if (channel >= LPD_DMA_NUM_CHANNELS) {
        return DMA_ERROR_INVALID_PARAM;
    }

    if (!g_LpdDma.channels[channel].initialized) {
        return DMA_ERROR_NOT_INIT;
    }

    /* Invalidate destination buffer */
    Xil_DCacheInvalidateRange(dst_addr, length);

    /* Clear completion flags */
    g_LpdDma.channels[channel].transfer_complete = false;
    g_LpdDma.channels[channel].busy = true;

    /* Clear interrupts */
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_ISR, XLPDDMA_IXR_ALL_MASK);

    /* Setup write-only data word */
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_WR_ONLY_WORD, data);

    /* Setup destination descriptor */
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_DST_DSCR_WRD0, (uint32_t)(dst_addr & 0xFFFFFFFF));
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_DST_DSCR_WRD1, (uint32_t)(dst_addr >> 32));
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_DST_DSCR_WRD2, length);
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_DST_DSCR_WRD3, XLPDDMA_DESC_CTRL_INTR_EN);

    /* Configure for write-only mode */
    ctrl0 = XLPDDMA_CTRL0_MODE_WONLY;
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_CTRL0, ctrl0);

    /* Start transfer */
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_CTRL1, 0);

    return DMA_SUCCESS;
}

/*******************************************************************************
 * Wait Functions
 ******************************************************************************/

int lpd_dma_wait_complete(uint32_t channel, uint32_t timeout_us)
{
    uint64_t start_time = timer_get_us();
    uint32_t status;
    uint32_t isr;

    if (channel >= LPD_DMA_NUM_CHANNELS) {
        return DMA_ERROR_INVALID_PARAM;
    }

    while ((timer_get_us() - start_time) < timeout_us) {
        /* Check interrupt status register */
        isr = lpd_dma_read_reg(channel, XLPDDMA_ZDMA_CH_ISR);

        /* Check for errors */
        if (isr & XLPDDMA_IXR_ERR_MASK) {
            g_LpdDma.channels[channel].transfer_error = isr;
            g_LpdDma.channels[channel].errors++;
            g_LpdDma.channels[channel].busy = false;
            /* Clear interrupt */
            lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_ISR, isr);
            return DMA_ERROR_DMA_FAIL;
        }

        /* Check for completion */
        if (isr & XLPDDMA_IXR_DMA_DONE) {
            g_LpdDma.channels[channel].transfer_complete = true;
            g_LpdDma.channels[channel].num_transfers++;
            g_LpdDma.channels[channel].busy = false;
            /* Clear interrupt */
            lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_ISR, isr);
            return DMA_SUCCESS;
        }

        /* Also check status register for idle */
        status = lpd_dma_read_reg(channel, XLPDDMA_ZDMA_CH_STATUS);
        if ((status & XLPDDMA_STATUS_STATE_MASK) == XLPDDMA_STATUS_STATE_IDLE) {
            /* Channel went idle, check if transfer was successful */
            isr = lpd_dma_read_reg(channel, XLPDDMA_ZDMA_CH_ISR);
            if (!(isr & XLPDDMA_IXR_ERR_MASK)) {
                g_LpdDma.channels[channel].transfer_complete = true;
                g_LpdDma.channels[channel].num_transfers++;
                g_LpdDma.channels[channel].busy = false;
                lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_ISR, isr);
                return DMA_SUCCESS;
            }
        }
    }

    g_LpdDma.channels[channel].busy = false;
    return DMA_ERROR_TIMEOUT;
}

/*******************************************************************************
 * Status Functions
 ******************************************************************************/

bool lpd_dma_is_busy(uint32_t channel)
{
    uint32_t status;

    if (channel >= LPD_DMA_NUM_CHANNELS) {
        return false;
    }

    status = lpd_dma_read_reg(channel, XLPDDMA_ZDMA_CH_STATUS);
    return (status & XLPDDMA_STATUS_STATE_MASK) == XLPDDMA_STATUS_STATE_BUSY;
}

uint32_t lpd_dma_get_status(uint32_t channel)
{
    if (channel >= LPD_DMA_NUM_CHANNELS) {
        return 0;
    }
    return lpd_dma_read_reg(channel, XLPDDMA_ZDMA_CH_STATUS);
}

uint32_t lpd_dma_get_total_bytes(uint32_t channel)
{
    if (channel >= LPD_DMA_NUM_CHANNELS) {
        return 0;
    }
    return lpd_dma_read_reg(channel, XLPDDMA_ZDMA_CH_TOTAL_BYTE);
}

LpdDmaInst_t* lpd_dma_get_instance(void)
{
    return &g_LpdDma;
}

/*******************************************************************************
 * Statistics Functions
 ******************************************************************************/

void lpd_dma_print_stats(void)
{
    uint32_t i;

    LOG_ALWAYS("\r\n=== LPD DMA Statistics ===\r\n");
    LOG_ALWAYS("Total Transfers: %lu\r\n", g_LpdDma.total_transfers);
    LOG_ALWAYS("Total Bytes:     %llu\r\n", g_LpdDma.total_bytes);
    LOG_ALWAYS("Total Errors:    %lu\r\n", g_LpdDma.total_errors);

    LOG_ALWAYS("\r\nPer-Channel Statistics:\r\n");
    for (i = 0; i < LPD_DMA_NUM_CHANNELS; i++) {
        if (g_LpdDma.channels[i].initialized) {
            LOG_ALWAYS("  CH%d: %lu transfers, %llu bytes, %lu errors\r\n",
                       i, g_LpdDma.channels[i].num_transfers,
                       g_LpdDma.channels[i].bytes_transferred,
                       g_LpdDma.channels[i].errors);
        }
    }
    LOG_ALWAYS("==========================\r\n");
}

void lpd_dma_clear_stats(void)
{
    uint32_t i;

    g_LpdDma.total_bytes = 0;
    g_LpdDma.total_transfers = 0;
    g_LpdDma.total_errors = 0;

    for (i = 0; i < LPD_DMA_NUM_CHANNELS; i++) {
        g_LpdDma.channels[i].bytes_transferred = 0;
        g_LpdDma.channels[i].num_transfers = 0;
        g_LpdDma.channels[i].errors = 0;
    }
}

/*******************************************************************************
 * Interrupt Handler
 ******************************************************************************/

void lpd_dma_irq_handler(uint32_t channel)
{
    uint32_t isr;

    if (channel >= LPD_DMA_NUM_CHANNELS) {
        return;
    }

    /* Read and clear interrupt status */
    isr = lpd_dma_read_reg(channel, XLPDDMA_ZDMA_CH_ISR);
    lpd_dma_write_reg(channel, XLPDDMA_ZDMA_CH_ISR, isr);

    /* Handle errors */
    if (isr & XLPDDMA_IXR_ERR_MASK) {
        g_LpdDma.channels[channel].transfer_error = isr;
        g_LpdDma.channels[channel].errors++;
        g_LpdDma.total_errors++;
    }

    /* Handle completion */
    if (isr & XLPDDMA_IXR_DMA_DONE) {
        g_LpdDma.channels[channel].transfer_complete = true;
        g_LpdDma.channels[channel].num_transfers++;
        g_LpdDma.total_transfers++;

        /* Read total bytes transferred */
        g_LpdDma.channels[channel].bytes_transferred +=
            lpd_dma_read_reg(channel, XLPDDMA_ZDMA_CH_TOTAL_BYTE);
    }

    g_LpdDma.channels[channel].busy = false;
}
