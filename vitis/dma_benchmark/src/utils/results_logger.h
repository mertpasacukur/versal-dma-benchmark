/**
 * @file results_logger.h
 * @brief Results Logger Header
 *
 * CSV and UART logging utilities for benchmark results.
 */

#ifndef RESULTS_LOGGER_H
#define RESULTS_LOGGER_H

#include <stdint.h>
#include <stdbool.h>
#include "../dma_benchmark.h"

/*******************************************************************************
 * Constants
 ******************************************************************************/

#define MAX_SESSION_NAME_LEN    64
#define MAX_CSV_LINE_LEN        512

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize results logger
 * @return 0 on success, negative error code on failure
 */
int results_logger_init(void);

/**
 * @brief Cleanup results logger
 */
void results_logger_cleanup(void);

/**
 * @brief Start a new logging session
 * @param session_name Name for this session (used in output)
 */
void results_logger_start_session(const char* session_name);

/**
 * @brief End current logging session
 */
void results_logger_end_session(void);

/**
 * @brief Log a single test result
 * @param result Test result to log
 */
void results_logger_log_result(const TestResult_t* result);

/**
 * @brief Log test result in CSV format (to UART)
 * @param result Test result to log
 */
void results_logger_log_csv(const TestResult_t* result);

/**
 * @brief Log test result in human-readable format
 * @param result Test result to log
 */
void results_logger_log_text(const TestResult_t* result);

/**
 * @brief Print CSV header line
 */
void results_logger_print_csv_header(void);

/**
 * @brief Print session summary
 */
void results_logger_print_summary(void);

/**
 * @brief Set output mode (CSV, text, or both)
 * @param csv_enabled Enable CSV output
 * @param text_enabled Enable text output
 */
void results_logger_set_mode(bool csv_enabled, bool text_enabled);

/**
 * @brief Log a message
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void results_logger_log(const char* format, ...);

/**
 * @brief Log an error message
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void results_logger_error(const char* format, ...);

/**
 * @brief Log a warning message
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void results_logger_warn(const char* format, ...);

/**
 * @brief Log test progress
 * @param current Current test number
 * @param total Total number of tests
 * @param test_name Current test name
 */
void results_logger_progress(uint32_t current, uint32_t total, const char* test_name);

/**
 * @brief Format throughput value
 * @param throughput_mbps Throughput in MB/s
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return Pointer to buffer
 */
char* results_logger_format_throughput(double throughput_mbps, char* buffer, uint32_t buffer_size);

/**
 * @brief Format size value with appropriate unit
 * @param size_bytes Size in bytes
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return Pointer to buffer
 */
char* results_logger_format_size(uint64_t size_bytes, char* buffer, uint32_t buffer_size);

/**
 * @brief Format time value with appropriate unit
 * @param time_us Time in microseconds
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return Pointer to buffer
 */
char* results_logger_format_time(double time_us, char* buffer, uint32_t buffer_size);

#endif /* RESULTS_LOGGER_H */
