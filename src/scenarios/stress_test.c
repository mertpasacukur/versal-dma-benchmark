/**
 * @file stress_test.c
 * @brief Stress Test Scenarios Implementation
 */

#include <string.h>
#include "stress_test.h"
#include "../utils/debug_print.h"
#include "../dma_benchmark.h"
#include "../drivers/axi_dma_driver.h"
#include "../drivers/axi_cdma_driver.h"
#include "../drivers/axi_mcdma_driver.h"
#include "../drivers/lpd_dma_driver.h"
#include "../utils/timer_utils.h"
#include "../utils/memory_utils.h"
#include "../utils/data_patterns.h"
#include "../utils/cache_utils.h"

/*******************************************************************************
 * Local Variables
 ******************************************************************************/

#define STRESS_BUFFER_SIZE      MB(1)
#define STRESS_REPORT_INTERVAL  60  /* Report every 60 seconds */

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

int stress_test_run(uint32_t duration_sec)
{
    LOG_RESULT("\r\n");
    LOG_RESULT("================================================================\r\n");
    LOG_RESULT("                   Stress Test Suite\r\n");
    LOG_RESULT("================================================================\r\n\r\n");

    LOG_RESULT("Duration: %lu seconds\r\n", duration_sec);
    LOG_RESULT("Press any key to abort...\r\n\r\n");

    /* Run main stress test */
    int status = stress_test_continuous(duration_sec);

    if (g_TestAbort) {
        LOG_RESULT("\r\nStress test aborted by user.\r\n");
    } else {
        LOG_RESULT("\r\nStress test completed successfully.\r\n");
    }

    return status;
}

int stress_test_continuous(uint32_t duration_sec)
{
    uint64_t src_addr, dst_addr;
    uint64_t start_time, current_time;
    uint64_t last_report_time;
    uint64_t total_bytes = 0;
    uint64_t total_transfers = 0;
    uint32_t errors = 0;
    uint32_t elapsed_sec = 0;
    uint32_t size = STRESS_BUFFER_SIZE;
    int status;

    /* Allocate buffers */
    src_addr = memory_get_test_addr(MEM_REGION_DDR4, 0, size);
    dst_addr = memory_get_test_addr(MEM_REGION_DDR4, size * 2, size);

    if (src_addr == 0 || dst_addr == 0) {
        LOG_RESULT("ERROR: Could not allocate stress test buffers\r\n");
        return DMA_ERROR_NO_MEMORY;
    }

    /* Initialize source buffer */
    LOG_RESULT("Initializing test buffers...\r\n");
    pattern_fill((void*)(uintptr_t)src_addr, size, PATTERN_RANDOM, 0xDEADBEEF);
    cache_prep_dma_src(src_addr, size);

    LOG_RESULT("Starting continuous DMA stress test (CDMA)...\r\n\r\n");
    LOG_RESULT("  Time (s) | Transfers | Bytes (GB) | Throughput (MB/s) | Errors\r\n");
    LOG_RESULT("  ---------|-----------|------------|-------------------|-------\r\n");

    start_time = timer_get_us();
    last_report_time = start_time;

    while (elapsed_sec < duration_sec && !g_TestAbort) {
        /* Check for user abort (non-blocking) */
        /* In real implementation, check UART for any key press */

        /* Perform transfer */
        cache_prep_dma_dst(dst_addr, size);

        status = axi_cdma_simple_transfer(src_addr, dst_addr, size);
        if (status != DMA_SUCCESS) {
            errors++;
            continue;
        }

        status = axi_cdma_wait_complete(DMA_TIMEOUT_US);
        if (status != DMA_SUCCESS) {
            errors++;
            continue;
        }

        total_transfers++;
        total_bytes += size;

        /* Periodic verification (every 100 transfers) */
        if ((total_transfers % 100) == 0) {
            cache_complete_dma_dst(dst_addr, size);
            uint32_t error_offset;
            uint8_t expected, actual;
            if (!pattern_verify((void*)(uintptr_t)dst_addr, size, PATTERN_RANDOM,
                               0xDEADBEEF, &error_offset, &expected, &actual)) {
                errors++;
                LOG_RESULT("  WARNING: Data verification failed at offset %lu\r\n", error_offset);
            }
        }

        /* Update time */
        current_time = timer_get_us();
        elapsed_sec = (current_time - start_time) / 1000000;

        /* Periodic report */
        if ((current_time - last_report_time) >= (STRESS_REPORT_INTERVAL * 1000000ULL)) {
            double throughput = CALC_THROUGHPUT_MBPS(total_bytes, current_time - start_time);
            double gb_transferred = (double)total_bytes / (1024.0 * 1024.0 * 1024.0);

            LOG_RESULT("  %8lu | %9llu | %10.2f | %17.2f | %6lu\r\n",
                      elapsed_sec, total_transfers, gb_transferred, throughput, errors);

            last_report_time = current_time;
        }
    }

    /* Final report */
    current_time = timer_get_us();
    double final_throughput = CALC_THROUGHPUT_MBPS(total_bytes, current_time - start_time);
    double gb_transferred = (double)total_bytes / (1024.0 * 1024.0 * 1024.0);

    LOG_RESULT("\r\n");
    LOG_RESULT("=== Stress Test Results ===\r\n");
    LOG_RESULT("  Duration:          %lu seconds\r\n", elapsed_sec);
    LOG_RESULT("  Total Transfers:   %llu\r\n", total_transfers);
    LOG_RESULT("  Total Data:        %.2f GB\r\n", gb_transferred);
    LOG_RESULT("  Avg Throughput:    %.2f MB/s\r\n", final_throughput);
    LOG_RESULT("  Errors:            %lu\r\n", errors);
    LOG_RESULT("  Error Rate:        %.6f%%\r\n",
              total_transfers > 0 ? (100.0 * errors / total_transfers) : 0.0);
    LOG_RESULT("===========================\r\n");

    /* Update global stats */
    g_BenchmarkStats.tests_run++;
    if (errors == 0) {
        g_BenchmarkStats.tests_passed++;
    } else {
        g_BenchmarkStats.tests_failed++;
    }
    g_BenchmarkStats.total_bytes_transferred += total_bytes;

    return (errors == 0) ? DMA_SUCCESS : DMA_ERROR_VERIFY_FAIL;
}

int stress_test_random_pattern(uint32_t duration_sec)
{
    uint64_t src_addr, dst_addr;
    uint64_t start_time, current_time;
    uint32_t size = KB(256);
    uint32_t elapsed_sec = 0;
    uint32_t errors = 0;
    uint32_t transfer_count = 0;
    int status;

    src_addr = memory_get_test_addr(MEM_REGION_DDR4, MB(32), size);
    dst_addr = memory_get_test_addr(MEM_REGION_DDR4, MB(33), size);

    if (src_addr == 0 || dst_addr == 0) {
        return DMA_ERROR_NO_MEMORY;
    }

    LOG_RESULT("Running random pattern stress test...\r\n");

    start_time = timer_get_us();

    while (elapsed_sec < duration_sec && !g_TestAbort) {
        /* Generate new random pattern for each transfer */
        uint32_t seed = (uint32_t)(timer_get_us() & 0xFFFFFFFF);
        pattern_fill((void*)(uintptr_t)src_addr, size, PATTERN_RANDOM, seed);
        cache_prep_dma_src(src_addr, size);
        cache_prep_dma_dst(dst_addr, size);

        /* Transfer */
        status = axi_cdma_simple_transfer(src_addr, dst_addr, size);
        if (status == DMA_SUCCESS) {
            status = axi_cdma_wait_complete(DMA_TIMEOUT_US);
        }

        if (status == DMA_SUCCESS) {
            /* Verify */
            cache_complete_dma_dst(dst_addr, size);
            uint32_t error_offset;
            uint8_t expected, actual;
            if (!pattern_verify((void*)(uintptr_t)dst_addr, size, PATTERN_RANDOM,
                               seed, &error_offset, &expected, &actual)) {
                errors++;
            }
        } else {
            errors++;
        }

        transfer_count++;
        current_time = timer_get_us();
        elapsed_sec = (current_time - start_time) / 1000000;
    }

    LOG_RESULT("Random pattern test: %lu transfers, %lu errors\r\n",
              transfer_count, errors);

    return (errors == 0) ? DMA_SUCCESS : DMA_ERROR_VERIFY_FAIL;
}

int stress_test_multi_dma(uint32_t duration_sec)
{
    uint64_t start_time, current_time;
    uint32_t elapsed_sec = 0;
    uint32_t size = KB(64);
    uint64_t total_bytes = 0;
    uint32_t errors = 0;
    int status;

    /* Allocate buffers for each DMA */
    uint64_t cdma_src = memory_get_test_addr(MEM_REGION_DDR4, MB(64), size);
    uint64_t cdma_dst = memory_get_test_addr(MEM_REGION_DDR4, MB(65), size);
    uint64_t lpd_src = memory_get_test_addr(MEM_REGION_DDR4, MB(66), size);
    uint64_t lpd_dst = memory_get_test_addr(MEM_REGION_DDR4, MB(67), size);

    if (!cdma_src || !cdma_dst || !lpd_src || !lpd_dst) {
        return DMA_ERROR_NO_MEMORY;
    }

    /* Prepare buffers */
    pattern_fill((void*)(uintptr_t)cdma_src, size, PATTERN_INCREMENTAL, 0);
    pattern_fill((void*)(uintptr_t)lpd_src, size, PATTERN_CHECKERBOARD, 0);
    cache_prep_dma_src(cdma_src, size);
    cache_prep_dma_src(lpd_src, size);

    LOG_RESULT("Running multi-DMA stress test (CDMA + LPD DMA concurrent)...\r\n");

    start_time = timer_get_us();

    while (elapsed_sec < duration_sec && !g_TestAbort) {
        cache_prep_dma_dst(cdma_dst, size);
        cache_prep_dma_dst(lpd_dst, size);

        /* Start both DMAs concurrently */
        status = axi_cdma_simple_transfer(cdma_src, cdma_dst, size);
        if (status != DMA_SUCCESS) errors++;

        status = lpd_dma_transfer(0, lpd_src, lpd_dst, size);
        if (status != DMA_SUCCESS) errors++;

        /* Wait for both */
        status = axi_cdma_wait_complete(DMA_TIMEOUT_US);
        if (status != DMA_SUCCESS) errors++;

        status = lpd_dma_wait_complete(0, DMA_TIMEOUT_US);
        if (status != DMA_SUCCESS) errors++;

        total_bytes += size * 2;

        current_time = timer_get_us();
        elapsed_sec = (current_time - start_time) / 1000000;
    }

    double throughput = CALC_THROUGHPUT_MBPS(total_bytes, timer_get_us() - start_time);
    LOG_RESULT("Multi-DMA test: %.2f MB/s combined, %lu errors\r\n",
              throughput, errors);

    g_BenchmarkStats.total_bytes_transferred += total_bytes;

    return (errors == 0) ? DMA_SUCCESS : DMA_ERROR_DMA_FAIL;
}
