/**
 * @file comparison_test.c
 * @brief DMA Comparison Test Module Implementation
 */

#include <string.h>
#include "comparison_test.h"
#include "axi_dma_test.h"
#include "axi_cdma_test.h"
#include "axi_mcdma_test.h"
#include "lpd_dma_test.h"
#include "../utils/debug_print.h"
#include "../utils/timer_utils.h"
#include "../utils/memory_utils.h"
#include "../utils/data_patterns.h"
#include "../utils/cache_utils.h"

/*******************************************************************************
 * Local Types
 ******************************************************************************/

typedef struct {
    DmaType_t type;
    uint32_t throughput_mbps;
    uint32_t latency_us;
    bool tested;
} DmaComparisonResult_t;

/*******************************************************************************
 * Local Variables
 ******************************************************************************/

static DmaComparisonResult_t g_ComparisonResults[DMA_TYPE_COUNT];

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

int comparison_test_run(void)
{
    LOG_RESULT("Running DMA Comparison Tests...\r\n\r\n");

    /* Clear results */
    memset(g_ComparisonResults, 0, sizeof(g_ComparisonResults));

    /* Test various sizes */
    LOG_RESULT("1. Throughput Comparison by Transfer Size:\r\n\r\n");

    uint32_t test_sizes[] = {KB(1), KB(4), KB(16), KB(64), KB(256), MB(1), MB(4)};
    int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

    /* Print header */
    LOG_RESULT("  %10s", "Size");
    LOG_RESULT(" %10s", "AXI_DMA");
    LOG_RESULT(" %10s", "AXI_CDMA");
    LOG_RESULT(" %10s", "AXI_MCDMA");
    LOG_RESULT(" %10s", "LPD_DMA");
    LOG_RESULT(" %10s", "CPU_MEMCPY");
    LOG_RESULT("\r\n");
    LOG_RESULT("  ---------- ---------- ---------- ---------- ---------- ----------\r\n");

    for (int i = 0; i < num_sizes; i++) {
        comparison_test_throughput(test_sizes[i]);
    }

    /* Latency comparison */
    LOG_RESULT("\r\n2. Latency Comparison (64-byte transfers):\r\n");
    comparison_test_latency();

    /* CPU comparison */
    LOG_RESULT("\r\n3. CPU memcpy vs DMA Engines:\r\n");
    comparison_test_vs_cpu();

    /* Summary */
    LOG_RESULT("\r\n");
    comparison_test_print_summary();

    return DMA_SUCCESS;
}

int comparison_test_throughput(uint32_t size)
{
    TestResult_t result;
    char size_str[16];
    uint32_t results[DMA_TYPE_COUNT] = {0};

    results_logger_format_size(size, size_str, sizeof(size_str));
    LOG_RESULT("  %10s", size_str);

    /* AXI DMA */
    memset(&result, 0, sizeof(result));
    result.transfer_size = size;
    if (axi_dma_test_throughput(MEM_REGION_DDR4, MEM_REGION_DDR4, &result) == DMA_SUCCESS) {
        results[DMA_TYPE_AXI_DMA] = result.throughput_mbps;
        LOG_RESULT(" %10lu", (unsigned long)result.throughput_mbps);
    } else {
        LOG_RESULT(" %10s", "---");
    }

    /* AXI CDMA */
    memset(&result, 0, sizeof(result));
    if (axi_cdma_test_throughput(MEM_REGION_DDR4, MEM_REGION_DDR4, size, &result) == DMA_SUCCESS) {
        results[DMA_TYPE_AXI_CDMA] = result.throughput_mbps;
        LOG_RESULT(" %10lu", (unsigned long)result.throughput_mbps);
    } else {
        LOG_RESULT(" %10s", "---");
    }

    /* AXI MCDMA (single channel) */
    memset(&result, 0, sizeof(result));
    if (axi_mcdma_test_single_channel(0, size, &result) == DMA_SUCCESS) {
        results[DMA_TYPE_AXI_MCDMA] = result.throughput_mbps;
        LOG_RESULT(" %10lu", (unsigned long)result.throughput_mbps);
    } else {
        LOG_RESULT(" %10s", "---");
    }

    /* LPD DMA */
    memset(&result, 0, sizeof(result));
    if (lpd_dma_test_throughput(0, size, &result) == DMA_SUCCESS) {
        results[DMA_TYPE_LPD_DMA] = result.throughput_mbps;
        LOG_RESULT(" %10lu", (unsigned long)result.throughput_mbps);
    } else {
        LOG_RESULT(" %10s", "---");
    }

    /* CPU memcpy */
    uint64_t src = memory_get_test_addr(MEM_REGION_DDR4, 0, size);
    uint64_t dst = memory_get_test_addr(MEM_REGION_DDR4, size * 2, size);
    if (src && dst) {
        uint32_t cpu_tp = (uint32_t)memory_cpu_memcpy_benchmark((void*)(uintptr_t)dst,
                                                     (void*)(uintptr_t)src,
                                                     size, 100);
        results[DMA_TYPE_CPU_MEMCPY] = cpu_tp;
        LOG_RESULT(" %10lu", (unsigned long)cpu_tp);
    } else {
        LOG_RESULT(" %10s", "---");
    }

    LOG_RESULT("\r\n");

    /* Update best results */
    for (int t = 0; t < DMA_TYPE_COUNT; t++) {
        if (results[t] > g_ComparisonResults[t].throughput_mbps) {
            g_ComparisonResults[t].throughput_mbps = results[t];
            g_ComparisonResults[t].tested = true;
        }
    }

    return DMA_SUCCESS;
}

int comparison_test_latency(void)
{
    TestResult_t result;
    uint32_t latencies[DMA_TYPE_COUNT] = {0};

    LOG_RESULT("  DMA Type     | Latency (us)\r\n");
    LOG_RESULT("  -------------|-------------\r\n");

    /* AXI DMA */
    memset(&result, 0, sizeof(result));
    if (axi_dma_test_latency(MEM_REGION_DDR4, MEM_REGION_DDR4, &result) == DMA_SUCCESS) {
        latencies[DMA_TYPE_AXI_DMA] = result.latency_us;
        LOG_RESULT("  AXI_DMA      | %11lu\r\n", (unsigned long)result.latency_us);
    }

    /* AXI CDMA */
    memset(&result, 0, sizeof(result));
    if (axi_cdma_test_latency(MEM_REGION_DDR4, MEM_REGION_DDR4, &result) == DMA_SUCCESS) {
        latencies[DMA_TYPE_AXI_CDMA] = result.latency_us;
        LOG_RESULT("  AXI_CDMA     | %11lu\r\n", (unsigned long)result.latency_us);
    }

    /* LPD DMA */
    memset(&result, 0, sizeof(result));
    if (lpd_dma_test_latency(0, &result) == DMA_SUCCESS) {
        latencies[DMA_TYPE_LPD_DMA] = result.latency_us;
        LOG_RESULT("  LPD_DMA      | %11lu\r\n", (unsigned long)result.latency_us);
    }

    /* Update results */
    for (int t = 0; t < DMA_TYPE_COUNT; t++) {
        if (latencies[t] > 0) {
            g_ComparisonResults[t].latency_us = latencies[t];
        }
    }

    return DMA_SUCCESS;
}

int comparison_test_vs_cpu(void)
{
    TestResult_t result;
    uint32_t size = MB(1);
    uint32_t cpu_throughput;
    uint32_t best_dma_throughput = 0;
    DmaType_t best_dma = DMA_TYPE_COUNT;

    /* CPU baseline */
    uint64_t src = memory_get_test_addr(MEM_REGION_DDR4, 0, size);
    uint64_t dst = memory_get_test_addr(MEM_REGION_DDR4, size * 2, size);

    pattern_fill((void*)(uintptr_t)src, size, PATTERN_RANDOM, 0);
    cpu_throughput = (uint32_t)memory_cpu_memcpy_benchmark((void*)(uintptr_t)dst,
                                                  (void*)(uintptr_t)src,
                                                  size, 50);

    LOG_RESULT("  CPU memcpy (1MB):      %lu MB/s\r\n", (unsigned long)cpu_throughput);

    /* Best DMA */
    memset(&result, 0, sizeof(result));
    if (axi_cdma_test_throughput(MEM_REGION_DDR4, MEM_REGION_DDR4, size, &result) == DMA_SUCCESS) {
        if (result.throughput_mbps > best_dma_throughput) {
            best_dma_throughput = result.throughput_mbps;
            best_dma = DMA_TYPE_AXI_CDMA;
        }
    }

    LOG_RESULT("  Best DMA (%s): %lu MB/s\r\n",
              dma_type_to_string(best_dma), (unsigned long)best_dma_throughput);

    if (best_dma_throughput > 0 && cpu_throughput > 0) {
        uint32_t speedup = best_dma_throughput / cpu_throughput;
        LOG_RESULT("  DMA Speedup:           %lux\r\n", (unsigned long)speedup);
    }

    return DMA_SUCCESS;
}

void comparison_test_print_summary(void)
{
    LOG_RESULT("=== DMA Comparison Summary ===\r\n\r\n");
    LOG_RESULT("  DMA Type     | Max Throughput | Min Latency | Best Use Case\r\n");
    LOG_RESULT("  -------------|----------------|-------------|------------------------\r\n");

    const char* use_cases[] = {
        [DMA_TYPE_AXI_DMA]    = "Stream peripherals, loopback",
        [DMA_TYPE_AXI_CDMA]   = "Memory-to-memory copy",
        [DMA_TYPE_AXI_MCDMA]  = "Multi-stream applications",
        [DMA_TYPE_LPD_DMA]    = "Low-power transfers",
        [DMA_TYPE_QDMA]       = "Host-device exchange",
        [DMA_TYPE_CPU_MEMCPY] = "Small transfers, flexibility"
    };

    for (int t = 0; t < DMA_TYPE_COUNT; t++) {
        if (g_ComparisonResults[t].tested || g_ComparisonResults[t].throughput_mbps > 0) {
            LOG_RESULT("  %-13s| %11lu MB/s| %8lu us | %s\r\n",
                      dma_type_to_string((DmaType_t)t),
                      (unsigned long)g_ComparisonResults[t].throughput_mbps,
                      (unsigned long)g_ComparisonResults[t].latency_us,
                      use_cases[t] ? use_cases[t] : "N/A");
        }
    }

    LOG_RESULT("\r\n");

    /* Find best */
    uint32_t best_tp = 0;
    DmaType_t best = DMA_TYPE_COUNT;
    for (int t = 0; t < DMA_TYPE_COUNT; t++) {
        if (g_ComparisonResults[t].throughput_mbps > best_tp) {
            best_tp = g_ComparisonResults[t].throughput_mbps;
            best = (DmaType_t)t;
        }
    }

    if (best < DMA_TYPE_COUNT) {
        LOG_RESULT("  Highest Throughput: %s (%lu MB/s)\r\n",
                  dma_type_to_string(best), (unsigned long)best_tp);
    }

    uint32_t best_lat = 0xFFFFFFFF;
    best = DMA_TYPE_COUNT;
    for (int t = 0; t < DMA_TYPE_COUNT; t++) {
        if (g_ComparisonResults[t].latency_us > 0 &&
            g_ComparisonResults[t].latency_us < best_lat) {
            best_lat = g_ComparisonResults[t].latency_us;
            best = (DmaType_t)t;
        }
    }

    if (best < DMA_TYPE_COUNT) {
        LOG_RESULT("  Lowest Latency:     %s (%lu us)\r\n",
                  dma_type_to_string(best), (unsigned long)best_lat);
    }

    LOG_RESULT("==============================\r\n");
}
