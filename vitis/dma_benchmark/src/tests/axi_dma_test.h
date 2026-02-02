/**
 * @file axi_dma_test.h
 * @brief AXI DMA Test Module Header
 */

#ifndef AXI_DMA_TEST_H
#define AXI_DMA_TEST_H

#include "../dma_benchmark.h"

/**
 * @brief Run all AXI DMA tests
 * @return 0 on success, negative error code on failure
 */
int axi_dma_test_run_all(void);

/**
 * @brief Run AXI DMA throughput test
 * @param src_region Source memory region
 * @param dst_region Destination memory region
 * @param result Test result output
 * @return 0 on success, negative error code on failure
 */
int axi_dma_test_throughput(MemoryRegion_t src_region, MemoryRegion_t dst_region, TestResult_t* result);

/**
 * @brief Run AXI DMA latency test
 * @param src_region Source memory region
 * @param dst_region Destination memory region
 * @param result Test result output
 * @return 0 on success, negative error code on failure
 */
int axi_dma_test_latency(MemoryRegion_t src_region, MemoryRegion_t dst_region, TestResult_t* result);

/**
 * @brief Run AXI DMA data integrity test
 * @param pattern Data pattern to test
 * @param result Test result output
 * @return 0 on success, negative error code on failure
 */
int axi_dma_test_integrity(DataPattern_t pattern, TestResult_t* result);

/**
 * @brief Run AXI DMA simple mode test
 * @param result Test result output
 * @return 0 on success, negative error code on failure
 */
int axi_dma_test_simple_mode(TestResult_t* result);

/**
 * @brief Run AXI DMA scatter-gather mode test
 * @param result Test result output
 * @return 0 on success, negative error code on failure
 */
int axi_dma_test_sg_mode(TestResult_t* result);

/**
 * @brief Run AXI DMA bidirectional transfer test
 * @param result Test result output
 * @return 0 on success, negative error code on failure
 */
int axi_dma_test_bidirectional(TestResult_t* result);

#endif /* AXI_DMA_TEST_H */
