/**
 * @file timer_utils.h
 * @brief High-Resolution Timer Utilities Header
 *
 * Performance timing utilities using ARM PMU and TTC.
 */

#ifndef TIMER_UTILS_H
#define TIMER_UTILS_H

#include <stdint.h>

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize timer subsystem
 * @return 0 on success, negative error code on failure
 */
int timer_init(void);

/**
 * @brief Cleanup timer subsystem
 */
void timer_cleanup(void);

/**
 * @brief Get current time in microseconds
 * @return Time in microseconds since initialization
 */
uint64_t timer_get_us(void);

/**
 * @brief Get current time in nanoseconds
 * @return Time in nanoseconds since initialization
 */
uint64_t timer_get_ns(void);

/**
 * @brief Get raw cycle count from PMU
 * @return Raw cycle count
 */
uint64_t timer_get_cycles(void);

/**
 * @brief Convert cycles to microseconds
 * @param cycles Cycle count
 * @return Time in microseconds
 */
uint64_t timer_cycles_to_us(uint64_t cycles);

/**
 * @brief Convert cycles to nanoseconds
 * @param cycles Cycle count
 * @return Time in nanoseconds
 */
uint64_t timer_cycles_to_ns(uint64_t cycles);

/**
 * @brief Start a measurement
 * @return Start time in cycles
 */
uint64_t timer_start(void);

/**
 * @brief Stop a measurement and return elapsed time in microseconds
 * @param start_time Start time returned by timer_start()
 * @return Elapsed time in microseconds
 */
uint64_t timer_stop_us(uint64_t start_time);

/**
 * @brief Stop a measurement and return elapsed time in nanoseconds
 * @param start_time Start time returned by timer_start()
 * @return Elapsed time in nanoseconds
 */
uint64_t timer_stop_ns(uint64_t start_time);

/**
 * @brief Busy-wait delay in microseconds
 * @param us Delay in microseconds
 */
void timer_delay_us(uint32_t us);

/**
 * @brief Get timer frequency in Hz
 * @return Timer frequency in Hz
 */
uint64_t timer_get_frequency(void);

/*******************************************************************************
 * Measurement Helper Macros
 ******************************************************************************/

#define TIMER_MEASURE_START() \
    uint64_t _timer_start = timer_start()

#define TIMER_MEASURE_END_US() \
    timer_stop_us(_timer_start)

#define TIMER_MEASURE_END_NS() \
    timer_stop_ns(_timer_start)

#endif /* TIMER_UTILS_H */
