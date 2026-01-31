/**
 * @file axi_dma_driver.c
 * @brief AXI DMA Driver Wrapper Implementation
 *
 * Wrapper for Xilinx AXI DMA IP with Scatter-Gather support.
 */

#include <string.h>
#include "xil_io.h"
#include "xil_cache.h"
#include "sleep.h"
#include "axi_dma_driver.h"
#include "../platform_config.h"
#include "../utils/timer_utils.h"
#include "../utils/debug_print.h"

/*******************************************************************************
 * Local Variables
 ******************************************************************************/

static AxiDmaInst_t g_AxiDma = {0};

/* Descriptor memory (aligned to 64 bytes) */
static AxiDmaSgDesc_t g_TxDescRing[MAX_SG_DESCRIPTORS] __attribute__((aligned(64)));
static AxiDmaSgDesc_t g_RxDescRing[MAX_SG_DESCRIPTORS] __attribute__((aligned(64)));

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

static inline void axi_dma_write_reg(uint32_t offset, uint32_t value)
{
    Xil_Out32(g_AxiDma.base_addr + offset, value);
}

static inline uint32_t axi_dma_read_reg(uint32_t offset)
{
    return Xil_In32(g_AxiDma.base_addr + offset);
}

static inline void axi_dma_write_tx_reg(uint32_t offset, uint32_t value)
{
    Xil_Out32(g_AxiDma.base_addr + XAXIDMA_TX_OFFSET + offset, value);
}

static inline uint32_t axi_dma_read_tx_reg(uint32_t offset)
{
    return Xil_In32(g_AxiDma.base_addr + XAXIDMA_TX_OFFSET + offset);
}

static inline void axi_dma_write_rx_reg(uint32_t offset, uint32_t value)
{
    Xil_Out32(g_AxiDma.base_addr + XAXIDMA_RX_OFFSET + offset, value);
}

static inline uint32_t axi_dma_read_rx_reg(uint32_t offset)
{
    return Xil_In32(g_AxiDma.base_addr + XAXIDMA_RX_OFFSET + offset);
}

/*******************************************************************************
 * Initialization Functions
 ******************************************************************************/

int axi_dma_init(void)
{
    uint32_t tx_status, rx_status;

    LOG_ALWAYS("AXI DMA: Initializing at 0x%08llX\r\n", (unsigned long long)AXI_DMA_BASE_ADDR);

    /* Initialize instance structure */
    memset(&g_AxiDma, 0, sizeof(g_AxiDma));
    g_AxiDma.base_addr = AXI_DMA_BASE_ADDR;
    g_AxiDma.data_width = AXI_DMA_DATA_WIDTH;
    g_AxiDma.addr_width = AXI_DMA_ADDR_WIDTH;
    g_AxiDma.max_transfer_len = AXI_DMA_MAX_TRANSFER_SIZE;

    LOG_ALWAYS("AXI DMA: Max transfer len = %lu bytes\r\n", (unsigned long)g_AxiDma.max_transfer_len);

    /* Read and display status registers */
    tx_status = axi_dma_read_tx_reg(XAXIDMA_SR_OFFSET);
    rx_status = axi_dma_read_rx_reg(XAXIDMA_SR_OFFSET);

    LOG_ALWAYS("AXI DMA: TX Status = 0x%08lX, RX Status = 0x%08lX\r\n",
               (unsigned long)tx_status, (unsigned long)rx_status);

    /* Check SG mode support */
    g_AxiDma.sg_mode = (tx_status & XAXIDMA_SR_SGINCL_MASK) ? true : false;
    g_AxiDma.has_mm2s = true;
    g_AxiDma.has_s2mm = true;

    LOG_ALWAYS("AXI DMA: SG Mode = %s\r\n", g_AxiDma.sg_mode ? "Yes" : "No");
    LOG_ALWAYS("AXI DMA: MM2S = %s, S2MM = %s\r\n",
              g_AxiDma.has_mm2s ? "Yes" : "No",
              g_AxiDma.has_s2mm ? "Yes" : "No");

    /* Check for halted state (indicates IP might not be accessible) */
    if (tx_status & XAXIDMA_SR_HALTED_MASK) {
        LOG_ALWAYS("AXI DMA: TX channel is halted (normal before start)\r\n");
    }
    if (rx_status & XAXIDMA_SR_HALTED_MASK) {
        LOG_ALWAYS("AXI DMA: RX channel is halted (normal before start)\r\n");
    }

    /* Check for any pre-existing errors */
    if (tx_status & XAXIDMA_SR_ALL_ERR_MASK) {
        LOG_ERROR("AXI DMA: TX has pre-existing errors: 0x%03lX\r\n",
                  (unsigned long)(tx_status & XAXIDMA_SR_ALL_ERR_MASK));
    }
    if (rx_status & XAXIDMA_SR_ALL_ERR_MASK) {
        LOG_ERROR("AXI DMA: RX has pre-existing errors: 0x%03lX\r\n",
                  (unsigned long)(rx_status & XAXIDMA_SR_ALL_ERR_MASK));
    }

    /* Reset DMA */
    if (axi_dma_reset() != 0) {
        LOG_ERROR("AXI DMA: Reset failed\r\n");
        return DMA_ERROR_DMA_FAIL;
    }

    /* Setup SG descriptor rings if SG mode */
    if (g_AxiDma.sg_mode) {
        LOG_ALWAYS("AXI DMA: Setting up SG rings at TX=0x%p, RX=0x%p\r\n",
                   (void*)g_TxDescRing, (void*)g_RxDescRing);
        if (axi_dma_setup_sg_ring(g_TxDescRing, g_RxDescRing, MAX_SG_DESCRIPTORS) != 0) {
            LOG_ERROR("AXI DMA: SG ring setup failed\r\n");
            return DMA_ERROR_DMA_FAIL;
        }
    }

    g_AxiDma.initialized = true;
    LOG_ALWAYS("AXI DMA: Initialization complete\r\n");
    return DMA_SUCCESS;
}

void axi_dma_cleanup(void)
{
    if (!g_AxiDma.initialized) {
        return;
    }

    /* Stop DMA channels */
    axi_dma_write_tx_reg(XAXIDMA_CR_OFFSET, 0);
    axi_dma_write_rx_reg(XAXIDMA_CR_OFFSET, 0);

    /* Reset DMA */
    axi_dma_reset();

    g_AxiDma.initialized = false;
    LOG_DEBUG("AXI DMA: Cleanup complete\r\n");
}

int axi_dma_reset(void)
{
    uint32_t timeout = 1000;
    uint32_t status;

    /* Reset MM2S channel */
    axi_dma_write_tx_reg(XAXIDMA_CR_OFFSET, XAXIDMA_CR_RESET_MASK);

    /* Wait for reset to complete */
    while (timeout > 0) {
        status = axi_dma_read_tx_reg(XAXIDMA_CR_OFFSET);
        if (!(status & XAXIDMA_CR_RESET_MASK)) {
            break;
        }
        usleep(1);
        timeout--;
    }

    if (timeout == 0) {
        LOG_ERROR("AXI DMA: TX reset timeout\r\n");
        return DMA_ERROR_TIMEOUT;
    }

    /* Reset S2MM channel */
    timeout = 1000;
    axi_dma_write_rx_reg(XAXIDMA_CR_OFFSET, XAXIDMA_CR_RESET_MASK);

    while (timeout > 0) {
        status = axi_dma_read_rx_reg(XAXIDMA_CR_OFFSET);
        if (!(status & XAXIDMA_CR_RESET_MASK)) {
            break;
        }
        usleep(1);
        timeout--;
    }

    if (timeout == 0) {
        LOG_ERROR("AXI DMA: RX reset timeout\r\n");
        return DMA_ERROR_TIMEOUT;
    }

    /* Clear completion flags */
    g_AxiDma.tx_complete = false;
    g_AxiDma.rx_complete = false;
    g_AxiDma.tx_error = 0;
    g_AxiDma.rx_error = 0;

    return DMA_SUCCESS;
}

int axi_dma_configure(bool use_sg, bool use_irq)
{
    uint32_t cr_value = 0;

    if (!g_AxiDma.initialized) {
        return DMA_ERROR_NOT_INIT;
    }

    /* Configure control register */
    if (use_irq) {
        cr_value |= XAXIDMA_CR_IOC_IRQ_EN | XAXIDMA_CR_ERR_IRQ_EN;
    }

    /* Configure TX channel */
    axi_dma_write_tx_reg(XAXIDMA_CR_OFFSET, cr_value);

    /* Configure RX channel */
    axi_dma_write_rx_reg(XAXIDMA_CR_OFFSET, cr_value);

    return DMA_SUCCESS;
}

int axi_dma_setup_sg_ring(AxiDmaSgDesc_t* tx_descs, AxiDmaSgDesc_t* rx_descs, uint32_t num_descs)
{
    uint32_t i;
    uint64_t desc_addr;

    if (!tx_descs || !rx_descs || num_descs == 0) {
        return DMA_ERROR_INVALID_PARAM;
    }

    g_AxiDma.tx_ring = tx_descs;
    g_AxiDma.rx_ring = rx_descs;
    g_AxiDma.ring_size = num_descs;

    /* Initialize TX descriptor ring */
    memset(tx_descs, 0, num_descs * sizeof(AxiDmaSgDesc_t));
    for (i = 0; i < num_descs; i++) {
        desc_addr = (uint64_t)&tx_descs[(i + 1) % num_descs];
        tx_descs[i].next_desc = (uint32_t)(desc_addr & 0xFFFFFFFF);
        tx_descs[i].next_desc_msb = (uint32_t)(desc_addr >> 32);
    }

    /* Initialize RX descriptor ring */
    memset(rx_descs, 0, num_descs * sizeof(AxiDmaSgDesc_t));
    for (i = 0; i < num_descs; i++) {
        desc_addr = (uint64_t)&rx_descs[(i + 1) % num_descs];
        rx_descs[i].next_desc = (uint32_t)(desc_addr & 0xFFFFFFFF);
        rx_descs[i].next_desc_msb = (uint32_t)(desc_addr >> 32);
    }

    /* Flush descriptor memory */
    Xil_DCacheFlushRange((UINTPTR)tx_descs, num_descs * sizeof(AxiDmaSgDesc_t));
    Xil_DCacheFlushRange((UINTPTR)rx_descs, num_descs * sizeof(AxiDmaSgDesc_t));

    g_AxiDma.tx_head = 0;
    g_AxiDma.tx_tail = 0;
    g_AxiDma.rx_head = 0;
    g_AxiDma.rx_tail = 0;

    return DMA_SUCCESS;
}

/*******************************************************************************
 * Transfer Functions
 ******************************************************************************/

int axi_dma_simple_transfer(uint64_t src_addr, uint64_t dst_addr, uint32_t length)
{
    int status;

    LOG_DEBUG("AXI DMA Simple: src=0x%llX, dst=0x%llX, len=%lu\r\n",
              (unsigned long long)src_addr, (unsigned long long)dst_addr, (unsigned long)length);

    if (!g_AxiDma.initialized) {
        LOG_ERROR("AXI DMA Simple: Not initialized!\r\n");
        return DMA_ERROR_NOT_INIT;
    }

    if (length > g_AxiDma.max_transfer_len) {
        LOG_ERROR("AXI DMA Simple: Length %lu exceeds max %lu\r\n",
                  (unsigned long)length, (unsigned long)g_AxiDma.max_transfer_len);
        return DMA_ERROR_INVALID_PARAM;
    }

    /* Start RX first (to receive data) */
    status = axi_dma_start_rx(dst_addr, length);
    if (status != DMA_SUCCESS) {
        return status;
    }

    /* Start TX (to send data through loopback) */
    status = axi_dma_start_tx(src_addr, length);
    if (status != DMA_SUCCESS) {
        return status;
    }

    return DMA_SUCCESS;
}

int axi_dma_sg_transfer(uint64_t src_addr, uint64_t dst_addr, uint32_t length)
{
    uint64_t desc_addr;
    AxiDmaSgDesc_t* tx_desc;
    AxiDmaSgDesc_t* rx_desc;
    uint32_t cr_value;

    LOG_DEBUG("AXI DMA SG: src=0x%llX, dst=0x%llX, len=%lu\r\n",
              (unsigned long long)src_addr, (unsigned long long)dst_addr, (unsigned long)length);

    if (!g_AxiDma.initialized) {
        LOG_ERROR("AXI DMA SG: Not initialized!\r\n");
        return DMA_ERROR_NOT_INIT;
    }

    if (!g_AxiDma.sg_mode) {
        LOG_ERROR("AXI DMA SG: SG mode not enabled in hardware!\r\n");
        return DMA_ERROR_NOT_INIT;
    }

    if (length > g_AxiDma.max_transfer_len) {
        LOG_ERROR("AXI DMA SG: Length %lu exceeds max %lu\r\n",
                  (unsigned long)length, (unsigned long)g_AxiDma.max_transfer_len);
        return DMA_ERROR_INVALID_PARAM;
    }

    /* Setup TX descriptor */
    tx_desc = &g_AxiDma.tx_ring[g_AxiDma.tx_head];
    tx_desc->buffer_addr = (uint32_t)(src_addr & 0xFFFFFFFF);
    tx_desc->buffer_addr_msb = (uint32_t)(src_addr >> 32);
    tx_desc->control = XAXIDMA_BD_CTRL_TXSOF_MASK | XAXIDMA_BD_CTRL_TXEOF_MASK | length;
    tx_desc->status = 0;

    LOG_DEBUG("AXI DMA SG: TX desc at 0x%p, buf=0x%08lX:%08lX, ctrl=0x%08lX\r\n",
              (void*)tx_desc, (unsigned long)tx_desc->buffer_addr_msb,
              (unsigned long)tx_desc->buffer_addr, (unsigned long)tx_desc->control);

    /* Setup RX descriptor */
    rx_desc = &g_AxiDma.rx_ring[g_AxiDma.rx_head];
    rx_desc->buffer_addr = (uint32_t)(dst_addr & 0xFFFFFFFF);
    rx_desc->buffer_addr_msb = (uint32_t)(dst_addr >> 32);
    rx_desc->control = length;
    rx_desc->status = 0;

    LOG_DEBUG("AXI DMA SG: RX desc at 0x%p, buf=0x%08lX:%08lX, ctrl=0x%08lX\r\n",
              (void*)rx_desc, (unsigned long)rx_desc->buffer_addr_msb,
              (unsigned long)rx_desc->buffer_addr, (unsigned long)rx_desc->control);

    /* Flush descriptors */
    Xil_DCacheFlushRange((UINTPTR)tx_desc, sizeof(AxiDmaSgDesc_t));
    Xil_DCacheFlushRange((UINTPTR)rx_desc, sizeof(AxiDmaSgDesc_t));

    /* Flush source buffer, invalidate destination buffer */
    Xil_DCacheFlushRange(src_addr, length);
    Xil_DCacheInvalidateRange(dst_addr, length);

    /* Clear completion flags */
    g_AxiDma.tx_complete = false;
    g_AxiDma.rx_complete = false;

    /* Start RX channel */
    desc_addr = (uint64_t)rx_desc;
    axi_dma_write_rx_reg(XAXIDMA_CDESC_OFFSET, (uint32_t)(desc_addr & 0xFFFFFFFF));
    axi_dma_write_rx_reg(XAXIDMA_CDESC_MSB_OFFSET, (uint32_t)(desc_addr >> 32));

    cr_value = axi_dma_read_rx_reg(XAXIDMA_CR_OFFSET);
    axi_dma_write_rx_reg(XAXIDMA_CR_OFFSET, cr_value | XAXIDMA_CR_RUNSTOP_MASK);

    axi_dma_write_rx_reg(XAXIDMA_TDESC_OFFSET, (uint32_t)(desc_addr & 0xFFFFFFFF));
    axi_dma_write_rx_reg(XAXIDMA_TDESC_MSB_OFFSET, (uint32_t)(desc_addr >> 32));

    /* Start TX channel */
    desc_addr = (uint64_t)tx_desc;
    axi_dma_write_tx_reg(XAXIDMA_CDESC_OFFSET, (uint32_t)(desc_addr & 0xFFFFFFFF));
    axi_dma_write_tx_reg(XAXIDMA_CDESC_MSB_OFFSET, (uint32_t)(desc_addr >> 32));

    cr_value = axi_dma_read_tx_reg(XAXIDMA_CR_OFFSET);
    axi_dma_write_tx_reg(XAXIDMA_CR_OFFSET, cr_value | XAXIDMA_CR_RUNSTOP_MASK);

    axi_dma_write_tx_reg(XAXIDMA_TDESC_OFFSET, (uint32_t)(desc_addr & 0xFFFFFFFF));
    axi_dma_write_tx_reg(XAXIDMA_TDESC_MSB_OFFSET, (uint32_t)(desc_addr >> 32));

    /* Debug: check status immediately after starting */
    {
        uint32_t tx_sr = axi_dma_read_tx_reg(XAXIDMA_SR_OFFSET);
        uint32_t rx_sr = axi_dma_read_rx_reg(XAXIDMA_SR_OFFSET);
        LOG_DEBUG("AXI DMA SG: After start TX_SR=0x%08lX, RX_SR=0x%08lX\r\n",
                  (unsigned long)tx_sr, (unsigned long)rx_sr);
    }

    /* Update ring indices */
    g_AxiDma.tx_head = (g_AxiDma.tx_head + 1) % g_AxiDma.ring_size;
    g_AxiDma.rx_head = (g_AxiDma.rx_head + 1) % g_AxiDma.ring_size;

    return DMA_SUCCESS;
}

int axi_dma_start_tx(uint64_t buffer_addr, uint32_t length)
{
    uint32_t cr_value;

    if (!g_AxiDma.initialized) {
        return DMA_ERROR_NOT_INIT;
    }

    /* Flush buffer from cache */
    Xil_DCacheFlushRange(buffer_addr, length);

    /* Set source address */
    axi_dma_write_tx_reg(XAXIDMA_SRCADDR_OFFSET, (uint32_t)(buffer_addr & 0xFFFFFFFF));
    axi_dma_write_tx_reg(XAXIDMA_SRCADDR_OFFSET + 4, (uint32_t)(buffer_addr >> 32));

    /* Clear completion flag */
    g_AxiDma.tx_complete = false;

    /* Start DMA */
    cr_value = axi_dma_read_tx_reg(XAXIDMA_CR_OFFSET);
    axi_dma_write_tx_reg(XAXIDMA_CR_OFFSET, cr_value | XAXIDMA_CR_RUNSTOP_MASK);

    /* Set transfer length (starts the transfer) */
    axi_dma_write_tx_reg(XAXIDMA_BUFFLEN_OFFSET, length);

    return DMA_SUCCESS;
}

int axi_dma_start_rx(uint64_t buffer_addr, uint32_t length)
{
    uint32_t cr_value;

    if (!g_AxiDma.initialized) {
        return DMA_ERROR_NOT_INIT;
    }

    /* Invalidate destination buffer */
    Xil_DCacheInvalidateRange(buffer_addr, length);

    /* Set destination address */
    axi_dma_write_rx_reg(XAXIDMA_DSTADDR_OFFSET, (uint32_t)(buffer_addr & 0xFFFFFFFF));
    axi_dma_write_rx_reg(XAXIDMA_DSTADDR_OFFSET + 4, (uint32_t)(buffer_addr >> 32));

    /* Clear completion flag */
    g_AxiDma.rx_complete = false;

    /* Start DMA */
    cr_value = axi_dma_read_rx_reg(XAXIDMA_CR_OFFSET);
    axi_dma_write_rx_reg(XAXIDMA_CR_OFFSET, cr_value | XAXIDMA_CR_RUNSTOP_MASK);

    /* Set transfer length (starts the transfer) */
    axi_dma_write_rx_reg(XAXIDMA_BUFFLEN_OFFSET, length);

    return DMA_SUCCESS;
}

/*******************************************************************************
 * Wait Functions
 ******************************************************************************/

int axi_dma_wait_tx(uint32_t timeout_us)
{
    uint32_t status;
    uint32_t elapsed_us = 0;
    uint32_t poll_interval_us = 10;  /* 10us poll interval */
    uint32_t loop_count = 0;

    LOG_DEBUG("AXI DMA: Wait TX, timeout=%lu us\r\n", (unsigned long)timeout_us);

    while (elapsed_us < timeout_us) {
        status = axi_dma_read_tx_reg(XAXIDMA_SR_OFFSET);
        loop_count++;

        /* Check for errors */
        if (status & XAXIDMA_SR_ALL_ERR_MASK) {
            g_AxiDma.tx_error = status & XAXIDMA_SR_ALL_ERR_MASK;
            g_AxiDma.errors++;
            LOG_ERROR("AXI DMA TX Error: status=0x%08lX, err_bits=0x%03lX (loops=%lu)\r\n",
                      (unsigned long)status, (unsigned long)(status & XAXIDMA_SR_ALL_ERR_MASK),
                      (unsigned long)loop_count);
            /* Clear error bits */
            axi_dma_write_tx_reg(XAXIDMA_SR_OFFSET, status & XAXIDMA_SR_ALL_ERR_MASK);
            return DMA_ERROR_DMA_FAIL;
        }

        /* Check for completion (idle) */
        if (status & XAXIDMA_SR_IDLE_MASK) {
            g_AxiDma.tx_complete = true;
            g_AxiDma.tx_transfers++;
            LOG_DEBUG("AXI DMA TX: Complete (idle), loops=%lu\r\n", (unsigned long)loop_count);
            return DMA_SUCCESS;
        }

        /* Check IOC interrupt status */
        if (status & XAXIDMA_SR_IOC_IRQ_MASK) {
            /* Clear interrupt */
            axi_dma_write_tx_reg(XAXIDMA_SR_OFFSET, XAXIDMA_SR_IOC_IRQ_MASK);
            g_AxiDma.tx_complete = true;
            g_AxiDma.tx_transfers++;
            LOG_DEBUG("AXI DMA TX: Complete (IOC), loops=%lu\r\n", (unsigned long)loop_count);
            return DMA_SUCCESS;
        }

        /* Use usleep for reliable timing */
        usleep(poll_interval_us);
        elapsed_us += poll_interval_us;
    }

    status = axi_dma_read_tx_reg(XAXIDMA_SR_OFFSET);
    LOG_ERROR("AXI DMA TX Timeout: final_status=0x%08lX, loops=%lu, elapsed=%lu us\r\n",
              (unsigned long)status, (unsigned long)loop_count, (unsigned long)elapsed_us);
    return DMA_ERROR_TIMEOUT;
}

int axi_dma_wait_rx(uint32_t timeout_us)
{
    uint32_t status;
    uint32_t elapsed_us = 0;
    uint32_t poll_interval_us = 10;  /* 10us poll interval */
    uint32_t loop_count = 0;

    LOG_DEBUG("AXI DMA: Wait RX, timeout=%lu us\r\n", (unsigned long)timeout_us);

    while (elapsed_us < timeout_us) {
        status = axi_dma_read_rx_reg(XAXIDMA_SR_OFFSET);
        loop_count++;

        /* Check for errors */
        if (status & XAXIDMA_SR_ALL_ERR_MASK) {
            g_AxiDma.rx_error = status & XAXIDMA_SR_ALL_ERR_MASK;
            g_AxiDma.errors++;
            LOG_ERROR("AXI DMA RX Error: status=0x%08lX, err_bits=0x%03lX (loops=%lu)\r\n",
                      (unsigned long)status, (unsigned long)(status & XAXIDMA_SR_ALL_ERR_MASK),
                      (unsigned long)loop_count);
            /* Clear error bits */
            axi_dma_write_rx_reg(XAXIDMA_SR_OFFSET, status & XAXIDMA_SR_ALL_ERR_MASK);
            return DMA_ERROR_DMA_FAIL;
        }

        /* Check for completion (idle) */
        if (status & XAXIDMA_SR_IDLE_MASK) {
            g_AxiDma.rx_complete = true;
            g_AxiDma.rx_transfers++;
            LOG_DEBUG("AXI DMA RX: Complete (idle), loops=%lu\r\n", (unsigned long)loop_count);
            return DMA_SUCCESS;
        }

        /* Check IOC interrupt status */
        if (status & XAXIDMA_SR_IOC_IRQ_MASK) {
            /* Clear interrupt */
            axi_dma_write_rx_reg(XAXIDMA_SR_OFFSET, XAXIDMA_SR_IOC_IRQ_MASK);
            g_AxiDma.rx_complete = true;
            g_AxiDma.rx_transfers++;
            LOG_DEBUG("AXI DMA RX: Complete (IOC), loops=%lu\r\n", (unsigned long)loop_count);
            return DMA_SUCCESS;
        }

        /* Use usleep for reliable timing */
        usleep(poll_interval_us);
        elapsed_us += poll_interval_us;
    }

    status = axi_dma_read_rx_reg(XAXIDMA_SR_OFFSET);
    LOG_ERROR("AXI DMA RX Timeout: final_status=0x%08lX, loops=%lu, elapsed=%lu us\r\n",
              (unsigned long)status, (unsigned long)loop_count, (unsigned long)elapsed_us);
    return DMA_ERROR_TIMEOUT;
}

int axi_dma_wait_complete(uint32_t timeout_us)
{
    int status;

    /* Wait for both channels */
    status = axi_dma_wait_tx(timeout_us);
    if (status != DMA_SUCCESS) {
        return status;
    }

    status = axi_dma_wait_rx(timeout_us);
    return status;
}

/*******************************************************************************
 * Status Functions
 ******************************************************************************/

uint32_t axi_dma_get_tx_status(void)
{
    return axi_dma_read_tx_reg(XAXIDMA_SR_OFFSET);
}

uint32_t axi_dma_get_rx_status(void)
{
    return axi_dma_read_rx_reg(XAXIDMA_SR_OFFSET);
}

bool axi_dma_tx_busy(void)
{
    uint32_t status = axi_dma_get_tx_status();
    return !(status & XAXIDMA_SR_IDLE_MASK);
}

bool axi_dma_rx_busy(void)
{
    uint32_t status = axi_dma_get_rx_status();
    return !(status & XAXIDMA_SR_IDLE_MASK);
}

AxiDmaInst_t* axi_dma_get_instance(void)
{
    return &g_AxiDma;
}

/*******************************************************************************
 * Statistics Functions
 ******************************************************************************/

void axi_dma_print_stats(void)
{
    LOG_ALWAYS("\r\n=== AXI DMA Statistics ===\r\n");
    LOG_ALWAYS("TX Transfers: %lu\r\n", g_AxiDma.tx_transfers);
    LOG_ALWAYS("RX Transfers: %lu\r\n", g_AxiDma.rx_transfers);
    LOG_ALWAYS("TX Bytes:     %llu\r\n", g_AxiDma.tx_bytes);
    LOG_ALWAYS("RX Bytes:     %llu\r\n", g_AxiDma.rx_bytes);
    LOG_ALWAYS("Errors:       %lu\r\n", g_AxiDma.errors);
    LOG_ALWAYS("==========================\r\n");
}

void axi_dma_clear_stats(void)
{
    g_AxiDma.tx_bytes = 0;
    g_AxiDma.rx_bytes = 0;
    g_AxiDma.tx_transfers = 0;
    g_AxiDma.rx_transfers = 0;
    g_AxiDma.errors = 0;
}

/*******************************************************************************
 * Interrupt Handler
 ******************************************************************************/

void axi_dma_irq_handler(void)
{
    uint32_t tx_status, rx_status;

    /* Read status registers */
    tx_status = axi_dma_read_tx_reg(XAXIDMA_SR_OFFSET);
    rx_status = axi_dma_read_rx_reg(XAXIDMA_SR_OFFSET);

    /* Handle TX interrupts */
    if (tx_status & XAXIDMA_SR_ALL_IRQ_MASK) {
        /* Clear interrupt bits */
        axi_dma_write_tx_reg(XAXIDMA_SR_OFFSET, tx_status & XAXIDMA_SR_ALL_IRQ_MASK);

        if (tx_status & XAXIDMA_SR_ERR_IRQ_MASK) {
            g_AxiDma.tx_error = tx_status & XAXIDMA_SR_ALL_ERR_MASK;
            g_AxiDma.errors++;
        }

        if (tx_status & XAXIDMA_SR_IOC_IRQ_MASK) {
            g_AxiDma.tx_complete = true;
            g_AxiDma.tx_transfers++;
        }
    }

    /* Handle RX interrupts */
    if (rx_status & XAXIDMA_SR_ALL_IRQ_MASK) {
        /* Clear interrupt bits */
        axi_dma_write_rx_reg(XAXIDMA_SR_OFFSET, rx_status & XAXIDMA_SR_ALL_IRQ_MASK);

        if (rx_status & XAXIDMA_SR_ERR_IRQ_MASK) {
            g_AxiDma.rx_error = rx_status & XAXIDMA_SR_ALL_ERR_MASK;
            g_AxiDma.errors++;
        }

        if (rx_status & XAXIDMA_SR_IOC_IRQ_MASK) {
            g_AxiDma.rx_complete = true;
            g_AxiDma.rx_transfers++;
        }
    }
}
