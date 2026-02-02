/**
 * @file debug_print.c
 * @brief Debug Print Level System Implementation
 */

#include <stdio.h>
#include <stdarg.h>
#include "debug_print.h"
#include "xil_printf.h"

/*******************************************************************************
 * Local Variables
 ******************************************************************************/

/* Default log level is INFO */
static LogLevel_t g_CurrentLogLevel = LOG_LEVEL_INFO;

/* Level name strings */
static const char* g_LevelNames[] = {
    [LOG_LEVEL_ERROR]   = "ERROR",
    [LOG_LEVEL_WARNING] = "WARNING",
    [LOG_LEVEL_INFO]    = "INFO",
    [LOG_LEVEL_DEBUG]   = "DEBUG"
};

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

void debug_set_level(LogLevel_t level)
{
    if (level <= LOG_LEVEL_DEBUG) {
        g_CurrentLogLevel = level;
        xil_printf("[LOG] Level set to: %s\r\n", g_LevelNames[level]);
    }
}

LogLevel_t debug_get_level(void)
{
    return g_CurrentLogLevel;
}

const char* debug_level_to_string(LogLevel_t level)
{
    if (level <= LOG_LEVEL_DEBUG) {
        return g_LevelNames[level];
    }
    return "UNKNOWN";
}

void debug_print(LogLevel_t level, const char* prefix, const char* fmt, ...)
{
    va_list args;

    /* Only print if level is at or below current threshold */
    if (level > g_CurrentLogLevel) {
        return;
    }

    /* Print prefix for non-info messages */
    if (level != LOG_LEVEL_INFO) {
        xil_printf("[%s] ", prefix);
    }

    /* Print the formatted message */
    va_start(args, fmt);

    /* xil_printf doesn't support va_list, so we use a workaround */
    /* For Xilinx standalone, we'll use a simple approach */
    {
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        xil_printf("%s", buffer);
    }

    va_end(args);
}
