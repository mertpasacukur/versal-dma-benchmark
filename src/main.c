/**
 * @file main.c
 * @brief Versal DMA Benchmark - Main Application
 *
 * Comprehensive DMA benchmarking for VPK120 development board.
 * Tests all available DMA controllers across different memory types.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xil_cache.h"
#include "xparameters.h"
#include "sleep.h"

#include "dma_benchmark.h"
#include "platform_config.h"
#include "drivers/axi_dma_driver.h"
#include "drivers/axi_cdma_driver.h"
#include "drivers/axi_mcdma_driver.h"
#include "drivers/lpd_dma_driver.h"
#include "utils/timer_utils.h"
#include "utils/memory_utils.h"
#include "utils/data_patterns.h"
#include "utils/results_logger.h"
#include "utils/cache_utils.h"
#include "utils/debug_print.h"
#include "tests/axi_dma_test.h"
#include "tests/axi_cdma_test.h"
#include "tests/axi_mcdma_test.h"
#include "tests/lpd_dma_test.h"
#include "tests/comparison_test.h"
#include "scenarios/throughput_test.h"
#include "scenarios/latency_test.h"
#include "scenarios/multichannel_test.h"
#include "scenarios/stress_test.h"

/*******************************************************************************
 * Global Variables
 ******************************************************************************/

BenchmarkStats_t g_BenchmarkStats = {0};
volatile bool g_TestAbort = false;

/* Memory region table */
const MemoryRegionInfo_t g_MemoryRegions[MEM_REGION_COUNT] = {
    [MEM_REGION_DDR4] = {
        .name = "DDR4",
        .base_addr = DDR4_BASE_ADDR,
        .size = DDR4_SIZE,
        .test_base = DDR4_TEST_REGION_BASE,
        .test_size = DDR4_TEST_REGION_SIZE,
        .cacheable = 1
    },
    [MEM_REGION_LPDDR4] = {
        .name = "LPDDR4",
        .base_addr = LPDDR4_BASE_ADDR,
        .size = LPDDR4_SIZE,
        .test_base = LPDDR4_TEST_REGION_BASE,
        .test_size = LPDDR4_TEST_REGION_SIZE,
        .cacheable = 1
    },
    [MEM_REGION_OCM] = {
        .name = "OCM",
        .base_addr = OCM_BASE_ADDR,
        .size = OCM_SIZE,
        .test_base = OCM_BASE_ADDR,
        .test_size = OCM_SIZE,
        .cacheable = 0
    },
    [MEM_REGION_BRAM] = {
        .name = "BRAM",
        .base_addr = PL_BRAM_BASE_ADDR,
        .size = PL_BRAM_SIZE,
        .test_base = PL_BRAM_BASE_ADDR,
        .test_size = PL_BRAM_SIZE,
        .cacheable = 0
    },
    [MEM_REGION_URAM] = {
        .name = "URAM",
        .base_addr = PL_URAM_BASE_ADDR,
        .size = PL_URAM_SIZE,
        .test_base = PL_URAM_BASE_ADDR,
        .test_size = PL_URAM_SIZE,
        .cacheable = 0
    },
    [MEM_REGION_HOST] = {
        .name = "HOST",
        .base_addr = 0,
        .size = 0,
        .test_base = 0,
        .test_size = 0,
        .cacheable = 0
    }
};

/*******************************************************************************
 * String Conversion Functions
 ******************************************************************************/

const char* dma_type_to_string(DmaType_t type)
{
    static const char* names[] = {
        [DMA_TYPE_AXI_DMA]    = "AXI_DMA",
        [DMA_TYPE_AXI_CDMA]   = "AXI_CDMA",
        [DMA_TYPE_AXI_MCDMA]  = "AXI_MCDMA",
        [DMA_TYPE_LPD_DMA]    = "LPD_DMA",
        [DMA_TYPE_QDMA]       = "QDMA",
        [DMA_TYPE_CPU_MEMCPY] = "CPU_MEMCPY"
    };
    if (type < DMA_TYPE_COUNT) {
        return names[type];
    }
    return "UNKNOWN";
}

const char* memory_region_to_string(MemoryRegion_t region)
{
    if (region < MEM_REGION_COUNT) {
        return g_MemoryRegions[region].name;
    }
    return "UNKNOWN";
}

const char* pattern_to_string(DataPattern_t pattern)
{
    static const char* names[] = {
        [PATTERN_INCREMENTAL]  = "INCREMENTAL",
        [PATTERN_ALL_ONES]     = "ALL_ONES",
        [PATTERN_ALL_ZEROS]    = "ALL_ZEROS",
        [PATTERN_RANDOM]       = "RANDOM",
        [PATTERN_CHECKERBOARD] = "CHECKERBOARD"
    };
    if (pattern < PATTERN_COUNT) {
        return names[pattern];
    }
    return "UNKNOWN";
}

const char* dma_mode_to_string(DmaMode_t mode)
{
    static const char* names[] = {
        [DMA_MODE_SIMPLE]    = "SIMPLE",
        [DMA_MODE_SG]        = "SG",
        [DMA_MODE_POLLING]   = "POLLING",
        [DMA_MODE_INTERRUPT] = "INTERRUPT"
    };
    if (mode < DMA_MODE_COUNT) {
        return names[mode];
    }
    return "UNKNOWN";
}

const char* test_type_to_string(TestType_t type)
{
    static const char* names[] = {
        [TEST_THROUGHPUT]    = "THROUGHPUT",
        [TEST_LATENCY]       = "LATENCY",
        [TEST_INTEGRITY]     = "INTEGRITY",
        [TEST_STRESS]        = "STRESS",
        [TEST_MULTICHANNEL]  = "MULTICHANNEL",
        [TEST_COMPARISON]    = "COMPARISON"
    };
    if (type < TEST_TYPE_COUNT) {
        return names[type];
    }
    return "UNKNOWN";
}

/*******************************************************************************
 * Platform Functions
 ******************************************************************************/

const MemoryRegionInfo_t* platform_get_memory_info(MemoryRegion_t region)
{
    if (region < MEM_REGION_COUNT) {
        return &g_MemoryRegions[region];
    }
    return NULL;
}

int platform_is_region_accessible(MemoryRegion_t region)
{
    if (region >= MEM_REGION_COUNT) {
        return 0;
    }

    /* Host memory only accessible via QDMA */
    if (region == MEM_REGION_HOST) {
        return 0;
    }

    return 1;
}

int platform_init(void)
{
    LOG_DEBUG("Initializing platform...\r\n");

    /* Initialize caches */
    Xil_DCacheEnable();
    Xil_ICacheEnable();

    LOG_DEBUG("Platform initialized successfully\r\n");
    return 0;
}

void platform_cleanup(void)
{
    LOG_DEBUG("Cleaning up platform...\r\n");
    Xil_DCacheDisable();
    Xil_ICacheDisable();
}

/*******************************************************************************
 * Progress Callback
 ******************************************************************************/

static void progress_callback(uint32_t current, uint32_t total, const char* msg)
{
    LOG_INFO("[%3d%%] %s\r\n", (current * 100) / total, msg);
}

/*******************************************************************************
 * Print Banner
 ******************************************************************************/

static void print_banner(void)
{
    LOG_ALWAYS("\r\n");
    LOG_ALWAYS("================================================================\r\n");
    LOG_ALWAYS("         Versal DMA Benchmark Suite v%d.%d.%d\r\n",
               DMA_BENCHMARK_VERSION_MAJOR,
               DMA_BENCHMARK_VERSION_MINOR,
               DMA_BENCHMARK_VERSION_PATCH);
    LOG_ALWAYS("         Target: VPK120 (Versal Premium VP1202)\r\n");
    LOG_ALWAYS("================================================================\r\n");
    LOG_ALWAYS("\r\n");

    LOG_ALWAYS("DMA Controllers:\r\n");
    LOG_ALWAYS("  - AXI DMA (Scatter-Gather, 512-bit)\r\n");
    LOG_ALWAYS("  - AXI CDMA (Memory-to-Memory, 512-bit)\r\n");
    LOG_ALWAYS("  - AXI MCDMA (16 channels, 512-bit)\r\n");
    LOG_ALWAYS("  - LPD DMA / ADMA (8 channels)\r\n");
    LOG_ALWAYS("\r\n");

    LOG_ALWAYS("Memory Types:\r\n");
    LOG_ALWAYS("  - DDR4:   8GB @ 3200 MT/s\r\n");
    LOG_ALWAYS("  - LPDDR4: 2GB @ 4267 MT/s\r\n");
    LOG_ALWAYS("  - OCM:    256KB (on-chip)\r\n");
    LOG_ALWAYS("  - BRAM:   128KB (PL)\r\n");
    LOG_ALWAYS("  - URAM:   64KB (PL)\r\n");
    LOG_ALWAYS("\r\n");
}

/*******************************************************************************
 * Menu System
 ******************************************************************************/

static void print_menu(void)
{
    LOG_ALWAYS("\r\n");
    LOG_ALWAYS("=== DMA Benchmark Menu ===\r\n");
    LOG_ALWAYS("1. Run Full Benchmark Suite\r\n");
    LOG_ALWAYS("2. AXI DMA Tests Only\r\n");
    LOG_ALWAYS("3. AXI CDMA Tests Only\r\n");
    LOG_ALWAYS("4. AXI MCDMA Tests Only\r\n");
    LOG_ALWAYS("5. LPD DMA Tests Only\r\n");
    LOG_ALWAYS("6. Throughput Comparison\r\n");
    LOG_ALWAYS("7. Latency Tests\r\n");
    LOG_ALWAYS("8. Multi-Channel Tests (MCDMA)\r\n");
    LOG_ALWAYS("9. Stress Test (1 hour)\r\n");
    LOG_ALWAYS("A. Memory-to-Memory Matrix Test\r\n");
    LOG_ALWAYS("C. CPU memcpy Baseline\r\n");
    LOG_ALWAYS("D. Set Debug Level\r\n");
    LOG_ALWAYS("S. Print Statistics\r\n");
    LOG_ALWAYS("R. Reset Statistics\r\n");
    LOG_ALWAYS("Q. Quit\r\n");
    LOG_ALWAYS("\r\n");
    LOG_ALWAYS("Select option: ");
}

static int get_user_choice(void)
{
    char c;
    c = inbyte();
    LOG_ALWAYS("%c\r\n", c);
    return c;
}

/*******************************************************************************
 * Test Runners
 ******************************************************************************/

static int run_axi_dma_tests(void)
{
    LOG_ALWAYS("\r\n=== Running AXI DMA Tests ===\r\n\r\n");
    return axi_dma_test_run_all();
}

static int run_axi_cdma_tests(void)
{
    LOG_ALWAYS("\r\n=== Running AXI CDMA Tests ===\r\n\r\n");
    return axi_cdma_test_run_all();
}

static int run_axi_mcdma_tests(void)
{
    LOG_ALWAYS("\r\n=== Running AXI MCDMA Tests ===\r\n\r\n");
    return axi_mcdma_test_run_all();
}

static int run_lpd_dma_tests(void)
{
    LOG_ALWAYS("\r\n=== Running LPD DMA Tests ===\r\n\r\n");
    return lpd_dma_test_run_all();
}

static int run_throughput_comparison(void)
{
    LOG_ALWAYS("\r\n=== Running Throughput Comparison ===\r\n\r\n");
    return comparison_test_run();
}

static int run_latency_tests(void)
{
    LOG_ALWAYS("\r\n=== Running Latency Tests ===\r\n\r\n");
    return latency_test_run_all();
}

static int run_multichannel_tests(void)
{
    LOG_ALWAYS("\r\n=== Running Multi-Channel Tests ===\r\n\r\n");
    return multichannel_test_run_all();
}

static int run_stress_tests(void)
{
    LOG_ALWAYS("\r\n=== Running Stress Tests (1 hour) ===\r\n\r\n");
    LOG_ALWAYS("Press any key to abort...\r\n\r\n");
    return stress_test_run(3600);  /* 1 hour */
}

static int run_memory_matrix_test(void)
{
    LOG_ALWAYS("\r\n=== Running Memory-to-Memory Matrix Test ===\r\n\r\n");
    return throughput_test_run_memory_matrix();
}

static int run_cpu_baseline(void)
{
    LOG_ALWAYS("\r\n=== Running CPU memcpy Baseline ===\r\n\r\n");
    return throughput_test_run_cpu_baseline();
}

static void print_statistics(void)
{
    benchmark_print_summary();
}

static void reset_statistics(void)
{
    memset(&g_BenchmarkStats, 0, sizeof(g_BenchmarkStats));
    LOG_INFO("Statistics reset.\r\n");
}

static void set_debug_level(void)
{
    LOG_ALWAYS("\r\n=== Set Debug Level ===\r\n");
    LOG_ALWAYS("Current level: %s\r\n", debug_level_to_string(debug_get_level()));
    LOG_ALWAYS("0. ERROR (minimum output)\r\n");
    LOG_ALWAYS("1. WARNING\r\n");
    LOG_ALWAYS("2. INFO (default)\r\n");
    LOG_ALWAYS("3. DEBUG (maximum output)\r\n");
    LOG_ALWAYS("Select level: ");

    char c = inbyte();
    LOG_ALWAYS("%c\r\n", c);

    switch (c) {
        case '0':
            debug_set_level(LOG_LEVEL_ERROR);
            break;
        case '1':
            debug_set_level(LOG_LEVEL_WARNING);
            break;
        case '2':
            debug_set_level(LOG_LEVEL_INFO);
            break;
        case '3':
            debug_set_level(LOG_LEVEL_DEBUG);
            break;
        default:
            LOG_WARNING("Invalid selection, keeping current level.\r\n");
            break;
    }
}

/*******************************************************************************
 * Benchmark Control Functions
 ******************************************************************************/

int benchmark_init(void)
{
    int status;

    LOG_INFO("Initializing benchmark system...\r\n");

    /* Initialize platform */
    status = platform_init();
    if (status != 0) {
        LOG_ERROR("Platform init failed\r\n");
        return status;
    }

    /* Initialize timer subsystem */
    status = timer_init();
    if (status != 0) {
        LOG_ERROR("Timer init failed\r\n");
        return status;
    }

    /* Initialize results logger */
    status = results_logger_init();
    if (status != 0) {
        LOG_ERROR("Results logger init failed\r\n");
        return status;
    }

    /* Initialize DMA drivers */
    status = axi_dma_init();
    if (status != 0) {
        LOG_WARNING("AXI DMA init failed\r\n");
    }

    status = axi_cdma_init();
    if (status != 0) {
        LOG_WARNING("AXI CDMA init failed\r\n");
    }

    status = axi_mcdma_init();
    if (status != 0) {
        LOG_WARNING("AXI MCDMA init failed\r\n");
    }

    status = lpd_dma_init();
    if (status != 0) {
        LOG_WARNING("LPD DMA init failed\r\n");
    }

    /* Reset statistics */
    memset(&g_BenchmarkStats, 0, sizeof(g_BenchmarkStats));
    g_TestAbort = false;

    LOG_INFO("Benchmark system initialized\r\n");
    return 0;
}

void benchmark_cleanup(void)
{
    LOG_DEBUG("Cleaning up benchmark system...\r\n");

    /* Cleanup DMA drivers */
    lpd_dma_cleanup();
    axi_mcdma_cleanup();
    axi_cdma_cleanup();
    axi_dma_cleanup();

    /* Cleanup results logger */
    results_logger_cleanup();

    /* Cleanup platform */
    platform_cleanup();

    LOG_DEBUG("Benchmark system cleanup complete\r\n");
}

int benchmark_run_full_suite(ProgressCallback_t progress_cb)
{
    int status = 0;
    int total_steps = 9;
    int current_step = 0;

    LOG_ALWAYS("\r\n");
    LOG_ALWAYS("================================================================\r\n");
    LOG_ALWAYS("              Running Full Benchmark Suite\r\n");
    LOG_ALWAYS("================================================================\r\n");
    LOG_ALWAYS("\r\n");

    /* Start CSV logging */
    results_logger_start_session("full_benchmark");

    /* Run all test categories */
    if (progress_cb) progress_cb(++current_step, total_steps, "AXI DMA tests");
    status |= run_axi_dma_tests();

    if (g_TestAbort) goto cleanup;

    if (progress_cb) progress_cb(++current_step, total_steps, "AXI CDMA tests");
    status |= run_axi_cdma_tests();

    if (g_TestAbort) goto cleanup;

    if (progress_cb) progress_cb(++current_step, total_steps, "AXI MCDMA tests");
    status |= run_axi_mcdma_tests();

    if (g_TestAbort) goto cleanup;

    if (progress_cb) progress_cb(++current_step, total_steps, "LPD DMA tests");
    status |= run_lpd_dma_tests();

    if (g_TestAbort) goto cleanup;

    if (progress_cb) progress_cb(++current_step, total_steps, "CPU baseline");
    status |= run_cpu_baseline();

    if (g_TestAbort) goto cleanup;

    if (progress_cb) progress_cb(++current_step, total_steps, "Throughput comparison");
    status |= run_throughput_comparison();

    if (g_TestAbort) goto cleanup;

    if (progress_cb) progress_cb(++current_step, total_steps, "Latency tests");
    status |= run_latency_tests();

    if (g_TestAbort) goto cleanup;

    if (progress_cb) progress_cb(++current_step, total_steps, "Multi-channel tests");
    status |= run_multichannel_tests();

    if (g_TestAbort) goto cleanup;

    if (progress_cb) progress_cb(++current_step, total_steps, "Memory matrix test");
    status |= run_memory_matrix_test();

cleanup:
    /* End CSV logging */
    results_logger_end_session();

    /* Print summary */
    benchmark_print_summary();

    return status;
}

void benchmark_abort(void)
{
    g_TestAbort = true;
    LOG_WARNING("\r\nAbort requested...\r\n");
}

const BenchmarkStats_t* benchmark_get_stats(void)
{
    return &g_BenchmarkStats;
}

void benchmark_print_summary(void)
{
    LOG_ALWAYS("\r\n");
    LOG_ALWAYS("================================================================\r\n");
    LOG_ALWAYS("                    Benchmark Summary\r\n");
    LOG_ALWAYS("================================================================\r\n");
    LOG_ALWAYS("Tests Run:    %lu\r\n", g_BenchmarkStats.tests_run);
    LOG_ALWAYS("Tests Passed: %lu\r\n", g_BenchmarkStats.tests_passed);
    LOG_ALWAYS("Tests Failed: %lu\r\n", g_BenchmarkStats.tests_failed);
    LOG_ALWAYS("Total Data:   %llu MB\r\n",
               g_BenchmarkStats.total_bytes_transferred / (1024 * 1024));
    LOG_ALWAYS("Total Time:   %llu ms\r\n",
               g_BenchmarkStats.total_time_us / 1000);

    if (g_BenchmarkStats.total_time_us > 0) {
        g_BenchmarkStats.overall_throughput_mbps =
            CALC_THROUGHPUT_MBPS(g_BenchmarkStats.total_bytes_transferred,
                                 g_BenchmarkStats.total_time_us);
        LOG_ALWAYS("Avg Throughput: %.2f MB/s\r\n",
                   g_BenchmarkStats.overall_throughput_mbps);
    }
    LOG_ALWAYS("================================================================\r\n");
    LOG_ALWAYS("\r\n");
}

/*******************************************************************************
 * Main Function
 ******************************************************************************/

int main(void)
{
    int choice;
    int status;
    int running = 1;

    /* Print banner */
    print_banner();

    /* Initialize benchmark system */
    status = benchmark_init();
    if (status != 0) {
        LOG_ERROR("Failed to initialize benchmark system\r\n");
        return -1;
    }

    /* Main menu loop */
    while (running) {
        print_menu();
        choice = get_user_choice();

        switch (choice) {
            case '1':
                benchmark_run_full_suite(progress_callback);
                break;

            case '2':
                run_axi_dma_tests();
                break;

            case '3':
                run_axi_cdma_tests();
                break;

            case '4':
                run_axi_mcdma_tests();
                break;

            case '5':
                run_lpd_dma_tests();
                break;

            case '6':
                run_throughput_comparison();
                break;

            case '7':
                run_latency_tests();
                break;

            case '8':
                run_multichannel_tests();
                break;

            case '9':
                run_stress_tests();
                break;

            case 'A':
            case 'a':
                run_memory_matrix_test();
                break;

            case 'C':
            case 'c':
                run_cpu_baseline();
                break;

            case 'D':
            case 'd':
                set_debug_level();
                break;

            case 'S':
            case 's':
                print_statistics();
                break;

            case 'R':
            case 'r':
                reset_statistics();
                break;

            case 'Q':
            case 'q':
                running = 0;
                break;

            default:
                LOG_WARNING("Invalid option. Please try again.\r\n");
                break;
        }

        /* Reset abort flag after each menu selection */
        g_TestAbort = false;
    }

    /* Cleanup */
    benchmark_cleanup();

    LOG_ALWAYS("\r\nBenchmark application terminated.\r\n");
    return 0;
}
