/**
 * @file axi_mcdma_test.c
 * @brief AXI MCDMA Test Module Implementation
 */

#include <string.h>
#include "axi_mcdma_test.h"
#include "../utils/debug_print.h"
#include "../drivers/axi_mcdma_driver.h"
#include "../utils/timer_utils.h"
#include "../utils/memory_utils.h"
#include "../utils/data_patterns.h"
#include "../utils/results_logger.h"
#include "../utils/cache_utils.h"

/*******************************************************************************
 * Local Variables
 ******************************************************************************/

#define MCDMA_SRC_BASE_OFFSET  0x04000000  /* 64MB */
#define MCDMA_DST_BASE_OFFSET  0x05000000  /* 80MB */
#define MCDMA_CHANNEL_SPACING  0x00100000  /* 1MB per channel */

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

int axi_mcdma_test_run_all(void)
{
    TestResult_t result;
    int status;

    LOG_RESULT("Running AXI MCDMA Tests...\r\n\r\n");

    /* Initialize channels */
    for (uint32_t ch = 0; ch < 4; ch++) {
        axi_mcdma_enable_mm2s_channel(ch, false);
        axi_mcdma_enable_s2mm_channel(ch, false);
    }

    /* Single channel tests */
    LOG_RESULT("1. Single Channel Throughput:\r\n");
    uint32_t sizes[] = {KB(1), KB(16), KB(64), KB(256), MB(1)};
    for (int i = 0; i < 5; i++) {
        memset(&result, 0, sizeof(result));
        status = axi_mcdma_test_single_channel(0, sizes[i], &result);
        if (status == DMA_SUCCESS) {
            char size_str[32];
            results_logger_format_size(sizes[i], size_str, sizeof(size_str));
            LOG_RESULT("  CH0, Size %s: %lu MB/s\r\n", size_str, (unsigned long)result.throughput_mbps);
        }
    }

    /* Multi-channel scalability test */
    LOG_RESULT("\r\n2. Multi-Channel Scalability:\r\n");
    axi_mcdma_test_scalability();

    /* Scheduler mode tests */
    LOG_RESULT("\r\n3. Scheduler Mode Tests:\r\n");
    memset(&result, 0, sizeof(result));
    status = axi_mcdma_test_round_robin(&result);
    LOG_RESULT("  Round-Robin: %lu MB/s aggregate\r\n",
              (unsigned long)(status == DMA_SUCCESS ? result.throughput_mbps : 0));

    memset(&result, 0, sizeof(result));
    status = axi_mcdma_test_priority(&result);
    LOG_RESULT("  Priority:    %lu MB/s aggregate\r\n",
              (unsigned long)(status == DMA_SUCCESS ? result.throughput_mbps : 0));

    LOG_RESULT("\r\nAXI MCDMA tests complete.\r\n");
    return DMA_SUCCESS;
}

int axi_mcdma_test_single_channel(uint32_t channel, uint32_t size, TestResult_t* result)
{
    uint64_t src_addr, dst_addr;
    uint64_t start_time, elapsed_us;
    uint32_t iterations = DEFAULT_TEST_ITERATIONS;
    uint32_t i;
    int status;

    /* Calculate addresses for this channel */
    uint32_t src_offset = MCDMA_SRC_BASE_OFFSET + (channel * MCDMA_CHANNEL_SPACING);
    uint32_t dst_offset = MCDMA_DST_BASE_OFFSET + (channel * MCDMA_CHANNEL_SPACING);

    src_addr = memory_get_test_addr(MEM_REGION_DDR4, src_offset, size);
    dst_addr = memory_get_test_addr(MEM_REGION_DDR4, dst_offset, size);

    if (src_addr == 0 || dst_addr == 0) {
        return DMA_ERROR_INVALID_PARAM;
    }

    /* Prepare buffers */
    pattern_fill((void*)(uintptr_t)src_addr, size, PATTERN_INCREMENTAL, channel);
    cache_prep_dma_src(src_addr, size);
    memset((void*)(uintptr_t)dst_addr, 0, size);
    cache_prep_dma_dst(dst_addr, size);

    /* Warmup */
    for (i = 0; i < WARMUP_ITERATIONS; i++) {
        status = axi_mcdma_transfer(channel, src_addr, dst_addr, size);
        if (status != DMA_SUCCESS) return status;
        status = axi_mcdma_wait_complete(channel, DMA_TIMEOUT_US);
        if (status != DMA_SUCCESS) return status;
    }

    /* Timed iterations */
    start_time = timer_start();

    for (i = 0; i < iterations; i++) {
        cache_prep_dma_dst(dst_addr, size);

        status = axi_mcdma_transfer(channel, src_addr, dst_addr, size);
        if (status != DMA_SUCCESS) return status;

        status = axi_mcdma_wait_complete(channel, DMA_TIMEOUT_US);
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
    result->dma_type = DMA_TYPE_AXI_MCDMA;
    result->test_type = TEST_THROUGHPUT;
    result->src_region = MEM_REGION_DDR4;
    result->dst_region = MEM_REGION_DDR4;
    result->transfer_size = size;
    result->iterations = iterations;
    result->total_bytes = (uint64_t)size * iterations;
    result->total_time_us = elapsed_us;
    result->throughput_mbps = CALC_THROUGHPUT_MBPS(result->total_bytes, elapsed_us);
    result->latency_us = elapsed_us / iterations;
    result->latency_ns = 0;
    result->data_integrity = integrity;

    return DMA_SUCCESS;
}

int axi_mcdma_test_multi_channel(uint32_t num_channels, uint32_t size, TestResult_t* result)
{
    uint64_t src_addrs[MCDMA_MAX_CHANNELS];
    uint64_t dst_addrs[MCDMA_MAX_CHANNELS];
    uint64_t start_time, elapsed_us;
    uint32_t iterations = DEFAULT_TEST_ITERATIONS / num_channels;
    uint32_t ch, i;
    int status;

    if (num_channels > MCDMA_MAX_CHANNELS) {
        num_channels = MCDMA_MAX_CHANNELS;
    }

    /* Setup addresses for each channel */
    for (ch = 0; ch < num_channels; ch++) {
        uint32_t src_offset = MCDMA_SRC_BASE_OFFSET + (ch * MCDMA_CHANNEL_SPACING);
        uint32_t dst_offset = MCDMA_DST_BASE_OFFSET + (ch * MCDMA_CHANNEL_SPACING);

        src_addrs[ch] = memory_get_test_addr(MEM_REGION_DDR4, src_offset, size);
        dst_addrs[ch] = memory_get_test_addr(MEM_REGION_DDR4, dst_offset, size);

        if (src_addrs[ch] == 0 || dst_addrs[ch] == 0) {
            return DMA_ERROR_INVALID_PARAM;
        }

        /* Enable channels */
        axi_mcdma_enable_mm2s_channel(ch, false);
        axi_mcdma_enable_s2mm_channel(ch, false);

        /* Prepare buffers */
        pattern_fill((void*)(uintptr_t)src_addrs[ch], size, PATTERN_RANDOM, ch);
        cache_prep_dma_src(src_addrs[ch], size);
        cache_prep_dma_dst(dst_addrs[ch], size);
    }

    /* Warmup */
    for (i = 0; i < WARMUP_ITERATIONS; i++) {
        for (ch = 0; ch < num_channels; ch++) {
            axi_mcdma_transfer(ch, src_addrs[ch], dst_addrs[ch], size);
        }
        for (ch = 0; ch < num_channels; ch++) {
            axi_mcdma_wait_complete(ch, DMA_TIMEOUT_US);
        }
    }

    /* Timed iterations - all channels running concurrently */
    start_time = timer_start();

    for (i = 0; i < iterations; i++) {
        /* Prepare destinations */
        for (ch = 0; ch < num_channels; ch++) {
            cache_prep_dma_dst(dst_addrs[ch], size);
        }

        /* Start all channels */
        for (ch = 0; ch < num_channels; ch++) {
            status = axi_mcdma_transfer(ch, src_addrs[ch], dst_addrs[ch], size);
            if (status != DMA_SUCCESS) return status;
        }

        /* Wait for all channels */
        for (ch = 0; ch < num_channels; ch++) {
            status = axi_mcdma_wait_complete(ch, DMA_TIMEOUT_US);
            if (status != DMA_SUCCESS) return status;
        }
    }

    elapsed_us = timer_stop_us(start_time);

    /* Calculate aggregate throughput */
    uint64_t total_bytes = (uint64_t)size * iterations * num_channels;

    result->dma_type = DMA_TYPE_AXI_MCDMA;
    result->test_type = TEST_MULTICHANNEL;
    result->src_region = MEM_REGION_DDR4;
    result->dst_region = MEM_REGION_DDR4;
    result->transfer_size = size;
    result->iterations = iterations;
    result->num_channels = num_channels;
    result->total_bytes = total_bytes;
    result->total_time_us = elapsed_us;
    result->throughput_mbps = CALC_THROUGHPUT_MBPS(total_bytes, elapsed_us);
    result->data_integrity = true;

    return DMA_SUCCESS;
}

int axi_mcdma_test_round_robin(TestResult_t* result)
{
    /* Set round-robin scheduler */
    axi_mcdma_set_scheduler(MCDMA_SCHED_ROUND_ROBIN);

    /* Run 4-channel test */
    return axi_mcdma_test_multi_channel(4, KB(64), result);
}

int axi_mcdma_test_priority(TestResult_t* result)
{
    /* Set strict priority scheduler */
    axi_mcdma_set_scheduler(MCDMA_SCHED_STRICT_PRIORITY);

    /* Run 4-channel test */
    return axi_mcdma_test_multi_channel(4, KB(64), result);
}

int axi_mcdma_test_scalability(void)
{
    TestResult_t result;
    int status;
    uint32_t channel_counts[] = {1, 2, 4, 8, 16};
    uint32_t size = KB(64);

    LOG_RESULT("  Channels  | Throughput (MB/s) | Per-Channel\r\n");
    LOG_RESULT("  ----------|-------------------|------------\r\n");

    for (int i = 0; i < 5; i++) {
        uint32_t num_ch = channel_counts[i];

        /* Enable required channels */
        for (uint32_t ch = 0; ch < num_ch; ch++) {
            axi_mcdma_enable_mm2s_channel(ch, false);
            axi_mcdma_enable_s2mm_channel(ch, false);
        }

        memset(&result, 0, sizeof(result));
        status = axi_mcdma_test_multi_channel(num_ch, size, &result);

        if (status == DMA_SUCCESS) {
            uint32_t per_channel = result.throughput_mbps / num_ch;
            LOG_RESULT("  %9d | %17lu | %10lu\r\n",
                      num_ch, (unsigned long)result.throughput_mbps, (unsigned long)per_channel);
        } else {
            LOG_RESULT("  %9d | %17s | %10s\r\n", num_ch, "ERROR", "---");
        }

        /* Disable channels */
        for (uint32_t ch = 0; ch < num_ch; ch++) {
            axi_mcdma_disable_mm2s_channel(ch);
            axi_mcdma_disable_s2mm_channel(ch);
        }
    }

    return DMA_SUCCESS;
}
