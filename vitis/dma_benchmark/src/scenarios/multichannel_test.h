/**
 * @file multichannel_test.h
 * @brief Multi-Channel Test Scenarios Header
 */

#ifndef MULTICHANNEL_TEST_H
#define MULTICHANNEL_TEST_H

#include "../dma_benchmark.h"

/**
 * @brief Run all multi-channel tests
 * @return 0 on success, negative error code on failure
 */
int multichannel_test_run_all(void);

/**
 * @brief Test MCDMA channel scalability
 * @return 0 on success, negative error code on failure
 */
int multichannel_test_mcdma_scalability(void);

/**
 * @brief Test LPD DMA channel scalability
 * @return 0 on success, negative error code on failure
 */
int multichannel_test_lpd_scalability(void);

/**
 * @brief Test concurrent DMA operations from different controllers
 * @return 0 on success, negative error code on failure
 */
int multichannel_test_concurrent_dma(void);

/**
 * @brief Test channel fairness under load
 * @return 0 on success, negative error code on failure
 */
int multichannel_test_fairness(void);

#endif /* MULTICHANNEL_TEST_H */
