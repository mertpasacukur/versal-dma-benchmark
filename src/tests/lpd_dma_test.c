/**
 * @file lpd_dma_test.c
 * @brief LPD DMA Test Module Implementation
 */

#include <string.h>
#include "lpd_dma_test.h"
#include "xil_cache.h"
#include "../utils/debug_print.h"
#include "../drivers/lpd_dma_driver.h"
#include "../utils/timer_utils.h"
#include "../utils/memory_utils.h"
#include "../utils/data_patterns.h"
#include "../utils/results_logger.h"
#include "../utils/cache_utils.h"

/*******************************************************************************
 * Local Variables
 ******************************************************************************/

#define LPD_SRC_OFFSET  0x06000000  /* 96MB */
#define LPD_DST_OFFSET  0x07000000  /* 112MB */

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

int lpd_dma_test_run_all(void)
{
    TestResult_t result;
    int status;

    LOG_RESULT("Running LPD DMA (ADMA) Tests...\r\n\r\n");

    /* Single channel throughput tests */
    LOG_RESULT("1. Single Channel Throughput (CH0):\r\n");
    uint32_t sizes[] = {KB(1), KB(4), KB(16), KB(64), KB(256)};
    for (int i = 0; i < 5; i++) {
        memset(&result, 0, sizeof(result));
        status = lpd_dma_test_throughput(0, sizes[i], &result);
        if (status == DMA_SUCCESS) {
            char size_str[32];
            results_logger_format_size(sizes[i], size_str, sizeof(size_str));
            LOG_RESULT("  Size %s: %.2f MB/s\r\n", size_str, result.throughput_mbps);
        } else {
            LOG_RESULT("  Size %lu: ERROR %d\r\n", sizes[i], status);
        }
    }

    /* Test all 8 channels */
    LOG_RESULT("\r\n2. Per-Channel Test (64KB transfers):\r\n");
    for (uint32_t ch = 0; ch < LPD_DMA_NUM_CHANNELS; ch++) {
        memset(&result, 0, sizeof(result));
        status = lpd_dma_test_throughput(ch, KB(64), &result);
        if (status == DMA_SUCCESS) {
            LOG_RESULT("  CH%d: %.2f MB/s\r\n", ch, result.throughput_mbps);
        } else {
            LOG_RESULT("  CH%d: ERROR\r\n", ch);
        }
    }

    /* Multi-channel concurrent test */
    LOG_RESULT("\r\n3. Multi-Channel Concurrent:\r\n");
    uint32_t ch_counts[] = {1, 2, 4, 8};
    for (int i = 0; i < 4; i++) {
        memset(&result, 0, sizeof(result));
        status = lpd_dma_test_multi_channel(ch_counts[i], KB(64), &result);
        if (status == DMA_SUCCESS) {
            LOG_RESULT("  %d channels: %.2f MB/s aggregate\r\n",
                      ch_counts[i], result.throughput_mbps);
        }
    }

    /* Data integrity tests */
    LOG_RESULT("\r\n4. Data Integrity:\r\n");
    for (DataPattern_t p = PATTERN_INCREMENTAL; p < PATTERN_COUNT; p++) {
        memset(&result, 0, sizeof(result));
        status = lpd_dma_test_integrity(p, &result);
        LOG_RESULT("  %s: %s\r\n", pattern_to_string(p),
                  (status == DMA_SUCCESS && result.data_integrity) ? "PASS" : "FAIL");
    }

    LOG_RESULT("\r\nLPD DMA tests complete.\r\n");
    return DMA_SUCCESS;
}

int lpd_dma_test_throughput(uint32_t channel, uint32_t size, TestResult_t* result)
{
    uint64_t src_addr, dst_addr;
    uint64_t start_time, elapsed_us;
    uint32_t iterations = DEFAULT_TEST_ITERATIONS;
    uint32_t i;
    int status;

    if (channel >= LPD_DMA_NUM_CHANNELS) {
        return DMA_ERROR_INVALID_PARAM;
    }

    /* Use OCM for faster LPD DMA access, or DDR if size too large */
    MemoryRegion_t region = (size <= OCM_SIZE / 4) ? MEM_REGION_OCM : MEM_REGION_DDR4;
    uint32_t src_off = (region == MEM_REGION_OCM) ? 0 : LPD_SRC_OFFSET;
    uint32_t dst_off = (region == MEM_REGION_OCM) ? size * 2 : LPD_DST_OFFSET;

    src_addr = memory_get_test_addr(region, src_off, size);
    dst_addr = memory_get_test_addr(region, dst_off, size);

    if (src_addr == 0 || dst_addr == 0) {
        /* Fall back to DDR */
        src_addr = memory_get_test_addr(MEM_REGION_DDR4, LPD_SRC_OFFSET, size);
        dst_addr = memory_get_test_addr(MEM_REGION_DDR4, LPD_DST_OFFSET, size);
        region = MEM_REGION_DDR4;
    }

    /* Prepare buffers */
    pattern_fill((void*)(uintptr_t)src_addr, size, PATTERN_INCREMENTAL, channel);
    cache_prep_dma_src(src_addr, size);
    memset((void*)(uintptr_t)dst_addr, 0, size);
    cache_prep_dma_dst(dst_addr, size);

    /* Warmup */
    for (i = 0; i < WARMUP_ITERATIONS; i++) {
        status = lpd_dma_transfer(channel, src_addr, dst_addr, size);
        if (status != DMA_SUCCESS) return status;
        status = lpd_dma_wait_complete(channel, DMA_TIMEOUT_US);
        if (status != DMA_SUCCESS) return status;
    }

    /* Timed test */
    start_time = timer_start();

    for (i = 0; i < iterations; i++) {
        cache_prep_dma_dst(dst_addr, size);

        status = lpd_dma_transfer(channel, src_addr, dst_addr, size);
        if (status != DMA_SUCCESS) return status;

        status = lpd_dma_wait_complete(channel, DMA_TIMEOUT_US);
        if (status != DMA_SUCCESS) return status;
    }

    elapsed_us = timer_stop_us(start_time);

    /* Verify */
    cache_complete_dma_dst(dst_addr, size);
    uint32_t error_offset;
    uint8_t expected, actual;
    bool integrity = pattern_verify((void*)(uintptr_t)dst_addr, size,
                                    PATTERN_INCREMENTAL, channel,
                                    &error_offset, &expected, &actual);

    /* Fill result */
    result->dma_type = DMA_TYPE_LPD_DMA;
    result->test_type = TEST_THROUGHPUT;
    result->src_region = region;
    result->dst_region = region;
    result->transfer_size = size;
    result->iterations = iterations;
    result->total_bytes = (uint64_t)size * iterations;
    result->total_time_us = elapsed_us;
    result->throughput_mbps = CALC_THROUGHPUT_MBPS(result->total_bytes, elapsed_us);
    result->latency_us = (double)elapsed_us / iterations;
    result->data_integrity = integrity;

    return DMA_SUCCESS;
}

int lpd_dma_test_latency(uint32_t channel, TestResult_t* result)
{
    uint64_t src_addr, dst_addr;
    uint32_t size = 64;
    uint32_t iterations = 1000;
    uint64_t total_ns = 0;
    int status;

    src_addr = memory_get_test_addr(MEM_REGION_OCM, 0, size);
    dst_addr = memory_get_test_addr(MEM_REGION_OCM, size * 2, size);

    if (src_addr == 0 || dst_addr == 0) {
        return DMA_ERROR_INVALID_PARAM;
    }

    pattern_fill((void*)(uintptr_t)src_addr, size, PATTERN_INCREMENTAL, 0);

    for (uint32_t i = 0; i < iterations; i++) {
        uint64_t start = timer_start();
        status = lpd_dma_transfer(channel, src_addr, dst_addr, size);
        if (status == DMA_SUCCESS) {
            status = lpd_dma_wait_complete(channel, DMA_TIMEOUT_US);
        }
        total_ns += timer_stop_ns(start);
    }

    result->dma_type = DMA_TYPE_LPD_DMA;
    result->test_type = TEST_LATENCY;
    result->transfer_size = size;
    result->iterations = iterations;
    result->latency_us = (double)total_ns / iterations / 1000.0;
    result->data_integrity = true;

    return DMA_SUCCESS;
}

int lpd_dma_test_multi_channel(uint32_t num_channels, uint32_t size, TestResult_t* result)
{
    uint64_t src_addrs[LPD_DMA_NUM_CHANNELS];
    uint64_t dst_addrs[LPD_DMA_NUM_CHANNELS];
    uint64_t start_time, elapsed_us;
    uint32_t iterations = DEFAULT_TEST_ITERATIONS / num_channels;
    uint32_t ch, i;
    int status;

    if (num_channels > LPD_DMA_NUM_CHANNELS) {
        num_channels = LPD_DMA_NUM_CHANNELS;
    }

    /* Setup addresses */
    for (ch = 0; ch < num_channels; ch++) {
        uint32_t offset = LPD_SRC_OFFSET + (ch * size * 4);
        src_addrs[ch] = memory_get_test_addr(MEM_REGION_DDR4, offset, size);
        dst_addrs[ch] = memory_get_test_addr(MEM_REGION_DDR4, offset + size * 2, size);

        pattern_fill((void*)(uintptr_t)src_addrs[ch], size, PATTERN_RANDOM, ch);
        cache_prep_dma_src(src_addrs[ch], size);
        cache_prep_dma_dst(dst_addrs[ch], size);
    }

    /* Warmup */
    for (i = 0; i < WARMUP_ITERATIONS; i++) {
        for (ch = 0; ch < num_channels; ch++) {
            lpd_dma_transfer(ch, src_addrs[ch], dst_addrs[ch], size);
        }
        for (ch = 0; ch < num_channels; ch++) {
            lpd_dma_wait_complete(ch, DMA_TIMEOUT_US);
        }
    }

    /* Timed test */
    start_time = timer_start();

    for (i = 0; i < iterations; i++) {
        for (ch = 0; ch < num_channels; ch++) {
            cache_prep_dma_dst(dst_addrs[ch], size);
        }

        /* Start all channels */
        for (ch = 0; ch < num_channels; ch++) {
            status = lpd_dma_transfer(ch, src_addrs[ch], dst_addrs[ch], size);
            if (status != DMA_SUCCESS) return status;
        }

        /* Wait all */
        for (ch = 0; ch < num_channels; ch++) {
            status = lpd_dma_wait_complete(ch, DMA_TIMEOUT_US);
            if (status != DMA_SUCCESS) return status;
        }
    }

    elapsed_us = timer_stop_us(start_time);

    uint64_t total_bytes = (uint64_t)size * iterations * num_channels;

    result->dma_type = DMA_TYPE_LPD_DMA;
    result->test_type = TEST_MULTICHANNEL;
    result->transfer_size = size;
    result->iterations = iterations;
    result->num_channels = num_channels;
    result->total_bytes = total_bytes;
    result->total_time_us = elapsed_us;
    result->throughput_mbps = CALC_THROUGHPUT_MBPS(total_bytes, elapsed_us);
    result->data_integrity = true;

    return DMA_SUCCESS;
}

int lpd_dma_test_integrity(DataPattern_t pattern, TestResult_t* result)
{
    uint64_t src_addr, dst_addr;
    uint32_t size = KB(16);
    int status;

    src_addr = memory_get_test_addr(MEM_REGION_DDR4, LPD_SRC_OFFSET, size);
    dst_addr = memory_get_test_addr(MEM_REGION_DDR4, LPD_DST_OFFSET, size);

    LOG_DEBUG("LPD DMA Integrity: pattern=%d, src=0x%llX, dst=0x%llX, size=%lu\r\n",
              (int)pattern, (unsigned long long)src_addr, (unsigned long long)dst_addr,
              (unsigned long)size);

    /* Fill source and clear destination */
    pattern_fill((void*)(uintptr_t)src_addr, size, pattern, 0x55AA55AA);
    memset((void*)(uintptr_t)dst_addr, 0xDE, size);  /* Fill with 0xDE to detect no-transfer */

    /* Flush source to memory, invalidate destination from cache */
    Xil_DCacheFlushRange(src_addr, size);
    Xil_DCacheInvalidateRange(dst_addr, size);

    /* Memory barrier */
    __asm__ __volatile__("dsb sy" ::: "memory");

    status = lpd_dma_transfer(0, src_addr, dst_addr, size);
    if (status != DMA_SUCCESS) {
        LOG_ERROR("LPD DMA Integrity: transfer failed with %d\r\n", status);
        return status;
    }

    status = lpd_dma_wait_complete(0, DMA_TIMEOUT_US);
    if (status != DMA_SUCCESS) {
        LOG_ERROR("LPD DMA Integrity: wait failed with %d\r\n", status);
        return status;
    }

    /* Memory barrier and invalidate cache to read fresh data */
    __asm__ __volatile__("dsb sy" ::: "memory");
    Xil_DCacheInvalidateRange(dst_addr, size);

    /* Debug: check first few bytes */
    uint8_t* dst_ptr = (uint8_t*)(uintptr_t)dst_addr;
    LOG_DEBUG("LPD DMA Integrity: dst[0..7] = %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
              dst_ptr[0], dst_ptr[1], dst_ptr[2], dst_ptr[3],
              dst_ptr[4], dst_ptr[5], dst_ptr[6], dst_ptr[7]);

    /* Verify */
    uint32_t error_offset;
    uint8_t expected, actual;
    bool integrity = pattern_verify((void*)(uintptr_t)dst_addr, size, pattern,
                                    0x55AA55AA, &error_offset, &expected, &actual);

    if (!integrity) {
        LOG_DEBUG("LPD DMA Integrity: FAIL at offset %lu, expected=0x%02X, actual=0x%02X\r\n",
                  (unsigned long)error_offset, expected, actual);
    }

    result->dma_type = DMA_TYPE_LPD_DMA;
    result->test_type = TEST_INTEGRITY;
    result->pattern = pattern;
    result->transfer_size = size;
    result->data_integrity = integrity;
    result->error_count = integrity ? 0 : 1;
    result->first_error_offset = integrity ? 0 : error_offset;

    return DMA_SUCCESS;
}
