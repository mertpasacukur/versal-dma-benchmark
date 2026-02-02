/**
 * @file axi_mcdma_test.h
 * @brief AXI MCDMA Test Module Header
 */

#ifndef AXI_MCDMA_TEST_H
#define AXI_MCDMA_TEST_H

#include "../dma_benchmark.h"

/**
 * @brief Run all AXI MCDMA tests
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_test_run_all(void);

/**
 * @brief Run single channel throughput test
 * @param channel Channel number
 * @param size Transfer size
 * @param result Test result output
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_test_single_channel(uint32_t channel, uint32_t size, TestResult_t* result);

/**
 * @brief Run multi-channel concurrent test
 * @param num_channels Number of channels to use
 * @param size Transfer size per channel
 * @param result Aggregate test result output
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_test_multi_channel(uint32_t num_channels, uint32_t size, TestResult_t* result);

/**
 * @brief Test round-robin scheduling mode
 * @param result Test result output
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_test_round_robin(TestResult_t* result);

/**
 * @brief Test strict priority scheduling mode
 * @param result Test result output
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_test_priority(TestResult_t* result);

/**
 * @brief Test channel scalability (1, 2, 4, 8, 16 channels)
 * @return 0 on success, negative error code on failure
 */
int axi_mcdma_test_scalability(void);

#endif /* AXI_MCDMA_TEST_H */
