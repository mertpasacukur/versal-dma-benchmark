/**
 * @file qdma_benchmark.c
 * @brief QDMA Host-Side Benchmark Implementation
 *
 * Linux user-space application for benchmarking Xilinx QDMA.
 * Uses Xilinx QDMA Linux driver character device interface.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <getopt.h>

#include "qdma_benchmark.h"

/*******************************************************************************
 * Local Variables
 ******************************************************************************/

static int g_h2c_fd = -1;
static int g_c2h_fd = -1;
static void* g_host_buffer = NULL;
static size_t g_buffer_size = 0;

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

static uint64_t get_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static void fill_pattern(void* buffer, size_t size, uint32_t seed)
{
    uint32_t* p = (uint32_t*)buffer;
    size_t count = size / sizeof(uint32_t);

    for (size_t i = 0; i < count; i++) {
        p[i] = seed + i;
    }
}

static int verify_pattern(const void* buffer, size_t size, uint32_t seed)
{
    const uint32_t* p = (const uint32_t*)buffer;
    size_t count = size / sizeof(uint32_t);

    for (size_t i = 0; i < count; i++) {
        if (p[i] != seed + i) {
            return -1;
        }
    }
    return 0;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

int qdma_bench_init(const QdmaBenchConfig_t* config)
{
    char device_path[256];

    if (!config) {
        return -EINVAL;
    }

    /* Open H2C device */
    snprintf(device_path, sizeof(device_path), QDMA_DEVICE_PATH,
             config->bdf, 0);  /* Queue 0 */

    g_h2c_fd = open(device_path, O_RDWR);
    if (g_h2c_fd < 0) {
        fprintf(stderr, "Failed to open H2C device %s: %s\n",
                device_path, strerror(errno));
        return -errno;
    }

    /* Open C2H device */
    snprintf(device_path, sizeof(device_path), QDMA_DEVICE_PATH,
             config->bdf, 1);  /* Queue 1 for C2H */

    g_c2h_fd = open(device_path, O_RDWR);
    if (g_c2h_fd < 0) {
        fprintf(stderr, "Failed to open C2H device %s: %s\n",
                device_path, strerror(errno));
        close(g_h2c_fd);
        g_h2c_fd = -1;
        return -errno;
    }

    /* Allocate host buffer (page-aligned for DMA) */
    g_buffer_size = config->transfer_size;
    g_host_buffer = mmap(NULL, g_buffer_size * 2,  /* Double for TX and RX */
                         PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB,
                         -1, 0);

    if (g_host_buffer == MAP_FAILED) {
        /* Fall back to regular pages */
        g_host_buffer = mmap(NULL, g_buffer_size * 2,
                             PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS,
                             -1, 0);
    }

    if (g_host_buffer == MAP_FAILED) {
        fprintf(stderr, "Failed to allocate host buffer: %s\n", strerror(errno));
        close(g_h2c_fd);
        close(g_c2h_fd);
        g_h2c_fd = -1;
        g_c2h_fd = -1;
        return -ENOMEM;
    }

    printf("QDMA Benchmark initialized\n");
    printf("  H2C device: %s\n", device_path);
    printf("  Buffer size: %zu bytes\n", g_buffer_size);

    return 0;
}

void qdma_bench_cleanup(void)
{
    if (g_host_buffer && g_host_buffer != MAP_FAILED) {
        munmap(g_host_buffer, g_buffer_size * 2);
        g_host_buffer = NULL;
    }

    if (g_h2c_fd >= 0) {
        close(g_h2c_fd);
        g_h2c_fd = -1;
    }

    if (g_c2h_fd >= 0) {
        close(g_c2h_fd);
        g_c2h_fd = -1;
    }

    printf("QDMA Benchmark cleanup complete\n");
}

int qdma_bench_throughput(const QdmaBenchConfig_t* config, QdmaBenchResult_t* result)
{
    void* tx_buffer = g_host_buffer;
    void* rx_buffer = (char*)g_host_buffer + g_buffer_size;
    uint64_t start_time, elapsed_us;
    ssize_t bytes;
    uint32_t i;

    if (!config || !result || g_h2c_fd < 0) {
        return -EINVAL;
    }

    memset(result, 0, sizeof(*result));

    /* Fill TX buffer with pattern */
    fill_pattern(tx_buffer, config->transfer_size, 0xDEADBEEF);

    /* Warmup */
    for (i = 0; i < WARMUP_ITERATIONS; i++) {
        lseek(g_h2c_fd, 0, SEEK_SET);
        write(g_h2c_fd, tx_buffer, config->transfer_size);
    }

    /* H2C (Host to Card) throughput test */
    start_time = get_time_us();

    for (i = 0; i < config->iterations; i++) {
        lseek(g_h2c_fd, 0, SEEK_SET);
        bytes = write(g_h2c_fd, tx_buffer, config->transfer_size);
        if (bytes != (ssize_t)config->transfer_size) {
            result->errors++;
        }
    }

    elapsed_us = get_time_us() - start_time;
    result->total_bytes = (uint64_t)config->transfer_size * config->iterations;
    result->h2c_throughput_gbps = (double)result->total_bytes / elapsed_us / 1000.0;

    printf("H2C Throughput: %.2f GB/s (%llu bytes in %llu us)\n",
           result->h2c_throughput_gbps, result->total_bytes, elapsed_us);

    /* C2H (Card to Host) throughput test */
    if (config->direction == QDMA_DIR_C2H || config->direction == QDMA_DIR_BIDIR) {
        start_time = get_time_us();

        for (i = 0; i < config->iterations; i++) {
            lseek(g_c2h_fd, 0, SEEK_SET);
            bytes = read(g_c2h_fd, rx_buffer, config->transfer_size);
            if (bytes != (ssize_t)config->transfer_size) {
                result->errors++;
            }
        }

        elapsed_us = get_time_us() - start_time;
        result->c2h_throughput_gbps = (double)result->total_bytes / elapsed_us / 1000.0;

        printf("C2H Throughput: %.2f GB/s\n", result->c2h_throughput_gbps);
    }

    /* Data verification */
    if (config->verify_data) {
        /* Write pattern to card, read back and verify */
        fill_pattern(tx_buffer, config->transfer_size, 0x12345678);

        lseek(g_h2c_fd, 0, SEEK_SET);
        write(g_h2c_fd, tx_buffer, config->transfer_size);

        memset(rx_buffer, 0, config->transfer_size);
        lseek(g_c2h_fd, 0, SEEK_SET);
        read(g_c2h_fd, rx_buffer, config->transfer_size);

        if (verify_pattern(rx_buffer, config->transfer_size, 0x12345678) == 0) {
            result->data_valid = true;
            printf("Data verification: PASS\n");
        } else {
            result->data_valid = false;
            result->errors++;
            printf("Data verification: FAIL\n");
        }
    }

    return (result->errors == 0) ? 0 : -EIO;
}

int qdma_bench_latency(const QdmaBenchConfig_t* config, QdmaBenchResult_t* result)
{
    void* tx_buffer = g_host_buffer;
    uint64_t start_time, elapsed_ns;
    uint64_t total_h2c_ns = 0;
    uint32_t size = 64;  /* Small transfer for latency */
    uint32_t iterations = 10000;
    ssize_t bytes;

    if (!config || !result || g_h2c_fd < 0) {
        return -EINVAL;
    }

    memset(result, 0, sizeof(*result));

    /* Fill buffer */
    fill_pattern(tx_buffer, size, 0);

    /* Warmup */
    for (uint32_t i = 0; i < 100; i++) {
        lseek(g_h2c_fd, 0, SEEK_SET);
        write(g_h2c_fd, tx_buffer, size);
    }

    /* Measure H2C latency */
    for (uint32_t i = 0; i < iterations; i++) {
        start_time = get_time_us();

        lseek(g_h2c_fd, 0, SEEK_SET);
        bytes = write(g_h2c_fd, tx_buffer, size);

        elapsed_ns = (get_time_us() - start_time) * 1000;
        total_h2c_ns += elapsed_ns;

        if (bytes != (ssize_t)size) {
            result->errors++;
        }
    }

    result->h2c_latency_us = (double)total_h2c_ns / iterations / 1000.0;
    printf("H2C Latency: %.3f us (64-byte transfers)\n", result->h2c_latency_us);

    return 0;
}

int qdma_bench_multi_queue(const QdmaBenchConfig_t* config, QdmaBenchResult_t* result)
{
    printf("Multi-queue benchmark not implemented in this version.\n");
    printf("For multi-queue testing, use the Xilinx dma_perf utility.\n");
    return -ENOSYS;
}

int qdma_bench_run_all(const QdmaBenchConfig_t* config)
{
    QdmaBenchResult_t result;
    int status;

    printf("\n");
    printf("================================================================\n");
    printf("              QDMA Benchmark Suite\n");
    printf("================================================================\n\n");

    printf("Configuration:\n");
    printf("  BDF:           %05x\n", config->bdf);
    printf("  Transfer Size: %u bytes\n", config->transfer_size);
    printf("  Iterations:    %u\n", config->iterations);
    printf("  Mode:          %s\n", config->mode == QDMA_MODE_MM ? "MM" : "ST");
    printf("\n");

    /* Throughput test */
    printf("--- Throughput Test ---\n");
    status = qdma_bench_throughput(config, &result);
    if (status != 0) {
        printf("Throughput test failed: %d\n", status);
    }

    /* Latency test */
    printf("\n--- Latency Test ---\n");
    status = qdma_bench_latency(config, &result);
    if (status != 0) {
        printf("Latency test failed: %d\n", status);
    }

    /* Summary */
    printf("\n");
    printf("================================================================\n");
    printf("                    Summary\n");
    printf("================================================================\n");
    printf("  H2C Throughput:  %.2f GB/s (%.1f%% of PCIe Gen4 x8)\n",
           result.h2c_throughput_gbps,
           result.h2c_throughput_gbps / PCIE_GEN4_X8_BW_GBPS * 100.0);
    printf("  C2H Throughput:  %.2f GB/s\n", result.c2h_throughput_gbps);
    printf("  H2C Latency:     %.3f us\n", result.h2c_latency_us);
    printf("  Errors:          %u\n", result.errors);
    printf("================================================================\n\n");

    /* Export to CSV if requested */
    if (config->output_file) {
        qdma_bench_export_csv(config->output_file, &result);
    }

    return 0;
}

void qdma_bench_print_result(const QdmaBenchResult_t* result)
{
    printf("QDMA Benchmark Results:\n");
    printf("  H2C Throughput: %.2f GB/s\n", result->h2c_throughput_gbps);
    printf("  C2H Throughput: %.2f GB/s\n", result->c2h_throughput_gbps);
    printf("  H2C Latency:    %.3f us\n", result->h2c_latency_us);
    printf("  C2H Latency:    %.3f us\n", result->c2h_latency_us);
    printf("  Total Bytes:    %llu\n", result->total_bytes);
    printf("  Errors:         %u\n", result->errors);
    printf("  Data Valid:     %s\n", result->data_valid ? "Yes" : "No");
}

int qdma_bench_export_csv(const char* filename, const QdmaBenchResult_t* result)
{
    FILE* fp = fopen(filename, "a");
    if (!fp) {
        fprintf(stderr, "Failed to open %s: %s\n", filename, strerror(errno));
        return -errno;
    }

    /* Check if file is empty to add header */
    fseek(fp, 0, SEEK_END);
    if (ftell(fp) == 0) {
        fprintf(fp, "timestamp,h2c_gbps,c2h_gbps,h2c_latency_us,c2h_latency_us,"
                    "total_bytes,errors,data_valid\n");
    }

    /* Write result */
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(fp, "%s,%.3f,%.3f,%.3f,%.3f,%llu,%u,%s\n",
            timestamp,
            result->h2c_throughput_gbps,
            result->c2h_throughput_gbps,
            result->h2c_latency_us,
            result->c2h_latency_us,
            result->total_bytes,
            result->errors,
            result->data_valid ? "true" : "false");

    fclose(fp);
    printf("Results exported to %s\n", filename);
    return 0;
}

/*******************************************************************************
 * Main Function
 ******************************************************************************/

static void print_usage(const char* prog_name)
{
    printf("Usage: %s [options]\n\n", prog_name);
    printf("Options:\n");
    printf("  -d, --device BDF     PCIe Bus:Device:Function (default: 0x01000)\n");
    printf("  -s, --size SIZE      Transfer size in bytes (default: %d)\n", DEFAULT_TRANSFER_SIZE);
    printf("  -i, --iterations N   Number of iterations (default: %d)\n", DEFAULT_ITERATIONS);
    printf("  -m, --mode MODE      mm or st (default: mm)\n");
    printf("  -v, --verify         Enable data verification\n");
    printf("  -o, --output FILE    CSV output file\n");
    printf("  -h, --help           Show this help\n");
}

int main(int argc, char* argv[])
{
    QdmaBenchConfig_t config = {
        .bdf = 0x01000,
        .num_queues = QDMA_DEFAULT_QUEUES,
        .transfer_size = DEFAULT_TRANSFER_SIZE,
        .iterations = DEFAULT_ITERATIONS,
        .direction = QDMA_DIR_BIDIR,
        .mode = QDMA_MODE_MM,
        .verify_data = false,
        .verbose = false,
        .output_file = NULL
    };

    static struct option long_options[] = {
        {"device",     required_argument, 0, 'd'},
        {"size",       required_argument, 0, 's'},
        {"iterations", required_argument, 0, 'i'},
        {"mode",       required_argument, 0, 'm'},
        {"verify",     no_argument,       0, 'v'},
        {"output",     required_argument, 0, 'o'},
        {"help",       no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "d:s:i:m:vo:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                config.bdf = strtoul(optarg, NULL, 0);
                break;
            case 's':
                config.transfer_size = atoi(optarg);
                break;
            case 'i':
                config.iterations = atoi(optarg);
                break;
            case 'm':
                if (strcmp(optarg, "st") == 0) {
                    config.mode = QDMA_MODE_ST;
                } else {
                    config.mode = QDMA_MODE_MM;
                }
                break;
            case 'v':
                config.verify_data = true;
                break;
            case 'o':
                config.output_file = optarg;
                break;
            case 'h':
            default:
                print_usage(argv[0]);
                return (opt == 'h') ? 0 : 1;
        }
    }

    printf("QDMA Benchmark Application\n");
    printf("Target: VPK120 via PCIe Gen4 x8\n\n");

    int status = qdma_bench_init(&config);
    if (status != 0) {
        fprintf(stderr, "Failed to initialize QDMA benchmark: %d\n", status);
        fprintf(stderr, "\nMake sure:\n");
        fprintf(stderr, "  1. QDMA driver is loaded (lsmod | grep qdma)\n");
        fprintf(stderr, "  2. Device is detected (lspci | grep Xilinx)\n");
        fprintf(stderr, "  3. Queues are configured (dma-ctl qdma... q add)\n");
        return 1;
    }

    status = qdma_bench_run_all(&config);

    qdma_bench_cleanup();

    return status;
}
