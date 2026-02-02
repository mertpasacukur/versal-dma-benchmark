/**
 * @file latency_test.h
 * @brief Latency Test Scenarios Header
 */

#ifndef LATENCY_TEST_H
#define LATENCY_TEST_H

#include "../dma_benchmark.h"

/**
 * @brief Run all latency tests
 * @return 0 on success, negative error code on failure
 */
int latency_test_run_all(void);

/**
 * @brief Run first-byte latency test for a DMA type
 * @param dma_type DMA type to test
 * @param result Test result output
 * @return 0 on success, negative error code on failure
 */
int latency_test_first_byte(DmaType_t dma_type, TestResult_t* result);

/**
 * @brief Run DMA setup time test
 * @param dma_type DMA type to test
 * @return Setup time in microseconds
 */
double latency_test_setup_time(DmaType_t dma_type);

/**
 * @brief Compare polling vs interrupt latency
 * @return 0 on success, negative error code on failure
 */
int latency_test_polling_vs_interrupt(void);

#endif /* LATENCY_TEST_H */
