/**
 * @file stress_test.h
 * @brief Stress Test Scenarios Header
 */

#ifndef STRESS_TEST_H
#define STRESS_TEST_H

#include <stdint.h>

/**
 * @brief Run stress test for specified duration
 * @param duration_sec Duration in seconds
 * @return 0 on success, negative error code on failure
 */
int stress_test_run(uint32_t duration_sec);

/**
 * @brief Run continuous transfer stress test
 * @param duration_sec Duration in seconds
 * @return 0 on success, negative error code on failure
 */
int stress_test_continuous(uint32_t duration_sec);

/**
 * @brief Run random pattern stress test
 * @param duration_sec Duration in seconds
 * @return 0 on success, negative error code on failure
 */
int stress_test_random_pattern(uint32_t duration_sec);

/**
 * @brief Run multi-DMA stress test
 * @param duration_sec Duration in seconds
 * @return 0 on success, negative error code on failure
 */
int stress_test_multi_dma(uint32_t duration_sec);

#endif /* STRESS_TEST_H */
