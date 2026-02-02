/**
 * @file lpd_dma_test.h
 * @brief LPD DMA Test Module Header
 */

#ifndef LPD_DMA_TEST_H
#define LPD_DMA_TEST_H

#include "../dma_benchmark.h"

/**
 * @brief Run all LPD DMA tests
 * @return 0 on success, negative error code on failure
 */
int lpd_dma_test_run_all(void);

/**
 * @brief Run LPD DMA throughput test
 * @param channel Channel number (0-7)
 * @param size Transfer size
 * @param result Test result output
 * @return 0 on success, negative error code on failure
 */
int lpd_dma_test_throughput(uint32_t channel, uint32_t size, TestResult_t* result);

/**
 * @brief Run LPD DMA latency test
 * @param channel Channel number
 * @param result Test result output
 * @return 0 on success, negative error code on failure
 */
int lpd_dma_test_latency(uint32_t channel, TestResult_t* result);

/**
 * @brief Run LPD DMA multi-channel test
 * @param num_channels Number of channels to use
 * @param size Transfer size per channel
 * @param result Aggregate result
 * @return 0 on success, negative error code on failure
 */
int lpd_dma_test_multi_channel(uint32_t num_channels, uint32_t size, TestResult_t* result);

/**
 * @brief Run LPD DMA data integrity test
 * @param pattern Data pattern
 * @param result Test result output
 * @return 0 on success, negative error code on failure
 */
int lpd_dma_test_integrity(DataPattern_t pattern, TestResult_t* result);

#endif /* LPD_DMA_TEST_H */
