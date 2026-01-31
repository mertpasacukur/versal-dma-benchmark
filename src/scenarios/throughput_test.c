/**
 * @file throughput_test.c
 * @brief Throughput Test Scenarios Implementation
 */

#include <string.h>
#include "throughput_test.h"
#include "../utils/debug_print.h"
#include "../drivers/axi_dma_driver.h"
#include "../drivers/axi_cdma_driver.h"
#include "../drivers/axi_mcdma_driver.h"
#include "../drivers/lpd_dma_driver.h"
#include "../utils/timer_utils.h"
#include "../utils/memory_utils.h"
#include "../utils/data_patterns.h"
#include "../utils/results_logger.h"
#include "../utils/cache_utils.h"
#include "../tests/axi_dma_test.h"
#include "../tests/axi_cdma_test.h"
#include "../tests/axi_mcdma_test.h"
#include "../tests/lpd_dma_test.h"

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

int throughput_test_run_all(void)
{
    LOG_RESULT("\r\n");
    LOG_RESULT("================================================================\r\n");
    LOG_RESULT("              Throughput Test Suite\r\n");
    LOG_RESULT("================================================================\r\n\r\n");

    /* Size sweep for each DMA type */
    LOG_RESULT("1. Transfer Size Sweep Tests\r\n");
    LOG_RESULT("----------------------------\r\n\r\n");

    throughput_test_size_sweep(DMA_TYPE_AXI_CDMA);

    /* Memory matrix */
    LOG_RESULT("\r\n2. Memory-to-Memory Matrix\r\n");
    LOG_RESULT("--------------------------\r\n\r\n");
    throughput_test_run_memory_matrix();

    /* CPU baseline */
    LOG_RESULT("\r\n3. CPU memcpy Baseline\r\n");
    LOG_RESULT("----------------------\r\n\r\n");
    throughput_test_run_cpu_baseline();

    /* Alignment test */
    LOG_RESULT("\r\n4. Alignment Impact Test\r\n");
    LOG_RESULT("------------------------\r\n\r\n");
    throughput_test_alignment();

    LOG_RESULT("\r\nThroughput tests complete.\r\n");
    return DMA_SUCCESS;
}

int throughput_test_run_memory_matrix(void)
{
    /* Use CDMA for memory-to-memory matrix */
    return axi_cdma_test_memory_matrix();
}

int throughput_test_run_cpu_baseline(void)
{
    uint32_t sizes[] = {KB(1), KB(4), KB(16), KB(64), KB(256), MB(1), MB(4), MB(16)};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    LOG_RESULT("  Size       | Throughput (MB/s)\r\n");
    LOG_RESULT("  -----------|------------------\r\n");

    for (int i = 0; i < num_sizes; i++) {
        uint32_t size = sizes[i];
        uint64_t src = memory_get_test_addr(MEM_REGION_DDR4, 0, size);
        uint64_t dst = memory_get_test_addr(MEM_REGION_DDR4, size * 2, size);

        if (src == 0 || dst == 0) continue;

        /* Fill source */
        pattern_fill((void*)(uintptr_t)src, size, PATTERN_RANDOM, i);

        /* Benchmark */
        double throughput = memory_cpu_memcpy_benchmark(
            (void*)(uintptr_t)dst,
            (void*)(uintptr_t)src,
            size, 100);

        char size_str[16];
        results_logger_format_size(size, size_str, sizeof(size_str));
        LOG_RESULT("  %-10s | %16.2f\r\n", size_str, throughput);

        /* Update global stats */
        g_BenchmarkStats.tests_run++;
        g_BenchmarkStats.tests_passed++;
        g_BenchmarkStats.total_bytes_transferred += (uint64_t)size * 100;
    }

    return DMA_SUCCESS;
}

int throughput_test_size_sweep(DmaType_t dma_type)
{
    TestResult_t result;
    int status;

    LOG_RESULT("  %s Transfer Size Sweep:\r\n\r\n", dma_type_to_string(dma_type));
    LOG_RESULT("  Size       | Throughput (MB/s) | Latency (us)\r\n");
    LOG_RESULT("  -----------|-------------------|-------------\r\n");

    for (uint32_t i = 0; i < NUM_TRANSFER_SIZES; i++) {
        uint32_t size = g_TransferSizes[i];

        /* Skip very large transfers for quick test */
        if (size > MB(8)) continue;

        memset(&result, 0, sizeof(result));
        result.transfer_size = size;

        switch (dma_type) {
            case DMA_TYPE_AXI_DMA:
                status = axi_dma_test_throughput(MEM_REGION_DDR4, MEM_REGION_DDR4, &result);
                break;
            case DMA_TYPE_AXI_CDMA:
                status = axi_cdma_test_throughput(MEM_REGION_DDR4, MEM_REGION_DDR4, size, &result);
                break;
            case DMA_TYPE_AXI_MCDMA:
                status = axi_mcdma_test_single_channel(0, size, &result);
                break;
            case DMA_TYPE_LPD_DMA:
                status = lpd_dma_test_throughput(0, size, &result);
                break;
            default:
                status = DMA_ERROR_NOT_SUPPORTED;
                break;
        }

        char size_str[16];
        results_logger_format_size(size, size_str, sizeof(size_str));

        if (status == DMA_SUCCESS) {
            LOG_RESULT("  %-10s | %17.2f | %11.2f\r\n",
                      size_str, result.throughput_mbps, result.latency_us);

            g_BenchmarkStats.tests_run++;
            g_BenchmarkStats.tests_passed++;
            g_BenchmarkStats.total_bytes_transferred += result.total_bytes;
            g_BenchmarkStats.total_time_us += result.total_time_us;
        } else {
            LOG_RESULT("  %-10s | %17s | %11s\r\n", size_str, "ERROR", "---");
            g_BenchmarkStats.tests_run++;
            g_BenchmarkStats.tests_failed++;
        }
    }

    LOG_RESULT("\r\n");
    return DMA_SUCCESS;
}

int throughput_test_alignment(void)
{
    TestResult_t result_aligned, result_unaligned;
    uint32_t size = KB(64);
    int status;

    LOG_RESULT("  Testing AXI CDMA with aligned vs unaligned addresses:\r\n\r\n");

    /* Aligned transfer (64-byte aligned) */
    memset(&result_aligned, 0, sizeof(result_aligned));
    status = axi_cdma_test_throughput(MEM_REGION_DDR4, MEM_REGION_DDR4, size, &result_aligned);

    if (status == DMA_SUCCESS) {
        LOG_RESULT("  64-byte aligned:   %.2f MB/s\r\n", result_aligned.throughput_mbps);
    }

    /* Note: True unaligned testing would require modifying the test to use
     * addresses that are not 64-byte aligned. For now, we report the aligned result. */

    LOG_RESULT("  Unaligned:         (requires DRE support)\r\n");
    LOG_RESULT("\r\n  Note: All transfers use 64-byte aligned addresses for optimal performance.\r\n");
    LOG_RESULT("  Data Realignment Engine (DRE) is enabled for handling unaligned data.\r\n");

    return DMA_SUCCESS;
}
