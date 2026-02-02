/**
 * @file axi_dma_driver.h
 * @brief AXI DMA Driver Wrapper Header
 *
 * Wrapper for Xilinx AXI DMA IP with Scatter-Gather support.
 */

#ifndef AXI_DMA_DRIVER_H
#define AXI_DMA_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "../dma_benchmark.h"

/*******************************************************************************
 * AXI DMA Register Offsets
 ******************************************************************************/

#define XAXIDMA_CR_OFFSET          0x00  /* Control register */
#define XAXIDMA_SR_OFFSET          0x04  /* Status register */
#define XAXIDMA_CDESC_OFFSET       0x08  /* Current descriptor (low) */
#define XAXIDMA_CDESC_MSB_OFFSET   0x0C  /* Current descriptor (high) */
#define XAXIDMA_TDESC_OFFSET       0x10  /* Tail descriptor (low) */
#define XAXIDMA_TDESC_MSB_OFFSET   0x14  /* Tail descriptor (high) */
#define XAXIDMA_SRCADDR_OFFSET     0x18  /* Source address (simple mode) */
#define XAXIDMA_DSTADDR_OFFSET     0x20  /* Dest address (simple mode) */
#define XAXIDMA_BUFFLEN_OFFSET     0x28  /* Buffer length */

/* MM2S and S2MM channel base offsets */
#define XAXIDMA_TX_OFFSET          0x00
#define XAXIDMA_RX_OFFSET          0x30

/*******************************************************************************
 * AXI DMA Control Register Bits
 ******************************************************************************/

#define XAXIDMA_CR_RUNSTOP_MASK    0x00000001  /* Start/stop DMA */
#define XAXIDMA_CR_RESET_MASK      0x00000004  /* Reset DMA */
#define XAXIDMA_CR_KEYHOLE_MASK    0x00000008  /* Keyhole mode */
#define XAXIDMA_CR_CYCLIC_MASK     0x00000010  /* Cyclic mode */
#define XAXIDMA_CR_IOC_IRQ_EN      0x00001000  /* Interrupt on Complete */
#define XAXIDMA_CR_DLY_IRQ_EN      0x00002000  /* Delay interrupt */
#define XAXIDMA_CR_ERR_IRQ_EN      0x00004000  /* Error interrupt */
#define XAXIDMA_CR_ALL_IRQ_EN      0x00007000  /* All interrupts */

/*******************************************************************************
 * AXI DMA Status Register Bits
 ******************************************************************************/

#define XAXIDMA_SR_HALTED_MASK     0x00000001  /* DMA is halted */
#define XAXIDMA_SR_IDLE_MASK       0x00000002  /* DMA is idle */
#define XAXIDMA_SR_SGINCL_MASK     0x00000008  /* SG included */
#define XAXIDMA_SR_DMAINTERR_MASK  0x00000010  /* DMA internal error */
#define XAXIDMA_SR_DMASLVERR_MASK  0x00000020  /* DMA slave error */
#define XAXIDMA_SR_DMADECERR_MASK  0x00000040  /* DMA decode error */
#define XAXIDMA_SR_SGINTERR_MASK   0x00000100  /* SG internal error */
#define XAXIDMA_SR_SGSLVERR_MASK   0x00000200  /* SG slave error */
#define XAXIDMA_SR_SGDECERR_MASK   0x00000400  /* SG decode error */
#define XAXIDMA_SR_IOC_IRQ_MASK    0x00001000  /* IOC interrupt */
#define XAXIDMA_SR_DLY_IRQ_MASK    0x00002000  /* Delay interrupt */
#define XAXIDMA_SR_ERR_IRQ_MASK    0x00004000  /* Error interrupt */
#define XAXIDMA_SR_ALL_ERR_MASK    0x00000770  /* All error bits */
#define XAXIDMA_SR_ALL_IRQ_MASK    0x00007000  /* All interrupt bits */

/*******************************************************************************
 * AXI DMA Scatter-Gather Descriptor
 ******************************************************************************/

typedef struct __attribute__((aligned(64))) {
    uint32_t next_desc;        /* 0x00: Next descriptor pointer (low) */
    uint32_t next_desc_msb;    /* 0x04: Next descriptor pointer (high) */
    uint32_t buffer_addr;      /* 0x08: Buffer address (low) */
    uint32_t buffer_addr_msb;  /* 0x0C: Buffer address (high) */
    uint32_t reserved1[2];     /* 0x10-0x14: Reserved */
    uint32_t control;          /* 0x18: Control word */
    uint32_t status;           /* 0x1C: Status word */
    uint32_t app[5];           /* 0x20-0x30: Application words */
    uint32_t reserved2[3];     /* 0x34-0x3C: Padding to 64 bytes */
} AxiDmaSgDesc_t;

/* Descriptor control bits */
#define XAXIDMA_BD_CTRL_TXSOF_MASK  0x08000000  /* Start of frame */
#define XAXIDMA_BD_CTRL_TXEOF_MASK  0x04000000  /* End of frame */
#define XAXIDMA_BD_CTRL_LENGTH_MASK 0x03FFFFFF  /* Transfer length */

/* Descriptor status bits */
#define XAXIDMA_BD_STS_COMPLETE_MASK 0x80000000 /* Completed */
#define XAXIDMA_BD_STS_RXSOF_MASK    0x08000000 /* RX Start of frame */
#define XAXIDMA_BD_STS_RXEOF_MASK    0x04000000 /* RX End of frame */
#define XAXIDMA_BD_STS_ALL_ERR_MASK  0x70000000 /* All error bits */

/*******************************************************************************
 * AXI DMA Instance Structure
 ******************************************************************************/

typedef struct {
    uint64_t base_addr;
    bool initialized;
    bool sg_mode;
    bool has_mm2s;
    bool has_s2mm;
    uint32_t data_width;
    uint32_t addr_width;
    uint32_t max_transfer_len;

    /* Descriptor rings */
    AxiDmaSgDesc_t* tx_ring;
    AxiDmaSgDesc_t* rx_ring;
    uint32_t ring_size;
    uint32_t tx_head;
    uint32_t tx_tail;
    uint32_t rx_head;
    uint32_t rx_tail;

    /* Transfer state */
    volatile bool tx_complete;
    volatile bool rx_complete;
    volatile uint32_t tx_error;
    volatile uint32_t rx_error;

    /* Statistics */
    uint64_t tx_bytes;
    uint64_t rx_bytes;
    uint32_t tx_transfers;
    uint32_t rx_transfers;
    uint32_t errors;
} AxiDmaInst_t;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize AXI DMA driver
 * @return 0 on success, negative error code on failure
 */
int axi_dma_init(void);

/**
 * @brief Cleanup AXI DMA driver
 */
void axi_dma_cleanup(void);

/**
 * @brief Reset AXI DMA
 * @return 0 on success, negative error code on failure
 */
int axi_dma_reset(void);

/**
 * @brief Configure AXI DMA for operation
 * @param use_sg Enable Scatter-Gather mode
 * @param use_irq Enable interrupts
 * @return 0 on success, negative error code on failure
 */
int axi_dma_configure(bool use_sg, bool use_irq);

/**
 * @brief Setup SG descriptor ring
 * @param tx_descs Pointer to TX descriptor memory
 * @param rx_descs Pointer to RX descriptor memory
 * @param num_descs Number of descriptors in each ring
 * @return 0 on success, negative error code on failure
 */
int axi_dma_setup_sg_ring(AxiDmaSgDesc_t* tx_descs, AxiDmaSgDesc_t* rx_descs, uint32_t num_descs);

/**
 * @brief Start a simple (non-SG) transfer
 * @param src_addr Source address
 * @param dst_addr Destination address (for loopback)
 * @param length Transfer length in bytes
 * @return 0 on success, negative error code on failure
 */
int axi_dma_simple_transfer(uint64_t src_addr, uint64_t dst_addr, uint32_t length);

/**
 * @brief Start a Scatter-Gather transfer
 * @param src_addr Source buffer address
 * @param dst_addr Destination buffer address
 * @param length Transfer length in bytes
 * @return 0 on success, negative error code on failure
 */
int axi_dma_sg_transfer(uint64_t src_addr, uint64_t dst_addr, uint32_t length);

/**
 * @brief Start MM2S (TX) transfer
 * @param buffer_addr Source buffer address
 * @param length Transfer length
 * @return 0 on success, negative error code on failure
 */
int axi_dma_start_tx(uint64_t buffer_addr, uint32_t length);

/**
 * @brief Start S2MM (RX) transfer
 * @param buffer_addr Destination buffer address
 * @param length Transfer length
 * @return 0 on success, negative error code on failure
 */
int axi_dma_start_rx(uint64_t buffer_addr, uint32_t length);

/**
 * @brief Wait for TX transfer completion (polling)
 * @param timeout_us Timeout in microseconds
 * @return 0 on success, negative error code on failure/timeout
 */
int axi_dma_wait_tx(uint32_t timeout_us);

/**
 * @brief Wait for RX transfer completion (polling)
 * @param timeout_us Timeout in microseconds
 * @return 0 on success, negative error code on failure/timeout
 */
int axi_dma_wait_rx(uint32_t timeout_us);

/**
 * @brief Wait for both TX and RX completion
 * @param timeout_us Timeout in microseconds
 * @return 0 on success, negative error code on failure/timeout
 */
int axi_dma_wait_complete(uint32_t timeout_us);

/**
 * @brief Get TX channel status
 * @return Status register value
 */
uint32_t axi_dma_get_tx_status(void);

/**
 * @brief Get RX channel status
 * @return Status register value
 */
uint32_t axi_dma_get_rx_status(void);

/**
 * @brief Check if TX is busy
 * @return true if busy, false if idle
 */
bool axi_dma_tx_busy(void);

/**
 * @brief Check if RX is busy
 * @return true if busy, false if idle
 */
bool axi_dma_rx_busy(void);

/**
 * @brief Get AXI DMA instance pointer
 * @return Pointer to AXI DMA instance
 */
AxiDmaInst_t* axi_dma_get_instance(void);

/**
 * @brief Print AXI DMA statistics
 */
void axi_dma_print_stats(void);

/**
 * @brief Clear AXI DMA statistics
 */
void axi_dma_clear_stats(void);

/**
 * @brief AXI DMA interrupt handler
 */
void axi_dma_irq_handler(void);

#endif /* AXI_DMA_DRIVER_H */
