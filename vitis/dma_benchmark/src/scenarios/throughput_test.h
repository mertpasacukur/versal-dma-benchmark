/**
 * @file throughput_test.h
 * @brief Throughput Test Scenarios Header
 */

#ifndef THROUGHPUT_TEST_H
#define THROUGHPUT_TEST_H

#include "../dma_benchmark.h"

/**
 * @brief Run all throughput tests
 * @return 0 on success, negative error code on failure
 */
int throughput_test_run_all(void);

/**
 * @brief Run memory-to-memory transfer matrix test
 * @return 0 on success, negative error code on failure
 */
int throughput_test_run_memory_matrix(void);

/**
 * @brief Run CPU memcpy baseline test
 * @return 0 on success, negative error code on failure
 */
int throughput_test_run_cpu_baseline(void);

/**
 * @brief Run transfer size sweep test
 * @param dma_type DMA type to test
 * @return 0 on success, negative error code on failure
 */
int throughput_test_size_sweep(DmaType_t dma_type);

/**
 * @brief Run aligned vs unaligned transfer test
 * @return 0 on success, negative error code on failure
 */
int throughput_test_alignment(void);

#endif /* THROUGHPUT_TEST_H */
