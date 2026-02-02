/**
 * @file axi_cdma_test.h
 * @brief AXI CDMA Test Module Header
 */

#ifndef AXI_CDMA_TEST_H
#define AXI_CDMA_TEST_H

#include "../dma_benchmark.h"

/**
 * @brief Run all AXI CDMA tests
 * @return 0 on success, negative error code on failure
 */
int axi_cdma_test_run_all(void);

/**
 * @brief Run AXI CDMA throughput test between memory regions
 * @param src_region Source memory region
 * @param dst_region Destination memory region
 * @param size Transfer size
 * @param result Test result output
 * @return 0 on success, negative error code on failure
 */
int axi_cdma_test_throughput(MemoryRegion_t src_region, MemoryRegion_t dst_region,
                            uint32_t size, TestResult_t* result);

/**
 * @brief Run AXI CDMA latency test
 * @param src_region Source memory region
 * @param dst_region Destination memory region
 * @param result Test result output
 * @return 0 on success, negative error code on failure
 */
int axi_cdma_test_latency(MemoryRegion_t src_region, MemoryRegion_t dst_region,
                         TestResult_t* result);

/**
 * @brief Run AXI CDMA data integrity test
 * @param pattern Data pattern to test
 * @param result Test result output
 * @return 0 on success, negative error code on failure
 */
int axi_cdma_test_integrity(DataPattern_t pattern, TestResult_t* result);

/**
 * @brief Run memory-to-memory matrix test (all region combinations)
 * @return 0 on success, negative error code on failure
 */
int axi_cdma_test_memory_matrix(void);

#endif /* AXI_CDMA_TEST_H */
