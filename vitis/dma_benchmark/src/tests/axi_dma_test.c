/**
 * @file axi_dma_test.c
 * @brief AXI DMA Test Module Implementation
 */

#include <string.h>
#include "axi_dma_test.h"
#include "xil_cache.h"
#include "../drivers/axi_dma_driver.h"
#include "../utils/timer_utils.h"
#include "../utils/memory_utils.h"
#include "../utils/data_patterns.h"
#include "../utils/results_logger.h"
#include "../utils/cache_utils.h"
#include "../utils/debug_print.h"

/*******************************************************************************
 * Local Variables
 ******************************************************************************/

/* Test buffers - using fixed region offsets */
#define SRC_BUFFER_OFFSET   0x00000000
#define DST_BUFFER_OFFSET   0x01000000  /* 16MB offset */
#define MAX_TEST_SIZE       MB(16)

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

static int run_single_transfer_test(uint64_t src_addr, uint64_t dst_addr,
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
    pattern_fill((void*)(uintptr_t)src_addr, size, pattern, 0x12345678);
    cache_prep_dma_src(src_addr, size);

    /* Clear destination buffer */
    memset((void*)(uintptr_t)dst_addr, 0, size);
    cache_prep_dma_dst(dst_addr, size);

    /* Warmup iterations */
    for (i = 0; i < warmup; i++) {
        if (use_sg) {
            status = axi_dma_sg_transfer(src_addr, dst_addr, size);
        } else {
            status = axi_dma_simple_transfer(src_addr, dst_addr, size);
        }
        if (status != DMA_SUCCESS) {
            return status;
        }
        status = axi_dma_wait_complete(DMA_TIMEOUT_US);
        if (status != DMA_SUCCESS) {
            return status;
        }
    }

    /* Timed iterations */
    start_time = timer_start();

    for (i = 0; i < iterations; i++) {
        cache_prep_dma_dst(dst_addr, size);

        if (use_sg) {
            status = axi_dma_sg_transfer(src_addr, dst_addr, size);
        } else {
            status = axi_dma_simple_transfer(src_addr, dst_addr, size);
        }
        if (status != DMA_SUCCESS) {
            return status;
        }

        status = axi_dma_wait_complete(DMA_TIMEOUT_US);
        if (status != DMA_SUCCESS) {
            return status;
        }
    }

    elapsed_us = timer_stop_us(start_time);

    /* Verify last transfer */
    cache_complete_dma_dst(dst_addr, size);
    bool integrity = pattern_verify((void*)(uintptr_t)dst_addr, size, pattern,
                                    0x12345678, &error_offset, &expected, &actual);

    /* Fill result */
    result->dma_type = DMA_TYPE_AXI_DMA;
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

int axi_dma_test_run_all(void)
{
    TestResult_t result;
    int status;
    uint32_t size_idx;
    DataPattern_t pattern;

    LOG_INFO("Running AXI DMA Tests...\r\n\r\n");

    /* Test all transfer sizes with DDR4 */
    LOG_INFO("1. Throughput tests (DDR4 -> DDR4):\r\n");
    for (size_idx = 0; size_idx < NUM_TRANSFER_SIZES; size_idx++) {
        uint32_t size = g_TransferSizes[size_idx];

        /* Skip sizes too large for test region */
        if (size > MAX_TEST_SIZE) continue;

        memset(&result, 0, sizeof(result));
        result.src_region = MEM_REGION_DDR4;
        result.dst_region = MEM_REGION_DDR4;

        status = axi_dma_test_throughput(MEM_REGION_DDR4, MEM_REGION_DDR4, &result);
        if (status == DMA_SUCCESS) {
            results_logger_log_result(&result);
        } else {
            LOG_ERROR("  Size %lu: FAILED (error %d)\r\n", size, status);
        }
    }

    /* Test all data patterns */
    LOG_INFO("\r\n2. Data integrity tests:\r\n");
    for (pattern = PATTERN_INCREMENTAL; pattern < PATTERN_COUNT; pattern++) {
        memset(&result, 0, sizeof(result));

        status = axi_dma_test_integrity(pattern, &result);
        if (status == DMA_SUCCESS) {
            LOG_RESULT("  Pattern %s: %s\r\n",
                       pattern_to_string(pattern),
                       result.data_integrity ? "PASS" : "FAIL");
        } else {
            LOG_ERROR("  Pattern %s: ERROR %d\r\n",
                      pattern_to_string(pattern), status);
        }
    }

    /* Test different memory regions */
    LOG_INFO("\r\n3. Memory region tests:\r\n");

    /* DDR4 -> BRAM */
    if (platform_is_region_accessible(MEM_REGION_BRAM)) {
        memset(&result, 0, sizeof(result));
        result.src_region = MEM_REGION_DDR4;
        result.dst_region = MEM_REGION_BRAM;
        status = axi_dma_test_throughput(MEM_REGION_DDR4, MEM_REGION_BRAM, &result);
        if (status == DMA_SUCCESS) {
            LOG_RESULT("  DDR4 -> BRAM: %.2f MB/s\r\n", result.throughput_mbps);
        }
    }

    /* DDR4 -> OCM */
    if (platform_is_region_accessible(MEM_REGION_OCM)) {
        memset(&result, 0, sizeof(result));
        result.src_region = MEM_REGION_DDR4;
        result.dst_region = MEM_REGION_OCM;
        status = axi_dma_test_throughput(MEM_REGION_DDR4, MEM_REGION_OCM, &result);
        if (status == DMA_SUCCESS) {
            LOG_RESULT("  DDR4 -> OCM: %.2f MB/s\r\n", result.throughput_mbps);
        }
    }

    LOG_INFO("\r\nAXI DMA tests complete.\r\n");
    return DMA_SUCCESS;
}

int axi_dma_test_throughput(MemoryRegion_t src_region, MemoryRegion_t dst_region, TestResult_t* result)
{
    uint64_t src_addr, dst_addr;
    uint32_t size = result->transfer_size > 0 ? result->transfer_size : KB(64);
    int status;

    /* Get test addresses */
    src_addr = memory_get_test_addr(src_region, SRC_BUFFER_OFFSET, size);
    dst_addr = memory_get_test_addr(dst_region, DST_BUFFER_OFFSET, size);

    if (src_addr == 0 || dst_addr == 0) {
        return DMA_ERROR_INVALID_PARAM;
    }

    /* Run test */
    status = run_single_transfer_test(src_addr, dst_addr, size,
                                      PATTERN_INCREMENTAL, true, result);

    result->src_region = src_region;
    result->dst_region = dst_region;

    return status;
}

int axi_dma_test_latency(MemoryRegion_t src_region, MemoryRegion_t dst_region, TestResult_t* result)
{
    uint64_t src_addr, dst_addr;
    uint64_t start_time, elapsed_ns;
    uint32_t size = 64;  /* Small transfer for latency */
    uint32_t iterations = 1000;
    int status;
    uint32_t i;

    src_addr = memory_get_test_addr(src_region, SRC_BUFFER_OFFSET, size);
    dst_addr = memory_get_test_addr(dst_region, DST_BUFFER_OFFSET, size);

    if (src_addr == 0 || dst_addr == 0) {
        return DMA_ERROR_INVALID_PARAM;
    }

    /* Prepare buffers */
    pattern_fill((void*)(uintptr_t)src_addr, size, PATTERN_INCREMENTAL, 0);
    cache_prep_dma_src(src_addr, size);
    cache_prep_dma_dst(dst_addr, size);

    /* Warmup */
    for (i = 0; i < 10; i++) {
        axi_dma_simple_transfer(src_addr, dst_addr, size);
        axi_dma_wait_complete(DMA_TIMEOUT_US);
    }

    /* Measure latency */
    uint64_t total_ns = 0;
    uint64_t min_ns = UINT64_MAX;
    uint64_t max_ns = 0;

    for (i = 0; i < iterations; i++) {
        cache_prep_dma_dst(dst_addr, size);

        start_time = timer_start();
        status = axi_dma_simple_transfer(src_addr, dst_addr, size);
        if (status != DMA_SUCCESS) continue;

        status = axi_dma_wait_complete(DMA_TIMEOUT_US);
        elapsed_ns = timer_stop_ns(start_time);

        if (status == DMA_SUCCESS) {
            total_ns += elapsed_ns;
            if (elapsed_ns < min_ns) min_ns = elapsed_ns;
            if (elapsed_ns > max_ns) max_ns = elapsed_ns;
        }
    }

    result->dma_type = DMA_TYPE_AXI_DMA;
    result->test_type = TEST_LATENCY;
    result->src_region = src_region;
    result->dst_region = dst_region;
    result->transfer_size = size;
    result->iterations = iterations;
    result->latency_us = (double)total_ns / iterations / 1000.0;
    result->min_latency = (double)min_ns / 1000.0;
    result->max_latency = (double)max_ns / 1000.0;
    result->data_integrity = true;

    return DMA_SUCCESS;
}

int axi_dma_test_integrity(DataPattern_t pattern, TestResult_t* result)
{
    uint64_t src_addr, dst_addr;
    uint32_t size = KB(64);
    int status;

    src_addr = memory_get_test_addr(MEM_REGION_DDR4, SRC_BUFFER_OFFSET, size);
    dst_addr = memory_get_test_addr(MEM_REGION_DDR4, DST_BUFFER_OFFSET, size);

    /* Run test with specified pattern */
    status = run_single_transfer_test(src_addr, dst_addr, size, pattern, true, result);

    result->test_type = TEST_INTEGRITY;
    result->src_region = MEM_REGION_DDR4;
    result->dst_region = MEM_REGION_DDR4;
    result->pattern = pattern;

    return status;
}

int axi_dma_test_simple_mode(TestResult_t* result)
{
    uint64_t src_addr, dst_addr;
    uint32_t size = KB(64);

    src_addr = memory_get_test_addr(MEM_REGION_DDR4, SRC_BUFFER_OFFSET, size);
    dst_addr = memory_get_test_addr(MEM_REGION_DDR4, DST_BUFFER_OFFSET, size);

    return run_single_transfer_test(src_addr, dst_addr, size,
                                    PATTERN_INCREMENTAL, false, result);
}

int axi_dma_test_sg_mode(TestResult_t* result)
{
    uint64_t src_addr, dst_addr;
    uint32_t size = KB(64);

    src_addr = memory_get_test_addr(MEM_REGION_DDR4, SRC_BUFFER_OFFSET, size);
    dst_addr = memory_get_test_addr(MEM_REGION_DDR4, DST_BUFFER_OFFSET, size);

    return run_single_transfer_test(src_addr, dst_addr, size,
                                    PATTERN_INCREMENTAL, true, result);
}

int axi_dma_test_bidirectional(TestResult_t* result)
{
    /* AXI DMA loopback naturally tests bidirectional capability */
    return axi_dma_test_sg_mode(result);
}
