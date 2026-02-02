/**
 * @file cache_utils.h
 * @brief Cache Management Utilities Header
 *
 * Cache flush, invalidate, and maintenance operations.
 */

#ifndef CACHE_UTILS_H
#define CACHE_UTILS_H

#include <stdint.h>

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Flush data cache for address range
 * @param addr Start address
 * @param size Size in bytes
 */
void cache_flush_range(uint64_t addr, uint32_t size);

/**
 * @brief Invalidate data cache for address range
 * @param addr Start address
 * @param size Size in bytes
 */
void cache_invalidate_range(uint64_t addr, uint32_t size);

/**
 * @brief Flush and invalidate data cache for address range
 * @param addr Start address
 * @param size Size in bytes
 */
void cache_flush_invalidate_range(uint64_t addr, uint32_t size);

/**
 * @brief Flush entire data cache
 */
void cache_flush_all(void);

/**
 * @brief Invalidate entire data cache
 */
void cache_invalidate_all(void);

/**
 * @brief Enable data cache
 */
void cache_enable(void);

/**
 * @brief Disable data cache
 */
void cache_disable(void);

/**
 * @brief Check if data cache is enabled
 * @return 1 if enabled, 0 if disabled
 */
int cache_is_enabled(void);

/**
 * @brief Memory barrier (DSB)
 */
void cache_memory_barrier(void);

/**
 * @brief Instruction synchronization barrier (ISB)
 */
void cache_instruction_barrier(void);

/**
 * @brief Prepare source buffer for DMA (flush)
 * @param addr Buffer address
 * @param size Buffer size
 */
void cache_prep_dma_src(uint64_t addr, uint32_t size);

/**
 * @brief Prepare destination buffer for DMA (invalidate)
 * @param addr Buffer address
 * @param size Buffer size
 */
void cache_prep_dma_dst(uint64_t addr, uint32_t size);

/**
 * @brief Complete DMA to destination (invalidate to see new data)
 * @param addr Buffer address
 * @param size Buffer size
 */
void cache_complete_dma_dst(uint64_t addr, uint32_t size);

#endif /* CACHE_UTILS_H */
