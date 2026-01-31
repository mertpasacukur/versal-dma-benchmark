/**
 * @file lpd_dma_driver.h
 * @brief LPD DMA (ADMA) Driver Wrapper Header
 *
 * Wrapper for Versal PS LPD DMA (ADMA) - 8 channel DMA engine.
 */

#ifndef LPD_DMA_DRIVER_H
#define LPD_DMA_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "../dma_benchmark.h"

/*******************************************************************************
 * LPD DMA Configuration
 ******************************************************************************/

#define LPD_DMA_NUM_CHANNELS       8
#define LPD_DMA_CHANNEL_SPACING    0x10000

/*******************************************************************************
 * LPD DMA Register Offsets
 ******************************************************************************/

#define XLPDDMA_ZDMA_ERR_CTRL      0x000  /* Error control */
#define XLPDDMA_ZDMA_CH_ISR        0x100  /* Interrupt status */
#define XLPDDMA_ZDMA_CH_IMR        0x104  /* Interrupt mask */
#define XLPDDMA_ZDMA_CH_IEN        0x108  /* Interrupt enable */
#define XLPDDMA_ZDMA_CH_IDS        0x10C  /* Interrupt disable */
#define XLPDDMA_ZDMA_CH_CTRL0      0x110  /* Channel control 0 */
#define XLPDDMA_ZDMA_CH_CTRL1      0x114  /* Channel control 1 (pause/resume) */
#define XLPDDMA_ZDMA_CH_CTRL2      0x200  /* Channel control 2 (enable/start) */
#define XLPDDMA_ZDMA_CH_FCI        0x118  /* Flow control interval */
#define XLPDDMA_ZDMA_CH_STATUS     0x11C  /* Channel status */
#define XLPDDMA_ZDMA_CH_DATA_ATTR  0x120  /* Data attributes */
#define XLPDDMA_ZDMA_CH_DSCR_ATTR  0x124  /* Descriptor attributes */
#define XLPDDMA_ZDMA_CH_SRC_DSCR_WRD0 0x128  /* Source descriptor word 0 */
#define XLPDDMA_ZDMA_CH_SRC_DSCR_WRD1 0x12C  /* Source descriptor word 1 */
#define XLPDDMA_ZDMA_CH_SRC_DSCR_WRD2 0x130  /* Source descriptor word 2 */
#define XLPDDMA_ZDMA_CH_SRC_DSCR_WRD3 0x134  /* Source descriptor word 3 */
#define XLPDDMA_ZDMA_CH_DST_DSCR_WRD0 0x138  /* Destination descriptor word 0 */
#define XLPDDMA_ZDMA_CH_DST_DSCR_WRD1 0x13C  /* Destination descriptor word 1 */
#define XLPDDMA_ZDMA_CH_DST_DSCR_WRD2 0x140  /* Destination descriptor word 2 */
#define XLPDDMA_ZDMA_CH_DST_DSCR_WRD3 0x144  /* Destination descriptor word 3 */
#define XLPDDMA_ZDMA_CH_SRC_START_LSB 0x158  /* Source start addr (low) */
#define XLPDDMA_ZDMA_CH_SRC_START_MSB 0x15C  /* Source start addr (high) */
#define XLPDDMA_ZDMA_CH_DST_START_LSB 0x160  /* Destination start addr (low) */
#define XLPDDMA_ZDMA_CH_DST_START_MSB 0x164  /* Destination start addr (high) */
#define XLPDDMA_ZDMA_CH_RATE_CTRL  0x18C  /* Rate control */
#define XLPDDMA_ZDMA_CH_SRC_CUR_PYLD_LSB 0x168  /* Current source payload (low) */
#define XLPDDMA_ZDMA_CH_SRC_CUR_PYLD_MSB 0x16C  /* Current source payload (high) */
#define XLPDDMA_ZDMA_CH_DST_CUR_PYLD_LSB 0x170  /* Current dest payload (low) */
#define XLPDDMA_ZDMA_CH_DST_CUR_PYLD_MSB 0x174  /* Current dest payload (high) */
#define XLPDDMA_ZDMA_CH_SRC_CUR_DSCR_LSB 0x178  /* Current source descriptor (low) */
#define XLPDDMA_ZDMA_CH_SRC_CUR_DSCR_MSB 0x17C  /* Current source descriptor (high) */
#define XLPDDMA_ZDMA_CH_DST_CUR_DSCR_LSB 0x180  /* Current dest descriptor (low) */
#define XLPDDMA_ZDMA_CH_DST_CUR_DSCR_MSB 0x184  /* Current dest descriptor (high) */
#define XLPDDMA_ZDMA_CH_TOTAL_BYTE 0x188  /* Total bytes transferred */
#define XLPDDMA_ZDMA_CH_WR_ONLY_WORD 0x190  /* Write-only word */

/*******************************************************************************
 * LPD DMA Control Register Bits
 ******************************************************************************/

#define XLPDDMA_CTRL0_OVR_FETCH    0x00000080  /* Overflow fetch */
#define XLPDDMA_CTRL0_POINT_TYPE   0x00000040  /* Pointer type (0=simple, 1=linear) */
#define XLPDDMA_CTRL0_MODE_MASK    0x00000030  /* Mode mask */
#define XLPDDMA_CTRL0_MODE_NORMAL  0x00000000  /* Normal mode */
#define XLPDDMA_CTRL0_MODE_WONLY   0x00000010  /* Write-only mode */
#define XLPDDMA_CTRL0_MODE_RONLY   0x00000020  /* Read-only mode */
#define XLPDDMA_CTRL0_RATE_CTRL    0x00000008  /* Rate control enable */
#define XLPDDMA_CTRL0_CONT_ADDR    0x00000004  /* Contiguous address */
#define XLPDDMA_CTRL0_CONT         0x00000002  /* Continuous mode */

#define XLPDDMA_CTRL1_PAUSE_MASK   0x00000002  /* Pause */
#define XLPDDMA_CTRL1_RESUME_MASK  0x00000001  /* Resume */

/*******************************************************************************
 * LPD DMA Status Register Bits (from Xilinx xzdma_hw.h)
 ******************************************************************************/

#define XLPDDMA_STATUS_STATE_MASK  0x00000003  /* State mask */
#define XLPDDMA_STATUS_STATE_DONE  0x00000000  /* Done/Idle (success) */
#define XLPDDMA_STATUS_STATE_PAUSE 0x00000001  /* Paused */
#define XLPDDMA_STATUS_STATE_BUSY  0x00000002  /* Busy */
#define XLPDDMA_STATUS_STATE_ERR   0x00000003  /* Done with error */

/* Legacy alias */
#define XLPDDMA_STATUS_STATE_IDLE  XLPDDMA_STATUS_STATE_DONE

/*******************************************************************************
 * LPD DMA Interrupt Bits (from Xilinx xzdma_hw.h)
 ******************************************************************************/

#define XLPDDMA_IXR_DMA_PAUSE      0x00000800  /* DMA paused */
#define XLPDDMA_IXR_DMA_DONE       0x00000400  /* DMA done */
#define XLPDDMA_IXR_AXI_WR_DATA    0x00000200  /* AXI write data error */
#define XLPDDMA_IXR_AXI_RD_DATA    0x00000100  /* AXI read data error */
#define XLPDDMA_IXR_AXI_RD_DST_DSCR 0x00000080 /* AXI read dst descriptor error */
#define XLPDDMA_IXR_AXI_RD_SRC_DSCR 0x00000040 /* AXI read src descriptor error */
#define XLPDDMA_IXR_DST_ACCT_ERR   0x00000020  /* DST interrupt count overflow */
#define XLPDDMA_IXR_SRC_ACCT_ERR   0x00000010  /* SRC interrupt count overflow */
#define XLPDDMA_IXR_BYTE_CNT_OVRFL 0x00000008  /* Byte count overflow */
#define XLPDDMA_IXR_DST_DSCR_DONE  0x00000004  /* Dest descriptor done */
#define XLPDDMA_IXR_SRC_DSCR_DONE  0x00000002  /* Source descriptor done */
#define XLPDDMA_IXR_INV_APB        0x00000001  /* Invalid APB access */
#define XLPDDMA_IXR_ALL_MASK       0x00000FFF  /* All interrupts */
#define XLPDDMA_IXR_ERR_MASK       0x00000BF9  /* All error interrupts */

/*******************************************************************************
 * LPD DMA Descriptor
 ******************************************************************************/

typedef struct __attribute__((aligned(64))) {
    uint64_t addr;             /* Address */
    uint32_t size;             /* Size */
    uint32_t ctrl;             /* Control */
} LpdDmaDesc_t;

/* Descriptor control bits */
#define XLPDDMA_DESC_CTRL_INTR_EN  0x00000008  /* Interrupt enable */
#define XLPDDMA_DESC_CTRL_PAUSE    0x00000010  /* Pause after this descriptor */
#define XLPDDMA_DESC_CTRL_TYPE_MASK 0x00000006 /* Type mask */
#define XLPDDMA_DESC_CTRL_TYPE_LINEAR 0x00000000  /* Linear descriptor */
#define XLPDDMA_DESC_CTRL_TYPE_LINKED 0x00000002  /* Linked list descriptor */

/*******************************************************************************
 * LPD DMA Channel Structure
 ******************************************************************************/

typedef struct {
    uint32_t channel_id;
    uint64_t base_addr;
    bool initialized;
    bool busy;
    volatile bool transfer_complete;
    volatile uint32_t transfer_error;
    uint64_t bytes_transferred;
    uint32_t num_transfers;
    uint32_t errors;
} LpdDmaChannel_t;

/*******************************************************************************
 * LPD DMA Instance Structure
 ******************************************************************************/

typedef struct {
    bool initialized;
    LpdDmaChannel_t channels[LPD_DMA_NUM_CHANNELS];
    uint64_t total_bytes;
    uint32_t total_transfers;
    uint32_t total_errors;
} LpdDmaInst_t;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize LPD DMA driver
 * @return 0 on success, negative error code on failure
 */
int lpd_dma_init(void);

/**
 * @brief Cleanup LPD DMA driver
 */
void lpd_dma_cleanup(void);

/**
 * @brief Reset LPD DMA channel
 * @param channel Channel number (0-7)
 * @return 0 on success, negative error code on failure
 */
int lpd_dma_reset_channel(uint32_t channel);

/**
 * @brief Configure LPD DMA channel
 * @param channel Channel number (0-7)
 * @param use_irq Enable interrupts
 * @return 0 on success, negative error code on failure
 */
int lpd_dma_configure_channel(uint32_t channel, bool use_irq);

/**
 * @brief Start a simple memory-to-memory transfer
 * @param channel Channel number (0-7)
 * @param src_addr Source address
 * @param dst_addr Destination address
 * @param length Transfer length in bytes
 * @return 0 on success, negative error code on failure
 */
int lpd_dma_transfer(uint32_t channel, uint64_t src_addr, uint64_t dst_addr, uint32_t length);

/**
 * @brief Start source-only transfer (read from memory)
 * @param channel Channel number
 * @param src_addr Source address
 * @param length Transfer length
 * @return 0 on success, negative error code on failure
 */
int lpd_dma_start_src(uint32_t channel, uint64_t src_addr, uint32_t length);

/**
 * @brief Start destination-only transfer (write to memory)
 * @param channel Channel number
 * @param dst_addr Destination address
 * @param length Transfer length
 * @param data Data word to write
 * @return 0 on success, negative error code on failure
 */
int lpd_dma_start_dst(uint32_t channel, uint64_t dst_addr, uint32_t length, uint32_t data);

/**
 * @brief Wait for transfer completion (polling)
 * @param channel Channel number (0-7)
 * @param timeout_us Timeout in microseconds
 * @return 0 on success, negative error code on failure/timeout
 */
int lpd_dma_wait_complete(uint32_t channel, uint32_t timeout_us);

/**
 * @brief Check if channel is busy
 * @param channel Channel number
 * @return true if busy, false if idle
 */
bool lpd_dma_is_busy(uint32_t channel);

/**
 * @brief Get channel status
 * @param channel Channel number
 * @return Status register value
 */
uint32_t lpd_dma_get_status(uint32_t channel);

/**
 * @brief Get total bytes transferred by channel
 * @param channel Channel number
 * @return Total bytes transferred
 */
uint32_t lpd_dma_get_total_bytes(uint32_t channel);

/**
 * @brief Get LPD DMA instance pointer
 * @return Pointer to LPD DMA instance
 */
LpdDmaInst_t* lpd_dma_get_instance(void);

/**
 * @brief Print LPD DMA statistics
 */
void lpd_dma_print_stats(void);

/**
 * @brief Clear LPD DMA statistics
 */
void lpd_dma_clear_stats(void);

/**
 * @brief LPD DMA interrupt handler
 * @param channel Channel that triggered interrupt
 */
void lpd_dma_irq_handler(uint32_t channel);

#endif /* LPD_DMA_DRIVER_H */
