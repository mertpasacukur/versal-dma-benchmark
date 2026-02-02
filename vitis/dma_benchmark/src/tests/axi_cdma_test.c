/**
 * @file axi_cdma_test.c
 * @brief AXI CDMA Test Module Implementation
 */

#include <string.h>
#include "axi_cdma_test.h"
#include "../drivers/axi_cdma_driver.h"
#include "../utils/timer_utils.h"
#include "../utils/memory_utils.h"
#include "../utils/data_patterns.h"
#include "../utils/results_logger.h"
#include "../utils/cache_utils.h"
#include "../utils/debug_print.h"

/*******************************************************************************
 * Local Variables
 ******************************************************************************/

#define SRC_BUFFER_OFFSET   0x02000000  /* 32MB offset */
#define DST_BUFFER_OFFSET   0x03000000  /* 48MB offset */

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

static int run_cdma_transfer(uint64_t src_addr, uint64_t dst_addr,
                            uint32_t size, DataPattern_t pattern,
                            bool use_sg, TestResult_t* result)
{
    uint64_t start_time, elapsed_us;
    uint32_t iterations = DEFAULT_TEST_ITERATIONS;
    uint32_t warmup = WARMUP_ITERATIONS;
    uint32_t i, error_offset;
    int status;
    uint8_t expected, actual;

    /* Fill source buffer */
    pattern_fill((void*)(uintptr_t)src_addr, size, pattern, 0xABCDEF01);
    cache_prep_dma_src(src_addr, size);

    /* Clear destination buffer */
    memset((void*)(uintptr_t)dst_addr, 0, size);
    cache_prep_dma_dst(dst_addr, size);

    /* Warmup */
    for (i = 0; i < warmup; i++) {
        if (use_sg) {
            status = axi_cdma_sg_transfer(src_addr, dst_addr, size);
        } else {
            status = axi_cdma_simple_transfer(src_addr, dst_addr, size);
        }
        if (status != DMA_SUCCESS) return status;

        status = axi_cdma_wait_complete(DMA_TIMEOUT_US);
        if (status != DMA_SUCCESS) return status;
    }

    /* Timed iterations */
    start_time = timer_start();

    for (i = 0; i < iterations; i++) {
        cache_prep_dma_dst(dst_addr, size);

        if (use_sg) {
            status = axi_cdma_sg_transfer(src_addr, dst_addr, size);
        } else {
            status = axi_cdma_simple_transfer(src_addr, dst_addr, size);
        }
        if (status != DMA_SUCCESS) return status;

        status = axi_cdma_wait_complete(DMA_TIMEOUT_US);
        if (status != DMA_SUCCESS) return status;
    }

    elapsed_us = timer_stop_us(start_time);

    /* Verify data */
    cache_complete_dma_dst(dst_addr, size);
    bool integrity = pattern_verify((void*)(uintptr_t)dst_addr, size, pattern,
                                    0xABCDEF01, &error_offset, &expected, &actual);

    /* Fill result */
    result->dma_type = DMA_TYPE_AXI_CDMA;
    result->test_type = TEST_THROUGHPUT;
    result->pattern = pattern;
    result->mode = use_sg ? DMA_MODE_SG : DMA_MODE_SIMPLE;
    result->transfer_size = size;
    result->iterations = iterations;
    result->total_bytes = (uint64_t)size * iterations;
    result->total_time_us = elapsed_us;
    result->throughput_mbps = CALC_THROUGHPUT_MBPS(result->total_bytes, elapsed_us);
    result->latency_us = (double)elapsed_us / iterations;
    result->data_integrity = integrity;
    result->error_count = integrity ? 0 : 1;
    result->first_error_offset = integrity ? 0 : error_offset;

    return DMA_SUCCESS;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

int axi_cdma_test_run_all(void)
{
    TestResult_t result;
    int status;

    LOG_INFO("Running AXI CDMA Tests...\r\n\r\n");

    /* Test various transfer sizes */
    LOG_INFO("1. Throughput tests (DDR4 -> DDR4):\r\n");
    uint32_t test_sizes[] = {KB(1), KB(4), KB(16), KB(64), KB(256), MB(1), MB(4)};
    int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

    for (int i = 0; i < num_sizes; i++) {
        memset(&result, 0, sizeof(result));
        status = axi_cdma_test_throughput(MEM_REGION_DDR4, MEM_REGION_DDR4,
                                         test_sizes[i], &result);
        if (status == DMA_SUCCESS) {
            char size_str[32];
            results_logger_format_size(test_sizes[i], size_str, sizeof(size_str));
            LOG_RESULT("  Size %s: %.2f MB/s\r\n", size_str, result.throughput_mbps);
        }
    }

    /* Test memory-to-memory matrix */
    LOG_INFO("\r\n2. Memory-to-Memory Matrix:\r\n");
    axi_cdma_test_memory_matrix();

    /* Test data integrity with all patterns */
    LOG_INFO("\r\n3. Data Integrity Tests:\r\n");
    for (DataPattern_t p = PATTERN_INCREMENTAL; p < PATTERN_COUNT; p++) {
        memset(&result, 0, sizeof(result));
        status = axi_cdma_test_integrity(p, &result);
        LOG_RESULT("  %s: %s\r\n", pattern_to_string(p),
                   (status == DMA_SUCCESS && result.data_integrity) ? "PASS" : "FAIL");
    }

    LOG_INFO("\r\nAXI CDMA tests complete.\r\n");
    return DMA_SUCCESS;
}

int axi_cdma_test_throughput(MemoryRegion_t src_region, MemoryRegion_t dst_region,
                            uint32_t size, TestResult_t* result)
{
    uint64_t src_addr, dst_addr;

    /* Determine buffer offset based on region */
    uint32_t src_offset = SRC_BUFFER_OFFSET;
    uint32_t dst_offset = DST_BUFFER_OFFSET;

    /* Adjust for smaller regions */
    if (src_region == MEM_REGION_OCM || src_region == MEM_REGION_BRAM ||
        src_region == MEM_REGION_URAM) {
        src_offset = 0;
    }
    if (dst_region == MEM_REGION_OCM || dst_region == MEM_REGION_BRAM ||
        dst_region == MEM_REGION_URAM) {
        dst_offset = size;  /* Use second half of small regions */
    }

    src_addr = memory_get_test_addr(src_region, src_offset, size);
    dst_addr = memory_get_test_addr(dst_region, dst_offset, size);

    if (src_addr == 0 || dst_addr == 0) {
        return DMA_ERROR_INVALID_PARAM;
    }

    int status = run_cdma_transfer(src_addr, dst_addr, size,
                                   PATTERN_INCREMENTAL, false, result);

    result->src_region = src_region;
    result->dst_region = dst_region;

    return status;
}

int axi_cdma_test_latency(MemoryRegion_t src_region, MemoryRegion_t dst_region,
                         TestResult_t* result)
{
    uint64_t src_addr, dst_addr;
    uint32_t size = 64;  /* Small transfer */
    uint32_t iterations = 1000;
    uint64_t total_ns = 0;
    int status;

    src_addr = memory_get_test_addr(src_region, 0, size);
    dst_addr = memory_get_test_addr(dst_region, size * 2, size);

    if (src_addr == 0 || dst_addr == 0) {
        return DMA_ERROR_INVALID_PARAM;
    }

    /* Prepare */
    pattern_fill((void*)(uintptr_t)src_addr, size, PATTERN_INCREMENTAL, 0);
    cache_prep_dma_src(src_addr, size);

    /* Measure */
    for (uint32_t i = 0; i < iterations; i++) {
        cache_prep_dma_dst(dst_addr, size);

        uint64_t start = timer_start();
        status = axi_cdma_simple_transfer(src_addr, dst_addr, size);
        if (status == DMA_SUCCESS) {
            status = axi_cdma_wait_complete(DMA_TIMEOUT_US);
        }
        total_ns += timer_stop_ns(start);
    }

    result->dma_type = DMA_TYPE_AXI_CDMA;
    result->test_type = TEST_LATENCY;
    result->src_region = src_region;
    result->dst_region = dst_region;
    result->transfer_size = size;
    result->iterations = iterations;
    result->latency_us = (double)total_ns / iterations / 1000.0;
    result->data_integrity = true;

    return DMA_SUCCESS;
}

int axi_cdma_test_integrity(DataPattern_t pattern, TestResult_t* result)
{
    uint64_t src_addr, dst_addr;
    uint32_t size = KB(64);

    src_addr = memory_get_test_addr(MEM_REGION_DDR4, SRC_BUFFER_OFFSET, size);
    dst_addr = memory_get_test_addr(MEM_REGION_DDR4, DST_BUFFER_OFFSET, size);

    int status = run_cdma_transfer(src_addr, dst_addr, size, pattern, false, result);

    result->test_type = TEST_INTEGRITY;
    result->src_region = MEM_REGION_DDR4;
    result->dst_region = MEM_REGION_DDR4;

    return status;
}

int axi_cdma_test_memory_matrix(void)
{
    TestResult_t result;
    int status;
    uint32_t size = KB(32);  /* Use smaller size for PL memories */

    /* Test regions that support CDMA access */
    MemoryRegion_t regions[] = {
        MEM_REGION_DDR4,
        MEM_REGION_LPDDR4,
        MEM_REGION_OCM,
        MEM_REGION_BRAM,
        MEM_REGION_URAM
    };
    int num_regions = sizeof(regions) / sizeof(regions[0]);

    LOG_RESULT("  Transfer Matrix (MB/s):\r\n");
    LOG_RESULT("  %10s", "From\\To");
    for (int j = 0; j < num_regions; j++) {
        LOG_RESULT(" %8s", memory_region_to_string(regions[j]));
    }
    LOG_RESULT("\r\n");

    for (int i = 0; i < num_regions; i++) {
        LOG_RESULT("  %10s", memory_region_to_string(regions[i]));

        for (int j = 0; j < num_regions; j++) {
            /* Adjust size for smaller regions */
            uint32_t test_size = size;
            uint64_t max_src = memory_get_max_size(regions[i]);
            uint64_t max_dst = memory_get_max_size(regions[j]);

            if (max_src < size * 2 || max_dst < size * 2) {
                test_size = MIN(max_src, max_dst) / 4;
                if (test_size < 64) test_size = 64;
            }

            memset(&result, 0, sizeof(result));
            status = axi_cdma_test_throughput(regions[i], regions[j], test_size, &result);

            if (status == DMA_SUCCESS && result.data_integrity) {
                LOG_RESULT(" %8.1f", result.throughput_mbps);
            } else {
                LOG_RESULT(" %8s", "---");
            }
        }
        LOG_RESULT("\r\n");
    }

    return DMA_SUCCESS;
}
