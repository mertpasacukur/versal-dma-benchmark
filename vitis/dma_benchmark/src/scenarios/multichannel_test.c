/**
 * @file multichannel_test.c
 * @brief Multi-Channel Test Scenarios Implementation
 */

#include <string.h>
#include "multichannel_test.h"
#include "../utils/debug_print.h"
#include "../drivers/axi_mcdma_driver.h"
#include "../drivers/lpd_dma_driver.h"
#include "../utils/timer_utils.h"
#include "../utils/memory_utils.h"
#include "../utils/data_patterns.h"
#include "../utils/cache_utils.h"
#include "../tests/axi_mcdma_test.h"
#include "../tests/lpd_dma_test.h"

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

int multichannel_test_run_all(void)
{
    LOG_RESULT("\r\n");
    LOG_RESULT("================================================================\r\n");
    LOG_RESULT("                Multi-Channel Test Suite\r\n");
    LOG_RESULT("================================================================\r\n\r\n");

    /* MCDMA scalability */
    LOG_RESULT("1. AXI MCDMA Channel Scalability:\r\n\r\n");
    multichannel_test_mcdma_scalability();

    /* LPD DMA scalability */
    LOG_RESULT("\r\n2. LPD DMA Channel Scalability:\r\n\r\n");
    multichannel_test_lpd_scalability();

    /* Concurrent DMA controllers */
    LOG_RESULT("\r\n3. Concurrent DMA Controller Test:\r\n\r\n");
    multichannel_test_concurrent_dma();

    /* Fairness test */
    LOG_RESULT("\r\n4. Channel Fairness Test:\r\n\r\n");
    multichannel_test_fairness();

    LOG_RESULT("\r\nMulti-channel tests complete.\r\n");
    return DMA_SUCCESS;
}

int multichannel_test_mcdma_scalability(void)
{
    TestResult_t result;
    int status;
    uint32_t channel_counts[] = {1, 2, 4, 8, 16};
    uint32_t size = KB(64);

    LOG_RESULT("  Channels | Aggregate (MB/s) | Per-Channel (MB/s) | Efficiency\r\n");
    LOG_RESULT("  ---------|------------------|--------------------|-----------\r\n");

    double single_channel_tp = 0;

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
            double per_channel = result.throughput_mbps / num_ch;

            if (num_ch == 1) {
                single_channel_tp = result.throughput_mbps;
            }

            double efficiency = (single_channel_tp > 0) ?
                (per_channel / single_channel_tp * 100.0) : 100.0;

            LOG_RESULT("  %8d | %16.2f | %18.2f | %8.1f%%\r\n",
                      num_ch, result.throughput_mbps, per_channel, efficiency);

            g_BenchmarkStats.tests_run++;
            g_BenchmarkStats.tests_passed++;
            g_BenchmarkStats.total_bytes_transferred += result.total_bytes;
        } else {
            LOG_RESULT("  %8d | %16s | %18s | %8s\r\n",
                      num_ch, "ERROR", "---", "---");
            g_BenchmarkStats.tests_run++;
            g_BenchmarkStats.tests_failed++;
        }

        /* Disable channels */
        for (uint32_t ch = 0; ch < num_ch; ch++) {
            axi_mcdma_disable_mm2s_channel(ch);
            axi_mcdma_disable_s2mm_channel(ch);
        }
    }

    return DMA_SUCCESS;
}

int multichannel_test_lpd_scalability(void)
{
    TestResult_t result;
    int status;
    uint32_t channel_counts[] = {1, 2, 4, 8};
    uint32_t size = KB(64);

    LOG_RESULT("  Channels | Aggregate (MB/s) | Per-Channel (MB/s) | Efficiency\r\n");
    LOG_RESULT("  ---------|------------------|--------------------|-----------\r\n");

    double single_channel_tp = 0;

    for (int i = 0; i < 4; i++) {
        uint32_t num_ch = channel_counts[i];

        memset(&result, 0, sizeof(result));
        status = lpd_dma_test_multi_channel(num_ch, size, &result);

        if (status == DMA_SUCCESS) {
            double per_channel = result.throughput_mbps / num_ch;

            if (num_ch == 1) {
                single_channel_tp = result.throughput_mbps;
            }

            double efficiency = (single_channel_tp > 0) ?
                (per_channel / single_channel_tp * 100.0) : 100.0;

            LOG_RESULT("  %8d | %16.2f | %18.2f | %8.1f%%\r\n",
                      num_ch, result.throughput_mbps, per_channel, efficiency);

            g_BenchmarkStats.tests_run++;
            g_BenchmarkStats.tests_passed++;
        } else {
            LOG_RESULT("  %8d | %16s | %18s | %8s\r\n",
                      num_ch, "ERROR", "---", "---");
            g_BenchmarkStats.tests_run++;
            g_BenchmarkStats.tests_failed++;
        }
    }

    return DMA_SUCCESS;
}

int multichannel_test_concurrent_dma(void)
{
    LOG_RESULT("  Testing concurrent operations from multiple DMA controllers...\r\n\r\n");

    /* This test runs MCDMA and LPD DMA concurrently to measure interference */
    uint32_t size = KB(64);
    uint32_t iterations = 50;

    /* Setup addresses for MCDMA */
    uint64_t mcdma_src = memory_get_test_addr(MEM_REGION_DDR4, MB(64), size);
    uint64_t mcdma_dst = memory_get_test_addr(MEM_REGION_DDR4, MB(65), size);

    /* Setup addresses for LPD DMA */
    uint64_t lpd_src = memory_get_test_addr(MEM_REGION_DDR4, MB(66), size);
    uint64_t lpd_dst = memory_get_test_addr(MEM_REGION_DDR4, MB(67), size);

    if (!mcdma_src || !mcdma_dst || !lpd_src || !lpd_dst) {
        LOG_RESULT("  ERROR: Could not allocate test buffers\r\n");
        return DMA_ERROR_NO_MEMORY;
    }

    /* Prepare buffers */
    pattern_fill((void*)(uintptr_t)mcdma_src, size, PATTERN_RANDOM, 0x1234);
    pattern_fill((void*)(uintptr_t)lpd_src, size, PATTERN_RANDOM, 0x5678);
    cache_prep_dma_src(mcdma_src, size);
    cache_prep_dma_src(lpd_src, size);

    /* Enable MCDMA channel 0 */
    axi_mcdma_enable_mm2s_channel(0, false);
    axi_mcdma_enable_s2mm_channel(0, false);

    /* Warmup */
    for (uint32_t i = 0; i < 5; i++) {
        cache_prep_dma_dst(mcdma_dst, size);
        cache_prep_dma_dst(lpd_dst, size);

        axi_mcdma_transfer(0, mcdma_src, mcdma_dst, size);
        lpd_dma_transfer(0, lpd_src, lpd_dst, size);

        axi_mcdma_wait_complete(0, DMA_TIMEOUT_US);
        lpd_dma_wait_complete(0, DMA_TIMEOUT_US);
    }

    /* Measure concurrent performance */
    uint64_t start = timer_start();

    for (uint32_t i = 0; i < iterations; i++) {
        cache_prep_dma_dst(mcdma_dst, size);
        cache_prep_dma_dst(lpd_dst, size);

        /* Start both DMAs */
        axi_mcdma_transfer(0, mcdma_src, mcdma_dst, size);
        lpd_dma_transfer(0, lpd_src, lpd_dst, size);

        /* Wait for both */
        axi_mcdma_wait_complete(0, DMA_TIMEOUT_US);
        lpd_dma_wait_complete(0, DMA_TIMEOUT_US);
    }

    uint64_t elapsed = timer_stop_us(start);

    /* Calculate combined throughput */
    uint64_t total_bytes = (uint64_t)size * iterations * 2;  /* Both DMAs */
    double combined_tp = CALC_THROUGHPUT_MBPS(total_bytes, elapsed);

    LOG_RESULT("  Concurrent MCDMA + LPD DMA:\r\n");
    LOG_RESULT("    Combined throughput: %.2f MB/s\r\n", combined_tp);
    LOG_RESULT("    Per-controller:      %.2f MB/s each (average)\r\n", combined_tp / 2.0);

    /* Disable MCDMA channel */
    axi_mcdma_disable_mm2s_channel(0);
    axi_mcdma_disable_s2mm_channel(0);

    g_BenchmarkStats.tests_run++;
    g_BenchmarkStats.tests_passed++;
    g_BenchmarkStats.total_bytes_transferred += total_bytes;

    return DMA_SUCCESS;
}

int multichannel_test_fairness(void)
{
    LOG_RESULT("  Testing round-robin fairness for MCDMA channels...\r\n\r\n");

    uint32_t num_channels = 4;
    uint32_t size = KB(64);
    uint32_t iterations = 100;
    uint64_t channel_bytes[4] = {0};

    /* Enable channels */
    for (uint32_t ch = 0; ch < num_channels; ch++) {
        axi_mcdma_enable_mm2s_channel(ch, false);
        axi_mcdma_enable_s2mm_channel(ch, false);
    }

    /* Set round-robin mode */
    axi_mcdma_set_scheduler(MCDMA_SCHED_ROUND_ROBIN);

    /* Prepare per-channel buffers */
    uint64_t src_addrs[4], dst_addrs[4];
    for (uint32_t ch = 0; ch < num_channels; ch++) {
        src_addrs[ch] = memory_get_test_addr(MEM_REGION_DDR4, MB(80) + ch * size * 4, size);
        dst_addrs[ch] = memory_get_test_addr(MEM_REGION_DDR4, MB(80) + ch * size * 4 + size * 2, size);
        pattern_fill((void*)(uintptr_t)src_addrs[ch], size, PATTERN_INCREMENTAL, ch);
        cache_prep_dma_src(src_addrs[ch], size);
    }

    /* Run transfers and measure per-channel throughput */
    uint64_t start_times[4], elapsed_times[4] = {0};

    for (uint32_t iter = 0; iter < iterations; iter++) {
        /* Start all channels */
        for (uint32_t ch = 0; ch < num_channels; ch++) {
            cache_prep_dma_dst(dst_addrs[ch], size);
            start_times[ch] = timer_start();
            axi_mcdma_transfer(ch, src_addrs[ch], dst_addrs[ch], size);
        }

        /* Wait for each and record time */
        for (uint32_t ch = 0; ch < num_channels; ch++) {
            axi_mcdma_wait_complete(ch, DMA_TIMEOUT_US);
            elapsed_times[ch] += timer_stop_us(start_times[ch]);
            channel_bytes[ch] += size;
        }
    }

    /* Calculate per-channel throughput */
    LOG_RESULT("  Channel | Throughput (MB/s) | Deviation\r\n");
    LOG_RESULT("  --------|-------------------|----------\r\n");

    double throughputs[4];
    double avg_tp = 0;

    for (uint32_t ch = 0; ch < num_channels; ch++) {
        throughputs[ch] = CALC_THROUGHPUT_MBPS(channel_bytes[ch], elapsed_times[ch]);
        avg_tp += throughputs[ch];
    }
    avg_tp /= num_channels;

    for (uint32_t ch = 0; ch < num_channels; ch++) {
        double deviation = ((throughputs[ch] - avg_tp) / avg_tp) * 100.0;
        LOG_RESULT("  CH%d     | %17.2f | %+7.1f%%\r\n",
                  ch, throughputs[ch], deviation);
    }

    LOG_RESULT("\r\n  Average: %.2f MB/s\r\n", avg_tp);
    LOG_RESULT("  Fairness: Good if all deviations are within +/-10%%\r\n");

    /* Disable channels */
    for (uint32_t ch = 0; ch < num_channels; ch++) {
        axi_mcdma_disable_mm2s_channel(ch);
        axi_mcdma_disable_s2mm_channel(ch);
    }

    g_BenchmarkStats.tests_run++;
    g_BenchmarkStats.tests_passed++;

    return DMA_SUCCESS;
}
