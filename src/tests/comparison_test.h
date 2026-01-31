/**
 * @file comparison_test.h
 * @brief DMA Comparison Test Module Header
 */

#ifndef COMPARISON_TEST_H
#define COMPARISON_TEST_H

#include "../dma_benchmark.h"

/**
 * @brief Run all DMA comparison tests
 * @return 0 on success, negative error code on failure
 */
int comparison_test_run(void);

/**
 * @brief Compare throughput across all DMA types
 * @param size Transfer size to test
 * @return 0 on success, negative error code on failure
 */
int comparison_test_throughput(uint32_t size);

/**
 * @brief Compare latency across all DMA types
 * @return 0 on success, negative error code on failure
 */
int comparison_test_latency(void);

/**
 * @brief Compare CPU memcpy vs DMA engines
 * @return 0 on success, negative error code on failure
 */
int comparison_test_vs_cpu(void);

/**
 * @brief Print comparison summary table
 */
void comparison_test_print_summary(void);

#endif /* COMPARISON_TEST_H */
