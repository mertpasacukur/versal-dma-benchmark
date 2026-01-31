/**
 * @file memory_utils.h
 * @brief Memory Utilities Header
 *
 * Memory allocation, comparison, and manipulation utilities.
 */

#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include "../platform_config.h"

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Allocate aligned buffer in specified memory region
 * @param region Memory region to allocate from
 * @param size Size in bytes
 * @param alignment Alignment requirement (must be power of 2)
 * @return Pointer to allocated buffer, NULL on failure
 */
void* memory_alloc_aligned(MemoryRegion_t region, uint32_t size, uint32_t alignment);

/**
 * @brief Free aligned buffer
 * @param ptr Pointer to buffer
 */
void memory_free_aligned(void* ptr);

/**
 * @brief Get buffer address in specified memory region for testing
 * @param region Memory region
 * @param offset Offset from region test base
 * @param size Required size
 * @return Address if valid, 0 on error
 */
uint64_t memory_get_test_addr(MemoryRegion_t region, uint32_t offset, uint32_t size);

/**
 * @brief Check if address range is valid for a memory region
 * @param region Memory region
 * @param addr Start address
 * @param size Size in bytes
 * @return true if valid, false otherwise
 */
bool memory_is_valid_range(MemoryRegion_t region, uint64_t addr, uint32_t size);

/**
 * @brief Compare two memory buffers
 * @param buf1 First buffer
 * @param buf2 Second buffer
 * @param size Size in bytes
 * @param first_diff Output: offset of first difference (if any)
 * @return true if buffers match, false otherwise
 */
bool memory_compare(const void* buf1, const void* buf2, uint32_t size, uint32_t* first_diff);

/**
 * @brief Compare memory buffer with expected pattern
 * @param buf Buffer to check
 * @param size Buffer size
 * @param pattern Expected pattern type
 * @param first_diff Output: offset of first difference (if any)
 * @return true if buffer matches pattern, false otherwise
 */
bool memory_verify_pattern(const void* buf, uint32_t size, uint8_t pattern_type, uint32_t* first_diff);

/**
 * @brief Perform CPU memcpy and return throughput
 * @param dst Destination address
 * @param src Source address
 * @param size Size in bytes
 * @param iterations Number of iterations
 * @return Throughput in MB/s
 */
double memory_cpu_memcpy_benchmark(void* dst, const void* src, uint32_t size, uint32_t iterations);

/**
 * @brief Get maximum available size for a memory region
 * @param region Memory region
 * @return Maximum available size for testing
 */
uint64_t memory_get_max_size(MemoryRegion_t region);

/**
 * @brief Print memory region information
 * @param region Memory region to print info for
 */
void memory_print_region_info(MemoryRegion_t region);

/**
 * @brief Print all memory region information
 */
void memory_print_all_regions(void);

#endif /* MEMORY_UTILS_H */
