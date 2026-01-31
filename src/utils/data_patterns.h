/**
 * @file data_patterns.h
 * @brief Data Pattern Generator Header
 *
 * Test data pattern generation and verification utilities.
 */

#ifndef DATA_PATTERNS_H
#define DATA_PATTERNS_H

#include <stdint.h>
#include <stdbool.h>
#include "../dma_benchmark.h"

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Fill buffer with specified pattern
 * @param buffer Buffer to fill
 * @param size Buffer size in bytes
 * @param pattern Pattern type
 * @param seed Random seed (for PATTERN_RANDOM)
 */
void pattern_fill(void* buffer, uint32_t size, DataPattern_t pattern, uint32_t seed);

/**
 * @brief Verify buffer contains expected pattern
 * @param buffer Buffer to verify
 * @param size Buffer size in bytes
 * @param pattern Expected pattern type
 * @param seed Random seed (for PATTERN_RANDOM)
 * @param error_offset Output: offset of first error (if any)
 * @param error_expected Output: expected value at error offset
 * @param error_actual Output: actual value at error offset
 * @return true if buffer matches pattern, false otherwise
 */
bool pattern_verify(const void* buffer, uint32_t size, DataPattern_t pattern,
                   uint32_t seed, uint32_t* error_offset,
                   uint8_t* error_expected, uint8_t* error_actual);

/**
 * @brief Generate incremental pattern
 * @param buffer Buffer to fill
 * @param size Buffer size in bytes
 */
void pattern_fill_incremental(void* buffer, uint32_t size);

/**
 * @brief Generate all-ones pattern (0xFF)
 * @param buffer Buffer to fill
 * @param size Buffer size in bytes
 */
void pattern_fill_all_ones(void* buffer, uint32_t size);

/**
 * @brief Generate all-zeros pattern (0x00)
 * @param buffer Buffer to fill
 * @param size Buffer size in bytes
 */
void pattern_fill_all_zeros(void* buffer, uint32_t size);

/**
 * @brief Generate random pattern using PRNG
 * @param buffer Buffer to fill
 * @param size Buffer size in bytes
 * @param seed Random seed
 */
void pattern_fill_random(void* buffer, uint32_t size, uint32_t seed);

/**
 * @brief Generate checkerboard pattern (0xAA, 0x55, 0xAA, ...)
 * @param buffer Buffer to fill
 * @param size Buffer size in bytes
 */
void pattern_fill_checkerboard(void* buffer, uint32_t size);

/**
 * @brief Generate walking ones pattern
 * @param buffer Buffer to fill
 * @param size Buffer size in bytes
 */
void pattern_fill_walking_ones(void* buffer, uint32_t size);

/**
 * @brief Generate walking zeros pattern
 * @param buffer Buffer to fill
 * @param size Buffer size in bytes
 */
void pattern_fill_walking_zeros(void* buffer, uint32_t size);

/**
 * @brief Get pattern name string
 * @param pattern Pattern type
 * @return Pattern name string
 */
const char* pattern_get_name(DataPattern_t pattern);

/**
 * @brief Initialize PRNG with seed
 * @param seed Initial seed value
 */
void pattern_seed_prng(uint32_t seed);

/**
 * @brief Get next random value from PRNG
 * @return Random 32-bit value
 */
uint32_t pattern_get_random(void);

#endif /* DATA_PATTERNS_H */
