/**
 * @file axi_cdma_driver.h
 * @brief AXI CDMA Driver Wrapper Header
 *
 * Wrapper for Xilinx AXI Central DMA IP for memory-to-memory transfers.
 */

#ifndef AXI_CDMA_DRIVER_H
#define AXI_CDMA_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "../dma_benchmark.h"

/*******************************************************************************
 * AXI CDMA Register Offsets
 ******************************************************************************/

#define XAXICDMA_CR_OFFSET         0x00  /* Control register */
#define XAXICDMA_SR_OFFSET         0x04  /* Status register */
#define XAXICDMA_CDESC_OFFSET      0x08  /* Current descriptor (low) */
#define XAXICDMA_CDESC_MSB_OFFSET  0x0C  /* Current descriptor (high) */
#define XAXICDMA_TDESC_OFFSET      0x10  /* Tail descriptor (low) */
#define XAXICDMA_TDESC_MSB_OFFSET  0x14  /* Tail descriptor (high) */
#define XAXICDMA_SA_OFFSET         0x18  /* Source address (low) */
#define XAXICDMA_SA_MSB_OFFSET     0x1C  /* Source address (high) */
#define XAXICDMA_DA_OFFSET         0x20  /* Destination address (low) */
#define XAXICDMA_DA_MSB_OFFSET     0x24  /* Destination address (high) */
#define XAXICDMA_BTT_OFFSET        0x28  /* Bytes to transfer */

/*******************************************************************************
 * AXI CDMA Control Register Bits
 ******************************************************************************/

#define XAXICDMA_CR_RESET_MASK     0x00000004  /* Reset CDMA */
#define XAXICDMA_CR_KEYHOLE_RD     0x00000008  /* Keyhole read */
#define XAXICDMA_CR_KEYHOLE_WR     0x00000010  /* Keyhole write */
#define XAXICDMA_CR_CYCLIC_BD      0x00000020  /* Cyclic BD enable */
#define XAXICDMA_CR_IOC_IRQ_EN     0x00001000  /* Interrupt on Complete */
#define XAXICDMA_CR_DLY_IRQ_EN     0x00002000  /* Delay interrupt */
#define XAXICDMA_CR_ERR_IRQ_EN     0x00004000  /* Error interrupt */
#define XAXICDMA_CR_ALL_IRQ_EN     0x00007000  /* All interrupts */

/*******************************************************************************
 * AXI CDMA Status Register Bits
 ******************************************************************************/

#define XAXICDMA_SR_IDLE_MASK      0x00000002  /* CDMA is idle */
#define XAXICDMA_SR_SGINCL_MASK    0x00000008  /* SG included */
#define XAXICDMA_SR_DMAINTERR_MASK 0x00000010  /* DMA internal error */
#define XAXICDMA_SR_DMASLVERR_MASK 0x00000020  /* DMA slave error */
#define XAXICDMA_SR_DMADECERR_MASK 0x00000040  /* DMA decode error */
#define XAXICDMA_SR_SGINTERR_MASK  0x00000100  /* SG internal error */
#define XAXICDMA_SR_SGSLVERR_MASK  0x00000200  /* SG slave error */
#define XAXICDMA_SR_SGDECERR_MASK  0x00000400  /* SG decode error */
#define XAXICDMA_SR_IOC_IRQ_MASK   0x00001000  /* IOC interrupt */
#define XAXICDMA_SR_DLY_IRQ_MASK   0x00002000  /* Delay interrupt */
#define XAXICDMA_SR_ERR_IRQ_MASK   0x00004000  /* Error interrupt */
#define XAXICDMA_SR_ALL_ERR_MASK   0x00000770  /* All error bits */
#define XAXICDMA_SR_ALL_IRQ_MASK   0x00007000  /* All interrupt bits */

/*******************************************************************************
 * AXI CDMA Scatter-Gather Descriptor
 ******************************************************************************/

typedef struct __attribute__((aligned(64))) {
    uint32_t next_desc;        /* 0x00: Next descriptor pointer (low) */
    uint32_t next_desc_msb;    /* 0x04: Next descriptor pointer (high) */
    uint32_t src_addr;         /* 0x08: Source address (low) */
    uint32_t src_addr_msb;     /* 0x0C: Source address (high) */
    uint32_t dst_addr;         /* 0x10: Destination address (low) */
    uint32_t dst_addr_msb;     /* 0x14: Destination address (high) */
    uint32_t control;          /* 0x18: Control word (BTT) */
    uint32_t status;           /* 0x1C: Status word */
    uint32_t reserved[8];      /* 0x20-0x3C: Padding to 64 bytes */
} AxiCdmaSgDesc_t;

/* Descriptor status bits */
#define XAXICDMA_BD_STS_COMPLETE_MASK 0x80000000 /* Completed */
#define XAXICDMA_BD_STS_DECERR_MASK   0x40000000 /* Decode error */
#define XAXICDMA_BD_STS_SLVERR_MASK   0x20000000 /* Slave error */
#define XAXICDMA_BD_STS_INTERR_MASK   0x10000000 /* Internal error */
#define XAXICDMA_BD_STS_ALL_ERR_MASK  0x70000000 /* All error bits */

/*******************************************************************************
 * AXI CDMA Instance Structure
 ******************************************************************************/

typedef struct {
    uint64_t base_addr;
    bool initialized;
    bool sg_mode;
    uint32_t data_width;
    uint32_t addr_width;
    uint32_t max_burst_len;

    /* Descriptor ring */
    AxiCdmaSgDesc_t* desc_ring;
    uint32_t ring_size;
    uint32_t desc_head;
    uint32_t desc_tail;

    /* Transfer state */
    volatile bool transfer_complete;
    volatile uint32_t transfer_error;

    /* Statistics */
    uint64_t bytes_transferred;
    uint32_t num_transfers;
    uint32_t errors;
} AxiCdmaInst_t;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize AXI CDMA driver
 * @return 0 on success, negative error code on failure
 */
int axi_cdma_init(void);

/**
 * @brief Cleanup AXI CDMA driver
 */
void axi_cdma_cleanup(void);

/**
 * @brief Reset AXI CDMA
 * @return 0 on success, negative error code on failure
 */
int axi_cdma_reset(void);

/**
 * @brief Configure AXI CDMA for operation
 * @param use_sg Enable Scatter-Gather mode
 * @param use_irq Enable interrupts
 * @return 0 on success, negative error code on failure
 */
int axi_cdma_configure(bool use_sg, bool use_irq);

/**
 * @brief Setup SG descriptor ring
 * @param descs Pointer to descriptor memory
 * @param num_descs Number of descriptors
 * @return 0 on success, negative error code on failure
 */
int axi_cdma_setup_sg_ring(AxiCdmaSgDesc_t* descs, uint32_t num_descs);

/**
 * @brief Start a simple (non-SG) memory-to-memory transfer
 * @param src_addr Source address
 * @param dst_addr Destination address
 * @param length Transfer length in bytes
 * @return 0 on success, negative error code on failure
 */
int axi_cdma_simple_transfer(uint64_t src_addr, uint64_t dst_addr, uint32_t length);

/**
 * @brief Start a Scatter-Gather memory-to-memory transfer
 * @param src_addr Source address
 * @param dst_addr Destination address
 * @param length Transfer length in bytes
 * @return 0 on success, negative error code on failure
 */
int axi_cdma_sg_transfer(uint64_t src_addr, uint64_t dst_addr, uint32_t length);

/**
 * @brief Wait for transfer completion (polling)
 * @param timeout_us Timeout in microseconds
 * @return 0 on success, negative error code on failure/timeout
 */
int axi_cdma_wait_complete(uint32_t timeout_us);

/**
 * @brief Check if CDMA is busy
 * @return true if busy, false if idle
 */
bool axi_cdma_is_busy(void);

/**
 * @brief Get CDMA status
 * @return Status register value
 */
uint32_t axi_cdma_get_status(void);

/**
 * @brief Get AXI CDMA instance pointer
 * @return Pointer to AXI CDMA instance
 */
AxiCdmaInst_t* axi_cdma_get_instance(void);

/**
 * @brief Print AXI CDMA statistics
 */
void axi_cdma_print_stats(void);

/**
 * @brief Clear AXI CDMA statistics
 */
void axi_cdma_clear_stats(void);

/**
 * @brief AXI CDMA interrupt handler
 */
void axi_cdma_irq_handler(void);

#endif /* AXI_CDMA_DRIVER_H */
