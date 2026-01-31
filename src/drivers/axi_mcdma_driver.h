/**
 * @file axi_mcdma_driver.h
 * @brief AXI MCDMA Driver Wrapper Header
 *
 * Wrapper for Xilinx AXI Multi-Channel DMA IP.
 */

#ifndef AXI_MCDMA_DRIVER_H
#define AXI_MCDMA_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "../dma_benchmark.h"

/*******************************************************************************
 * AXI MCDMA Configuration
 ******************************************************************************/

#define MCDMA_MAX_CHANNELS         16
#define MCDMA_MM2S_BASE_OFFSET     0x000
#define MCDMA_S2MM_BASE_OFFSET     0x500
#define MCDMA_CHANNEL_OFFSET       0x040

/*******************************************************************************
 * AXI MCDMA Common Register Offsets
 ******************************************************************************/

#define XMCDMA_CCR_OFFSET          0x00  /* Common control register */
#define XMCDMA_CSR_OFFSET          0x04  /* Common status register */
#define XMCDMA_CHEN_OFFSET         0x08  /* Channel enable register */
#define XMCDMA_CHSER_OFFSET        0x0C  /* Channel service register */

/*******************************************************************************
 * AXI MCDMA Per-Channel Register Offsets (relative to channel base)
 ******************************************************************************/

#define XMCDMA_CH_CR_OFFSET        0x00  /* Channel control register */
#define XMCDMA_CH_SR_OFFSET        0x04  /* Channel status register */
#define XMCDMA_CH_CDESC_OFFSET     0x08  /* Current descriptor (low) */
#define XMCDMA_CH_CDESC_MSB_OFFSET 0x0C  /* Current descriptor (high) */
#define XMCDMA_CH_TDESC_OFFSET     0x10  /* Tail descriptor (low) */
#define XMCDMA_CH_TDESC_MSB_OFFSET 0x14  /* Tail descriptor (high) */
#define XMCDMA_CH_PKTCNT_OFFSET    0x18  /* Packet count register */

/*******************************************************************************
 * AXI MCDMA Control Register Bits
 ******************************************************************************/

#define XMCDMA_CCR_RS_MASK         0x00000001  /* Run/Stop */
#define XMCDMA_CCR_RESET_MASK      0x00000004  /* Reset */

#define XMCDMA_CH_CR_RS_MASK       0x00000001  /* Channel Run/Stop */
#define XMCDMA_CH_CR_IOC_IRQ_EN    0x00001000  /* IOC interrupt enable */
#define XMCDMA_CH_CR_DLY_IRQ_EN    0x00002000  /* Delay interrupt enable */
#define XMCDMA_CH_CR_ERR_IRQ_EN    0x00004000  /* Error interrupt enable */
#define XMCDMA_CH_CR_ALL_IRQ_EN    0x00007000  /* All interrupts enable */

/*******************************************************************************
 * AXI MCDMA Status Register Bits
 ******************************************************************************/

#define XMCDMA_CSR_HALTED_MASK     0x00000001  /* Halted */
#define XMCDMA_CSR_IDLE_MASK       0x00000002  /* Idle */

#define XMCDMA_CH_SR_IDLE_MASK     0x00000002  /* Channel idle */
#define XMCDMA_CH_SR_ERR_MASK      0x00000070  /* Channel error mask */
#define XMCDMA_CH_SR_IOC_IRQ_MASK  0x00001000  /* IOC interrupt */
#define XMCDMA_CH_SR_DLY_IRQ_MASK  0x00002000  /* Delay interrupt */
#define XMCDMA_CH_SR_ERR_IRQ_MASK  0x00004000  /* Error interrupt */
#define XMCDMA_CH_SR_ALL_IRQ_MASK  0x00007000  /* All interrupts */

/*******************************************************************************
 * Scheduler Mode
 ******************************************************************************/

typedef enum {
    MCDMA_SCHED_ROUND_ROBIN = 0,
    MCDMA_SCHED_STRICT_PRIORITY
} McdmaSchedMode_t;

/*******************************************************************************
 * AXI MCDMA Scatter-Gather Descriptor
 ******************************************************************************/

typedef struct __attribute__((aligned(64))) {
    uint32_t next_desc;        /* 0x00: Next descriptor pointer (low) */
    uint32_t next_desc_msb;    /* 0x04: Next descriptor pointer (high) */
    uint32_t buffer_addr;      /* 0x08: Buffer address (low) */
    uint32_t buffer_addr_msb;  /* 0x0C: Buffer address (high) */
    uint32_t reserved1[2];     /* 0x10-0x14: Reserved */
    uint32_t control;          /* 0x18: Control word */
    uint32_t status;           /* 0x1C: Status word */
    uint32_t sideband[2];      /* 0x20-0x24: Sideband info */
    uint32_t app[3];           /* 0x28-0x30: Application words */
    uint32_t reserved2[3];     /* 0x34-0x3C: Padding to 64 bytes */
} McdmaSgDesc_t;

/* Descriptor control bits */
#define XMCDMA_BD_CTRL_SOF_MASK    0x80000000  /* Start of frame */
#define XMCDMA_BD_CTRL_EOF_MASK    0x40000000  /* End of frame */
#define XMCDMA_BD_CTRL_LENGTH_MASK 0x03FFFFFF  /* Transfer length */

/* Descriptor status bits */
#define XMCDMA_BD_STS_COMPLETE_MASK 0x80000000 /* Completed */
#define XMCDMA_BD_STS_ALL_ERR_MASK  0x70000000 /* All error bits */

/*******************************************************************************
 * AXI MCDMA Channel Structure
 ******************************************************************************/

typedef struct {
    uint32_t channel_id;
    bool enabled;
    bool busy;
    McdmaSgDesc_t* desc_ring;
    uint32_t ring_size;
    uint32_t desc_head;
    uint32_t desc_tail;
    volatile bool transfer_complete;
    volatile uint32_t transfer_error;
    uint64_t bytes_transferred;
    uint32_t num_transfers;
    uint32_t errors;
} McdmaChannel_t;

/*******************************************************************************
 * AXI MCDMA Instance Structure
 ******************************************************************************/

typedef struct {
    uint64_t base_addr;
    bool initialized;
    uint32_t num_mm2s_channels;
    uint32_t num_s2mm_channels;
    uint32_t data_width;
    McdmaSchedMode_t sched_mode;

    /* Channel structures */
    McdmaChannel_t mm2s_channels[MCDMA_MAX_CHANNELS];
    McdmaChannel_t s2mm_channels[MCDMA_MAX_CHANNELS];

    /* Global statistics */
    uint64_t total_bytes;
    uint32_t total_transfers;
    uint32_t total_errors;
} AxiMcdmaInst_t;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize AXI MCDMA driver
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_init(void);

/**
 * @brief Cleanup AXI MCDMA driver
 */
void axi_mcdma_cleanup(void);

/**
 * @brief Reset AXI MCDMA
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_reset(void);

/**
 * @brief Configure MCDMA scheduler mode
 * @param mode Scheduler mode (round-robin or strict priority)
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_set_scheduler(McdmaSchedMode_t mode);

/**
 * @brief Enable a MM2S (TX) channel
 * @param channel Channel number (0-15)
 * @param use_irq Enable interrupts
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_enable_mm2s_channel(uint32_t channel, bool use_irq);

/**
 * @brief Enable a S2MM (RX) channel
 * @param channel Channel number (0-15)
 * @param use_irq Enable interrupts
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_enable_s2mm_channel(uint32_t channel, bool use_irq);

/**
 * @brief Disable a MM2S channel
 * @param channel Channel number (0-15)
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_disable_mm2s_channel(uint32_t channel);

/**
 * @brief Disable a S2MM channel
 * @param channel Channel number (0-15)
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_disable_s2mm_channel(uint32_t channel);

/**
 * @brief Setup descriptor ring for a MM2S channel
 * @param channel Channel number
 * @param descs Descriptor memory
 * @param num_descs Number of descriptors
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_setup_mm2s_ring(uint32_t channel, McdmaSgDesc_t* descs, uint32_t num_descs);

/**
 * @brief Setup descriptor ring for a S2MM channel
 * @param channel Channel number
 * @param descs Descriptor memory
 * @param num_descs Number of descriptors
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_setup_s2mm_ring(uint32_t channel, McdmaSgDesc_t* descs, uint32_t num_descs);

/**
 * @brief Start a transfer on a specific channel pair (MM2S and S2MM)
 * @param channel Channel number
 * @param src_addr Source buffer address
 * @param dst_addr Destination buffer address
 * @param length Transfer length
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_transfer(uint32_t channel, uint64_t src_addr, uint64_t dst_addr, uint32_t length);

/**
 * @brief Start MM2S transfer on a channel
 * @param channel Channel number
 * @param buffer_addr Source buffer address
 * @param length Transfer length
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_start_mm2s(uint32_t channel, uint64_t buffer_addr, uint32_t length);

/**
 * @brief Start S2MM transfer on a channel
 * @param channel Channel number
 * @param buffer_addr Destination buffer address
 * @param length Transfer length
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_start_s2mm(uint32_t channel, uint64_t buffer_addr, uint32_t length);

/**
 * @brief Wait for MM2S channel completion
 * @param channel Channel number
 * @param timeout_us Timeout in microseconds
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_wait_mm2s(uint32_t channel, uint32_t timeout_us);

/**
 * @brief Wait for S2MM channel completion
 * @param channel Channel number
 * @param timeout_us Timeout in microseconds
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_wait_s2mm(uint32_t channel, uint32_t timeout_us);

/**
 * @brief Wait for both MM2S and S2MM channel completion
 * @param channel Channel number
 * @param timeout_us Timeout in microseconds
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_wait_complete(uint32_t channel, uint32_t timeout_us);

/**
 * @brief Check if MM2S channel is busy
 * @param channel Channel number
 * @return true if busy, false if idle
 */
bool axi_mcdma_mm2s_busy(uint32_t channel);

/**
 * @brief Check if S2MM channel is busy
 * @param channel Channel number
 * @return true if busy, false if idle
 */
bool axi_mcdma_s2mm_busy(uint32_t channel);

/**
 * @brief Get number of active MM2S channels
 * @return Number of enabled channels
 */
uint32_t axi_mcdma_get_mm2s_channel_count(void);

/**
 * @brief Get number of active S2MM channels
 * @return Number of enabled channels
 */
uint32_t axi_mcdma_get_s2mm_channel_count(void);

/**
 * @brief Get MCDMA instance pointer
 * @return Pointer to MCDMA instance
 */
AxiMcdmaInst_t* axi_mcdma_get_instance(void);

/**
 * @brief Print MCDMA statistics
 */
void axi_mcdma_print_stats(void);

/**
 * @brief Clear MCDMA statistics
 */
void axi_mcdma_clear_stats(void);

/**
 * @brief MCDMA interrupt handler
 */
void axi_mcdma_irq_handler(void);

#endif /* AXI_MCDMA_DRIVER_H */
