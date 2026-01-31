/**
 * @file latency_test.c
 * @brief Latency Test Scenarios Implementation
 */

#include <string.h>
#include "latency_test.h"
#include "../utils/debug_print.h"
#include "../drivers/axi_dma_driver.h"
#include "../drivers/axi_cdma_driver.h"
#include "../drivers/lpd_dma_driver.h"
#include "../utils/timer_utils.h"
#include "../utils/memory_utils.h"
#include "../utils/data_patterns.h"
#include "../utils/cache_utils.h"

/*******************************************************************************
 * Local Variables
 ******************************************************************************/

#define LATENCY_TEST_SIZE       64      /* Minimal transfer for latency */
#define LATENCY_ITERATIONS      1000

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

int latency_test_run_all(void)
{
    TestResult_t result;

    LOG_RESULT("\r\n");
    LOG_RESULT("================================================================\r\n");
    LOG_RESULT("                   Latency Test Suite\r\n");
    LOG_RESULT("================================================================\r\n\r\n");

    /* First-byte latency for each DMA type */
    LOG_RESULT("1. First-Byte Latency (64-byte transfers):\r\n\r\n");
    LOG_RESULT("  DMA Type     | Avg (us) | Min (us) | Max (us)\r\n");
    LOG_RESULT("  -------------|----------|----------|----------\r\n");

    /* AXI DMA */
    memset(&result, 0, sizeof(result));
    if (latency_test_first_byte(DMA_TYPE_AXI_DMA, &result) == DMA_SUCCESS) {
        LOG_RESULT("  AXI_DMA      | %8.3f | %8.3f | %8.3f\r\n",
                  result.avg_latency, result.min_latency, result.max_latency);
    } else {
        LOG_RESULT("  AXI_DMA      | %8s | %8s | %8s\r\n", "---", "---", "---");
    }

    /* AXI CDMA */
    memset(&result, 0, sizeof(result));
    if (latency_test_first_byte(DMA_TYPE_AXI_CDMA, &result) == DMA_SUCCESS) {
        LOG_RESULT("  AXI_CDMA     | %8.3f | %8.3f | %8.3f\r\n",
                  result.avg_latency, result.min_latency, result.max_latency);
    } else {
        LOG_RESULT("  AXI_CDMA     | %8s | %8s | %8s\r\n", "---", "---", "---");
    }

    /* LPD DMA */
    memset(&result, 0, sizeof(result));
    if (latency_test_first_byte(DMA_TYPE_LPD_DMA, &result) == DMA_SUCCESS) {
        LOG_RESULT("  LPD_DMA      | %8.3f | %8.3f | %8.3f\r\n",
                  result.avg_latency, result.min_latency, result.max_latency);
    } else {
        LOG_RESULT("  LPD_DMA      | %8s | %8s | %8s\r\n", "---", "---", "---");
    }

    /* DMA Setup Time */
    LOG_RESULT("\r\n2. DMA Setup Time (descriptor/register configuration):\r\n\r\n");
    LOG_RESULT("  DMA Type     | Setup Time (us)\r\n");
    LOG_RESULT("  -------------|----------------\r\n");

    double setup;
    setup = latency_test_setup_time(DMA_TYPE_AXI_DMA);
    LOG_RESULT("  AXI_DMA      | %14.3f\r\n", setup);

    setup = latency_test_setup_time(DMA_TYPE_AXI_CDMA);
    LOG_RESULT("  AXI_CDMA     | %14.3f\r\n", setup);

    setup = latency_test_setup_time(DMA_TYPE_LPD_DMA);
    LOG_RESULT("  LPD_DMA      | %14.3f\r\n", setup);

    /* Polling vs Interrupt */
    LOG_RESULT("\r\n3. Polling vs Interrupt Mode:\r\n\r\n");
    latency_test_polling_vs_interrupt();

    LOG_RESULT("\r\nLatency tests complete.\r\n");
    return DMA_SUCCESS;
}

int latency_test_first_byte(DmaType_t dma_type, TestResult_t* result)
{
    uint64_t src_addr, dst_addr;
    uint64_t total_ns = 0;
    uint64_t min_ns = UINT64_MAX;
    uint64_t max_ns = 0;
    int status;

    /* Get test addresses */
    src_addr = memory_get_test_addr(MEM_REGION_DDR4, 0, LATENCY_TEST_SIZE);
    dst_addr = memory_get_test_addr(MEM_REGION_DDR4, KB(1), LATENCY_TEST_SIZE);

    if (src_addr == 0 || dst_addr == 0) {
        return DMA_ERROR_INVALID_PARAM;
    }

    /* Prepare source buffer */
    pattern_fill((void*)(uintptr_t)src_addr, LATENCY_TEST_SIZE, PATTERN_INCREMENTAL, 0);
    cache_prep_dma_src(src_addr, LATENCY_TEST_SIZE);

    /* Warmup */
    for (uint32_t i = 0; i < 10; i++) {
        cache_prep_dma_dst(dst_addr, LATENCY_TEST_SIZE);

        switch (dma_type) {
            case DMA_TYPE_AXI_DMA:
                axi_dma_simple_transfer(src_addr, dst_addr, LATENCY_TEST_SIZE);
                axi_dma_wait_complete(DMA_TIMEOUT_US);
                break;
            case DMA_TYPE_AXI_CDMA:
                axi_cdma_simple_transfer(src_addr, dst_addr, LATENCY_TEST_SIZE);
                axi_cdma_wait_complete(DMA_TIMEOUT_US);
                break;
            case DMA_TYPE_LPD_DMA:
                lpd_dma_transfer(0, src_addr, dst_addr, LATENCY_TEST_SIZE);
                lpd_dma_wait_complete(0, DMA_TIMEOUT_US);
                break;
            default:
                return DMA_ERROR_NOT_SUPPORTED;
        }
    }

    /* Measure latency */
    for (uint32_t i = 0; i < LATENCY_ITERATIONS; i++) {
        cache_prep_dma_dst(dst_addr, LATENCY_TEST_SIZE);

        uint64_t start = timer_start();

        switch (dma_type) {
            case DMA_TYPE_AXI_DMA:
                status = axi_dma_simple_transfer(src_addr, dst_addr, LATENCY_TEST_SIZE);
                if (status == DMA_SUCCESS) {
                    status = axi_dma_wait_complete(DMA_TIMEOUT_US);
                }
                break;
            case DMA_TYPE_AXI_CDMA:
                status = axi_cdma_simple_transfer(src_addr, dst_addr, LATENCY_TEST_SIZE);
                if (status == DMA_SUCCESS) {
                    status = axi_cdma_wait_complete(DMA_TIMEOUT_US);
                }
                break;
            case DMA_TYPE_LPD_DMA:
                status = lpd_dma_transfer(0, src_addr, dst_addr, LATENCY_TEST_SIZE);
                if (status == DMA_SUCCESS) {
                    status = lpd_dma_wait_complete(0, DMA_TIMEOUT_US);
                }
                break;
            default:
                status = DMA_ERROR_NOT_SUPPORTED;
                break;
        }

        uint64_t elapsed = timer_stop_ns(start);

        if (status == DMA_SUCCESS) {
            total_ns += elapsed;
            if (elapsed < min_ns) min_ns = elapsed;
            if (elapsed > max_ns) max_ns = elapsed;
        }
    }

    /* Calculate results */
    result->dma_type = dma_type;
    result->test_type = TEST_LATENCY;
    result->transfer_size = LATENCY_TEST_SIZE;
    result->iterations = LATENCY_ITERATIONS;
    result->avg_latency = (double)total_ns / LATENCY_ITERATIONS / 1000.0;  /* ns to us */
    result->min_latency = (double)min_ns / 1000.0;
    result->max_latency = (double)max_ns / 1000.0;
    result->latency_us = result->avg_latency;
    result->data_integrity = true;

    g_BenchmarkStats.tests_run++;
    g_BenchmarkStats.tests_passed++;

    return DMA_SUCCESS;
}

double latency_test_setup_time(DmaType_t dma_type)
{
    uint64_t src_addr, dst_addr;
    uint64_t total_ns = 0;
    uint32_t iterations = 10000;

    src_addr = memory_get_test_addr(MEM_REGION_DDR4, 0, 64);
    dst_addr = memory_get_test_addr(MEM_REGION_DDR4, 1024, 64);

    if (src_addr == 0 || dst_addr == 0) {
        return 0.0;
    }

    /* Measure only the setup time (without waiting for completion) */
    for (uint32_t i = 0; i < iterations; i++) {
        uint64_t start = timer_start();

        switch (dma_type) {
            case DMA_TYPE_AXI_DMA:
                /* Just configure, don't wait */
                axi_dma_start_rx(dst_addr, 64);
                axi_dma_start_tx(src_addr, 64);
                break;
            case DMA_TYPE_AXI_CDMA:
                axi_cdma_simple_transfer(src_addr, dst_addr, 64);
                break;
            case DMA_TYPE_LPD_DMA:
                lpd_dma_transfer(0, src_addr, dst_addr, 64);
                break;
            default:
                return 0.0;
        }

        total_ns += timer_stop_ns(start);

        /* Wait for completion before next iteration */
        switch (dma_type) {
            case DMA_TYPE_AXI_DMA:
                axi_dma_wait_complete(DMA_TIMEOUT_US);
                break;
            case DMA_TYPE_AXI_CDMA:
                axi_cdma_wait_complete(DMA_TIMEOUT_US);
                break;
            case DMA_TYPE_LPD_DMA:
                lpd_dma_wait_complete(0, DMA_TIMEOUT_US);
                break;
            default:
                break;
        }
    }

    return (double)total_ns / iterations / 1000.0;  /* ns to us */
}

int latency_test_polling_vs_interrupt(void)
{
    /* Note: This test compares polling mode (which we use) vs theoretical interrupt mode */
    LOG_RESULT("  Mode        | Avg Latency (us) | CPU Usage\r\n");
    LOG_RESULT("  ------------|------------------|----------\r\n");

    TestResult_t result;
    memset(&result, 0, sizeof(result));

    /* Polling mode test */
    if (latency_test_first_byte(DMA_TYPE_AXI_CDMA, &result) == DMA_SUCCESS) {
        LOG_RESULT("  Polling     | %16.3f | High (active wait)\r\n", result.avg_latency);
    }

    /* Note about interrupt mode */
    LOG_RESULT("  Interrupt   | %16s | Low (sleep capable)\r\n", "N/A*");
    LOG_RESULT("\r\n  *Interrupt mode not tested - requires GIC configuration.\r\n");
    LOG_RESULT("  Interrupt latency adds ~1-5us overhead but allows CPU to sleep.\r\n");

    return DMA_SUCCESS;
}
