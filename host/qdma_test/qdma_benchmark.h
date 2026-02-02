/**
 * @file qdma_benchmark.h
 * @brief QDMA Host-Side Benchmark Header
 *
 * Linux user-space application for benchmarking Xilinx QDMA
 * on VPK120 connected via PCIe Gen4 x8.
 */

#ifndef QDMA_BENCHMARK_H
#define QDMA_BENCHMARK_H

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define QDMA_DEVICE_PATH        "/dev/qdma%05x-MM-%d"
#define QDMA_DEVICE_PATH_ST     "/dev/qdma%05x-ST-%d"

#define QDMA_MAX_QUEUES         2048
#define QDMA_DEFAULT_QUEUES     16
#define QDMA_H2C_CHANNELS       4
#define QDMA_C2H_CHANNELS       4

/* PCIe Gen4 x8 theoretical bandwidth */
#define PCIE_GEN4_X8_BW_GBPS    15.75  /* GB/s unidirectional */

/* Transfer sizes for benchmarking */
#define MIN_TRANSFER_SIZE       64
#define MAX_TRANSFER_SIZE       (64 * 1024 * 1024)  /* 64MB */
#define DEFAULT_TRANSFER_SIZE   (1 * 1024 * 1024)   /* 1MB */

/* Default iterations */
#define DEFAULT_ITERATIONS      100
#define WARMUP_ITERATIONS       5

/*******************************************************************************
 * Types
 ******************************************************************************/

typedef enum {
    QDMA_DIR_H2C = 0,   /* Host to Card */
    QDMA_DIR_C2H,       /* Card to Host */
    QDMA_DIR_BIDIR      /* Bidirectional */
} QdmaDirection_t;

typedef enum {
    QDMA_MODE_MM = 0,   /* Memory Mapped */
    QDMA_MODE_ST        /* Streaming */
} QdmaMode_t;

typedef struct {
    uint32_t bdf;               /* Bus:Device:Function */
    uint32_t num_queues;        /* Number of queues to use */
    uint32_t transfer_size;     /* Transfer size in bytes */
    uint32_t iterations;        /* Number of iterations */
    QdmaDirection_t direction;  /* Transfer direction */
    QdmaMode_t mode;            /* MM or ST mode */
    bool verify_data;           /* Enable data verification */
    bool verbose;               /* Verbose output */
    char* output_file;          /* CSV output file path */
} QdmaBenchConfig_t;

typedef struct {
    double h2c_throughput_gbps;
    double c2h_throughput_gbps;
    double h2c_latency_us;
    double c2h_latency_us;
    uint64_t total_bytes;
    uint64_t total_time_us;
    uint32_t errors;
    bool data_valid;
} QdmaBenchResult_t;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize QDMA benchmark
 * @param config Benchmark configuration
 * @return 0 on success, negative error code on failure
 */
int qdma_bench_init(const QdmaBenchConfig_t* config);

/**
 * @brief Cleanup QDMA benchmark resources
 */
void qdma_bench_cleanup(void);

/**
 * @brief Run QDMA throughput benchmark
 * @param config Benchmark configuration
 * @param result Benchmark result output
 * @return 0 on success, negative error code on failure
 */
int qdma_bench_throughput(const QdmaBenchConfig_t* config, QdmaBenchResult_t* result);

/**
 * @brief Run QDMA latency benchmark
 * @param config Benchmark configuration
 * @param result Benchmark result output
 * @return 0 on success, negative error code on failure
 */
int qdma_bench_latency(const QdmaBenchConfig_t* config, QdmaBenchResult_t* result);

/**
 * @brief Run QDMA multi-queue benchmark
 * @param config Benchmark configuration
 * @param result Benchmark result output
 * @return 0 on success, negative error code on failure
 */
int qdma_bench_multi_queue(const QdmaBenchConfig_t* config, QdmaBenchResult_t* result);

/**
 * @brief Run full QDMA benchmark suite
 * @param config Benchmark configuration
 * @return 0 on success, negative error code on failure
 */
int qdma_bench_run_all(const QdmaBenchConfig_t* config);

/**
 * @brief Print benchmark results to stdout
 * @param result Benchmark result
 */
void qdma_bench_print_result(const QdmaBenchResult_t* result);

/**
 * @brief Export benchmark results to CSV
 * @param filename Output CSV filename
 * @param result Benchmark result
 * @return 0 on success, negative error code on failure
 */
int qdma_bench_export_csv(const char* filename, const QdmaBenchResult_t* result);

#endif /* QDMA_BENCHMARK_H */
