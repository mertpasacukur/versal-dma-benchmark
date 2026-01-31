/**
 * @file dma_benchmark.h
 * @brief Main header for Versal DMA Benchmark Suite
 *
 * Comprehensive DMA benchmarking for VPK120 board covering:
 * - AXI DMA (Scatter-Gather mode)
 * - AXI CDMA (Memory-to-Memory)
 * - AXI MCDMA (Multi-Channel)
 * - LPD DMA (ADMA)
 * - QDMA (PCIe)
 */

#ifndef DMA_BENCHMARK_H
#define DMA_BENCHMARK_H

#include <stdint.h>
#include <stdbool.h>
#include "platform_config.h"

/*******************************************************************************
 * Version Information
 ******************************************************************************/

#define DMA_BENCHMARK_VERSION_MAJOR    1
#define DMA_BENCHMARK_VERSION_MINOR    0
#define DMA_BENCHMARK_VERSION_PATCH    0

/*******************************************************************************
 * DMA Types
 ******************************************************************************/

typedef enum {
    DMA_TYPE_AXI_DMA = 0,    /* PL AXI DMA */
    DMA_TYPE_AXI_CDMA,       /* PL AXI Central DMA */
    DMA_TYPE_AXI_MCDMA,      /* PL AXI Multi-Channel DMA */
    DMA_TYPE_LPD_DMA,        /* PS LPD DMA (ADMA) */
    DMA_TYPE_QDMA,           /* PCIe QDMA */
    DMA_TYPE_CPU_MEMCPY,     /* CPU memcpy (baseline) */
    DMA_TYPE_COUNT
} DmaType_t;

/*******************************************************************************
 * Data Patterns
 ******************************************************************************/

typedef enum {
    PATTERN_INCREMENTAL = 0, /* 0x00, 0x01, 0x02, ... */
    PATTERN_ALL_ONES,        /* 0xFF, 0xFF, 0xFF, ... */
    PATTERN_ALL_ZEROS,       /* 0x00, 0x00, 0x00, ... */
    PATTERN_RANDOM,          /* PRNG generated */
    PATTERN_CHECKERBOARD,    /* 0xAA, 0x55, 0xAA, 0x55, ... */
    PATTERN_COUNT
} DataPattern_t;

/*******************************************************************************
 * DMA Operation Modes
 ******************************************************************************/

typedef enum {
    DMA_MODE_SIMPLE = 0,     /* Direct register mode (no SG) */
    DMA_MODE_SG,             /* Scatter-Gather mode */
    DMA_MODE_POLLING,        /* Polling for completion */
    DMA_MODE_INTERRUPT,      /* Interrupt-driven */
    DMA_MODE_COUNT
} DmaMode_t;

/*******************************************************************************
 * Test Types
 ******************************************************************************/

typedef enum {
    TEST_THROUGHPUT = 0,     /* Measure transfer speed */
    TEST_LATENCY,            /* Measure first-byte latency */
    TEST_INTEGRITY,          /* Verify data correctness */
    TEST_STRESS,             /* Long-duration stress test */
    TEST_MULTICHANNEL,       /* Multi-channel concurrent */
    TEST_COMPARISON,         /* Compare DMA types */
    TEST_TYPE_COUNT
} TestType_t;

/*******************************************************************************
 * Transfer Sizes
 ******************************************************************************/

/* Predefined transfer sizes for benchmarking */
static const uint32_t g_TransferSizes[] = {
    /* Small transfers */
    64, 128, 256, 512,
    /* Medium transfers */
    1024, 2048, 4096, 8192, 16384, 32768, 65536,
    /* Large transfers */
    131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608, 16777216
};

#define NUM_TRANSFER_SIZES (sizeof(g_TransferSizes) / sizeof(g_TransferSizes[0]))

/*******************************************************************************
 * Test Configuration
 ******************************************************************************/

typedef struct {
    DmaType_t       dma_type;
    TestType_t      test_type;
    MemoryRegion_t  src_region;
    MemoryRegion_t  dst_region;
    DataPattern_t   pattern;
    DmaMode_t       mode;
    uint32_t        transfer_size;
    uint32_t        iterations;
    uint32_t        num_channels;      /* For MCDMA */
    bool            verify_data;
    bool            aligned;           /* 64-byte aligned addresses */
    bool            bidirectional;     /* Simultaneous TX/RX */
} TestConfig_t;

/*******************************************************************************
 * Test Results
 ******************************************************************************/

typedef struct {
    /* Identification */
    DmaType_t       dma_type;
    TestType_t      test_type;
    MemoryRegion_t  src_region;
    MemoryRegion_t  dst_region;
    DataPattern_t   pattern;
    DmaMode_t       mode;
    uint32_t        transfer_size;

    /* Performance metrics (integer to support xil_printf) */
    uint32_t        throughput_mbps;   /* MB/s (whole number) */
    uint32_t        latency_us;        /* microseconds (whole number) */
    uint32_t        latency_ns;        /* nanoseconds for sub-us precision */
    uint32_t        setup_time_us;     /* DMA setup time */
    uint32_t        cpu_utilization;   /* percentage (for polling) */

    /* Statistics (all in MB/s or us as integers) */
    uint32_t        min_throughput;
    uint32_t        max_throughput;
    uint32_t        avg_throughput;
    uint32_t        min_latency;
    uint32_t        max_latency;
    uint32_t        avg_latency;

    /* Integrity */
    bool            data_integrity;    /* Pass/Fail */
    uint32_t        error_count;
    uint64_t        first_error_offset;

    /* Test info */
    uint32_t        iterations;
    uint32_t        num_channels;      /* For MCDMA/multi-channel tests */
    uint64_t        total_bytes;
    uint64_t        total_time_us;
} TestResult_t;

/*******************************************************************************
 * DMA Controller Handle
 ******************************************************************************/

typedef struct {
    DmaType_t       type;
    uint64_t        base_addr;
    uint32_t        irq_id;
    bool            initialized;
    bool            busy;
    void*           private_data;
} DmaHandle_t;

/*******************************************************************************
 * Scatter-Gather Descriptor (Generic)
 ******************************************************************************/

typedef struct __attribute__((aligned(64))) {
    uint64_t        next_desc;         /* Next descriptor address */
    uint64_t        src_addr;          /* Source buffer address */
    uint64_t        dst_addr;          /* Destination buffer address (CDMA) */
    uint32_t        control;           /* Control word */
    uint32_t        status;            /* Status word */
    uint32_t        app[5];            /* Application-specific fields */
} SgDescriptor_t;

/*******************************************************************************
 * Benchmark Statistics
 ******************************************************************************/

typedef struct {
    uint32_t        tests_run;
    uint32_t        tests_passed;
    uint32_t        tests_failed;
    uint64_t        total_bytes_transferred;
    uint64_t        total_time_us;
    uint32_t        overall_throughput_mbps;  /* MB/s as integer */
} BenchmarkStats_t;

/*******************************************************************************
 * Callback Function Types
 ******************************************************************************/

typedef void (*DmaCallback_t)(DmaHandle_t* handle, int status);
typedef void (*ProgressCallback_t)(uint32_t current, uint32_t total, const char* msg);

/*******************************************************************************
 * Global Variables (extern declarations)
 ******************************************************************************/

extern BenchmarkStats_t g_BenchmarkStats;
extern volatile bool g_TestAbort;

/*******************************************************************************
 * String Conversion Functions
 ******************************************************************************/

/**
 * @brief Get DMA type name string
 */
const char* dma_type_to_string(DmaType_t type);

/**
 * @brief Get memory region name string
 */
const char* memory_region_to_string(MemoryRegion_t region);

/**
 * @brief Get data pattern name string
 */
const char* pattern_to_string(DataPattern_t pattern);

/**
 * @brief Get DMA mode name string
 */
const char* dma_mode_to_string(DmaMode_t mode);

/**
 * @brief Get test type name string
 */
const char* test_type_to_string(TestType_t type);

/*******************************************************************************
 * Benchmark Control Functions
 ******************************************************************************/

/**
 * @brief Initialize the benchmark system
 * @return 0 on success, negative error code on failure
 */
int benchmark_init(void);

/**
 * @brief Cleanup benchmark resources
 */
void benchmark_cleanup(void);

/**
 * @brief Run a single benchmark test
 * @param config Test configuration
 * @param result Pointer to store results
 * @return 0 on success, negative error code on failure
 */
int benchmark_run_test(const TestConfig_t* config, TestResult_t* result);

/**
 * @brief Run full benchmark suite
 * @param progress_cb Progress callback (can be NULL)
 * @return 0 on success, negative error code on failure
 */
int benchmark_run_full_suite(ProgressCallback_t progress_cb);

/**
 * @brief Run comparison tests between DMA types
 * @return 0 on success, negative error code on failure
 */
int benchmark_run_comparison(void);

/**
 * @brief Abort running benchmark
 */
void benchmark_abort(void);

/**
 * @brief Get benchmark statistics
 * @return Pointer to benchmark statistics structure
 */
const BenchmarkStats_t* benchmark_get_stats(void);

/**
 * @brief Print benchmark summary to UART
 */
void benchmark_print_summary(void);

/*******************************************************************************
 * Utility Macros
 ******************************************************************************/

/* Alignment macros */
#define ALIGN_UP(x, align)      (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align)    ((x) & ~((align) - 1))
#define IS_ALIGNED(x, align)    (((x) & ((align) - 1)) == 0)

/* Size conversion macros */
#define KB(x)                   ((x) * 1024ULL)
#define MB(x)                   ((x) * 1024ULL * 1024ULL)
#define GB(x)                   ((x) * 1024ULL * 1024ULL * 1024ULL)

/* Min/Max macros */
#define MIN(a, b)               (((a) < (b)) ? (a) : (b))
#define MAX(a, b)               (((a) > (b)) ? (a) : (b))

/* Array size */
#define ARRAY_SIZE(x)           (sizeof(x) / sizeof((x)[0]))

/* Throughput calculation: MB/s from bytes and microseconds (integer math)
 * Formula: throughput = bytes/time = bytes/(us/1000000) = bytes*1000000/us bytes/sec
 * In MB/s: bytes*1000000/(us*1048576) = bytes*1000000/(us<<20)
 * Simplified to avoid overflow: (bytes>>10) * 1000 / us  (approximation in KB/ms = MB/s)
 */
#define CALC_THROUGHPUT_MBPS(bytes, us) \
    (((us) > 0) ? (uint32_t)(((uint64_t)(bytes) * 1000000ULL) / ((uint64_t)(us) * 1048576ULL)) : 0)

/* Efficiency calculation (returns percentage as integer) */
#define CALC_EFFICIENCY(actual, theoretical) \
    (((theoretical) > 0) ? (uint32_t)(((uint64_t)(actual) * 100ULL) / (uint64_t)(theoretical)) : 0)

/*******************************************************************************
 * Error Codes
 ******************************************************************************/

#define DMA_SUCCESS             0
#define DMA_ERROR_INVALID_PARAM -1
#define DMA_ERROR_NOT_INIT      -2
#define DMA_ERROR_BUSY          -3
#define DMA_ERROR_TIMEOUT       -4
#define DMA_ERROR_DMA_FAIL      -5
#define DMA_ERROR_VERIFY_FAIL   -6
#define DMA_ERROR_NO_MEMORY     -7
#define DMA_ERROR_NOT_SUPPORTED -8

/*******************************************************************************
 * Timeout Values (microseconds)
 ******************************************************************************/

#define DMA_TIMEOUT_US          10000000  /* 10 seconds */
#define DMA_POLL_INTERVAL_US    1         /* 1 microsecond */

#endif /* DMA_BENCHMARK_H */
