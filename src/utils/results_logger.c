/**
 * @file results_logger.c
 * @brief Results Logger Implementation
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "results_logger.h"
#include "debug_print.h"

/*******************************************************************************
 * Local Variables
 ******************************************************************************/

static char g_SessionName[MAX_SESSION_NAME_LEN] = "default";
static bool g_CsvEnabled = true;
static bool g_TextEnabled = true;
static bool g_SessionActive = false;
static uint32_t g_TestCount = 0;
static uint32_t g_PassCount = 0;
static uint32_t g_FailCount = 0;

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

int results_logger_init(void)
{
    g_CsvEnabled = true;
    g_TextEnabled = true;
    g_SessionActive = false;
    g_TestCount = 0;
    g_PassCount = 0;
    g_FailCount = 0;

    return 0;
}

void results_logger_cleanup(void)
{
    if (g_SessionActive) {
        results_logger_end_session();
    }
}

void results_logger_start_session(const char* session_name)
{
    if (session_name && strlen(session_name) < MAX_SESSION_NAME_LEN) {
        strncpy(g_SessionName, session_name, MAX_SESSION_NAME_LEN - 1);
        g_SessionName[MAX_SESSION_NAME_LEN - 1] = '\0';
    } else {
        strcpy(g_SessionName, "default");
    }

    g_SessionActive = true;
    g_TestCount = 0;
    g_PassCount = 0;
    g_FailCount = 0;

    LOG_RESULT("\r\n");
    LOG_RESULT("=== Starting Benchmark Session: %s ===\r\n", g_SessionName);
    LOG_RESULT("\r\n");

    if (g_CsvEnabled) {
        results_logger_print_csv_header();
    }
}

void results_logger_end_session(void)
{
    if (!g_SessionActive) {
        return;
    }

    LOG_RESULT("\r\n");
    LOG_RESULT("=== Session Complete: %s ===\r\n", g_SessionName);
    results_logger_print_summary();

    g_SessionActive = false;
}

void results_logger_log_result(const TestResult_t* result)
{
    if (!result) {
        return;
    }

    g_TestCount++;
    if (result->data_integrity) {
        g_PassCount++;
    } else {
        g_FailCount++;
    }

    if (g_CsvEnabled) {
        results_logger_log_csv(result);
    }

    if (g_TextEnabled) {
        results_logger_log_text(result);
    }
}

void results_logger_log_csv(const TestResult_t* result)
{
    if (!result) {
        return;
    }

    /* CSV format:
     * dma_type,test_type,src_memory,dst_memory,transfer_size,data_pattern,mode,
     * throughput_mbps,latency_us,cpu_util,integrity,iterations
     * Note: Using integer format for xil_printf compatibility
     */
    LOG_RESULT("%s,%s,%s,%s,%lu,%s,%s,%lu,%lu,%lu,%s,%lu\r\n",
               dma_type_to_string(result->dma_type),
               test_type_to_string(result->test_type),
               memory_region_to_string(result->src_region),
               memory_region_to_string(result->dst_region),
               (unsigned long)result->transfer_size,
               pattern_to_string(result->pattern),
               dma_mode_to_string(result->mode),
               (unsigned long)result->throughput_mbps,
               (unsigned long)result->latency_us,
               (unsigned long)result->cpu_utilization,
               result->data_integrity ? "PASS" : "FAIL",
               (unsigned long)result->iterations);
}

void results_logger_log_text(const TestResult_t* result)
{
    char size_str[32];
    char throughput_str[32];
    char latency_str[32];

    if (!result) {
        return;
    }

    results_logger_format_size(result->transfer_size, size_str, sizeof(size_str));
    results_logger_format_throughput(result->throughput_mbps, throughput_str, sizeof(throughput_str));
    results_logger_format_time(result->latency_us, result->latency_ns, latency_str, sizeof(latency_str));

    LOG_RESULT("  [%s] %s -> %s, Size: %s\r\n",
               dma_type_to_string(result->dma_type),
               memory_region_to_string(result->src_region),
               memory_region_to_string(result->dst_region),
               size_str);
    LOG_RESULT("         Throughput: %s, Latency: %s, %s\r\n",
               throughput_str,
               latency_str,
               result->data_integrity ? "PASS" : "FAIL");
}

void results_logger_print_csv_header(void)
{
    LOG_RESULT("dma_type,test_type,src_memory,dst_memory,transfer_size,"
               "data_pattern,mode,throughput_mbps,latency_us,cpu_util,"
               "integrity,iterations\r\n");
}

void results_logger_print_summary(void)
{
    uint32_t pass_rate = (g_TestCount > 0) ? ((g_PassCount * 100) / g_TestCount) : 0;

    LOG_RESULT("\r\nSession Summary:\r\n");
    LOG_RESULT("  Total Tests: %lu\r\n", (unsigned long)g_TestCount);
    LOG_RESULT("  Passed:      %lu\r\n", (unsigned long)g_PassCount);
    LOG_RESULT("  Failed:      %lu\r\n", (unsigned long)g_FailCount);
    LOG_RESULT("  Pass Rate:   %lu%%\r\n", (unsigned long)pass_rate);
    LOG_RESULT("\r\n");
}

void results_logger_set_mode(bool csv_enabled, bool text_enabled)
{
    g_CsvEnabled = csv_enabled;
    g_TextEnabled = text_enabled;
}

void results_logger_log(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LOG_RESULT("[INFO] ");
    /* Note: LOG_RESULT doesn't support va_list, so we use a simplified approach */
    LOG_RESULT("%s", format);
    va_end(args);
    LOG_RESULT("\r\n");
}

void results_logger_error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LOG_RESULT("[ERROR] ");
    LOG_RESULT("%s", format);
    va_end(args);
    LOG_RESULT("\r\n");
}

void results_logger_warn(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LOG_RESULT("[WARN] ");
    LOG_RESULT("%s", format);
    va_end(args);
    LOG_RESULT("\r\n");
}

void results_logger_progress(uint32_t current, uint32_t total, const char* test_name)
{
    uint32_t percent = (total > 0) ? ((current * 100) / total) : 0;
    LOG_RESULT("[%3lu%%] Test %lu/%lu: %s\r\n", percent, current, total, test_name);
}

char* results_logger_format_throughput(uint32_t throughput_mbps, char* buffer, uint32_t buffer_size)
{
    if (throughput_mbps >= 1000) {
        /* Show as GB/s with one decimal using integer math: e.g., 12345 MB/s = 12.3 GB/s */
        uint32_t gb = throughput_mbps / 1000;
        uint32_t gb_frac = (throughput_mbps % 1000) / 100;  /* First decimal */
        snprintf(buffer, buffer_size, "%lu.%lu GB/s", (unsigned long)gb, (unsigned long)gb_frac);
    } else {
        snprintf(buffer, buffer_size, "%lu MB/s", (unsigned long)throughput_mbps);
    }
    return buffer;
}

char* results_logger_format_size(uint64_t size_bytes, char* buffer, uint32_t buffer_size)
{
    if (size_bytes >= MB(1)) {
        snprintf(buffer, buffer_size, "%llu MB", size_bytes / MB(1));
    } else if (size_bytes >= KB(1)) {
        snprintf(buffer, buffer_size, "%llu KB", size_bytes / KB(1));
    } else {
        snprintf(buffer, buffer_size, "%llu B", size_bytes);
    }
    return buffer;
}

char* results_logger_format_time(uint32_t time_us, uint32_t time_ns, char* buffer, uint32_t buffer_size)
{
    if (time_us >= 1000000) {
        /* Show as seconds */
        uint32_t s = time_us / 1000000;
        uint32_t s_frac = (time_us % 1000000) / 10000;  /* Two decimals */
        snprintf(buffer, buffer_size, "%lu.%02lu s", (unsigned long)s, (unsigned long)s_frac);
    } else if (time_us >= 1000) {
        /* Show as milliseconds */
        uint32_t ms = time_us / 1000;
        uint32_t ms_frac = (time_us % 1000) / 10;  /* Two decimals */
        snprintf(buffer, buffer_size, "%lu.%02lu ms", (unsigned long)ms, (unsigned long)ms_frac);
    } else if (time_us > 0) {
        /* Show as microseconds */
        snprintf(buffer, buffer_size, "%lu us", (unsigned long)time_us);
    } else if (time_ns > 0) {
        /* Show as nanoseconds */
        snprintf(buffer, buffer_size, "%lu ns", (unsigned long)time_ns);
    } else {
        snprintf(buffer, buffer_size, "0 us");
    }
    return buffer;
}
