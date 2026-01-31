/**
 * @file debug_print.h
 * @brief Debug Print Level System for DMA Benchmark
 *
 * Provides configurable print levels: ERROR, WARNING, INFO, DEBUG
 * Default level is INFO. Use debug_set_level() to change at runtime.
 */

#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H

#include <stdarg.h>
#include "xil_printf.h"

/*******************************************************************************
 * Print Levels
 ******************************************************************************/

typedef enum {
    LOG_LEVEL_ERROR = 0,    /* Always printed - critical errors */
    LOG_LEVEL_WARNING,      /* Important warnings */
    LOG_LEVEL_INFO,         /* General information (default) */
    LOG_LEVEL_DEBUG         /* Detailed debug information */
} LogLevel_t;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Set the current log level
 * @param level New log level (LOG_LEVEL_ERROR to LOG_LEVEL_DEBUG)
 */
void debug_set_level(LogLevel_t level);

/**
 * @brief Get the current log level
 * @return Current log level
 */
LogLevel_t debug_get_level(void);

/**
 * @brief Get log level name string
 * @param level Log level
 * @return String representation of the level
 */
const char* debug_level_to_string(LogLevel_t level);

/**
 * @brief Internal print function - use macros instead
 */
void debug_print(LogLevel_t level, const char* prefix, const char* fmt, ...);

/*******************************************************************************
 * Print Macros
 ******************************************************************************/

/**
 * LOG_ERROR - Always printed. Use for critical errors that prevent operation.
 * Example: LOG_ERROR("DMA transfer failed with code %d\r\n", status);
 */
#define LOG_ERROR(fmt, ...) \
    debug_print(LOG_LEVEL_ERROR, "ERROR", fmt, ##__VA_ARGS__)

/**
 * LOG_WARNING - Printed at WARNING level and above.
 * Use for recoverable issues or important notices.
 * Example: LOG_WARNING("Transfer timeout, retrying...\r\n");
 */
#define LOG_WARNING(fmt, ...) \
    debug_print(LOG_LEVEL_WARNING, "WARN", fmt, ##__VA_ARGS__)

/**
 * LOG_INFO - Printed at INFO level and above (default).
 * Use for general status messages and results.
 * Example: LOG_INFO("Test passed: %.2f MB/s\r\n", throughput);
 */
#define LOG_INFO(fmt, ...) \
    debug_print(LOG_LEVEL_INFO, "INFO", fmt, ##__VA_ARGS__)

/**
 * LOG_DEBUG - Only printed at DEBUG level.
 * Use for detailed diagnostic information.
 * Example: LOG_DEBUG("Register value: 0x%08X\r\n", reg);
 */
#define LOG_DEBUG(fmt, ...) \
    debug_print(LOG_LEVEL_DEBUG, "DEBUG", fmt, ##__VA_ARGS__)

/**
 * LOG_ALWAYS - Always printed regardless of level, no prefix.
 * Use for banners, menus, and user-facing output.
 * Example: LOG_ALWAYS("=== DMA Benchmark Menu ===\r\n");
 */
#define LOG_ALWAYS(fmt, ...) \
    xil_printf(fmt, ##__VA_ARGS__)

/**
 * LOG_RESULT - Always printed, for test results.
 * Example: LOG_RESULT("Throughput: %.2f MB/s\r\n", result);
 */
#define LOG_RESULT(fmt, ...) \
    xil_printf(fmt, ##__VA_ARGS__)

#endif /* DEBUG_PRINT_H */
