/**
 * @file data_patterns.c
 * @brief Data Pattern Generator Implementation
 */

#include <string.h>
#include "data_patterns.h"

/*******************************************************************************
 * PRNG State (xorshift128+)
 ******************************************************************************/

static uint64_t g_prng_state[2] = {0x12345678DEADBEEFULL, 0x87654321CAFEBABE};

/*******************************************************************************
 * PRNG Functions
 ******************************************************************************/

void pattern_seed_prng(uint32_t seed)
{
    g_prng_state[0] = ((uint64_t)seed << 32) | (seed ^ 0xDEADBEEF);
    g_prng_state[1] = ((uint64_t)seed << 16) | (seed ^ 0xCAFEBABE);

    /* Warm up PRNG */
    for (int i = 0; i < 20; i++) {
        pattern_get_random();
    }
}

uint32_t pattern_get_random(void)
{
    /* xorshift128+ algorithm */
    uint64_t s1 = g_prng_state[0];
    const uint64_t s0 = g_prng_state[1];

    g_prng_state[0] = s0;
    s1 ^= s1 << 23;
    g_prng_state[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);

    return (uint32_t)((g_prng_state[1] + s0) & 0xFFFFFFFF);
}

/*******************************************************************************
 * Pattern Fill Functions
 ******************************************************************************/

void pattern_fill(void* buffer, uint32_t size, DataPattern_t pattern, uint32_t seed)
{
    switch (pattern) {
        case PATTERN_INCREMENTAL:
            pattern_fill_incremental(buffer, size);
            break;
        case PATTERN_ALL_ONES:
            pattern_fill_all_ones(buffer, size);
            break;
        case PATTERN_ALL_ZEROS:
            pattern_fill_all_zeros(buffer, size);
            break;
        case PATTERN_RANDOM:
            pattern_fill_random(buffer, size, seed);
            break;
        case PATTERN_CHECKERBOARD:
            pattern_fill_checkerboard(buffer, size);
            break;
        default:
            memset(buffer, 0, size);
            break;
    }
}

void pattern_fill_incremental(void* buffer, uint32_t size)
{
    uint8_t* p = (uint8_t*)buffer;
    uint32_t i;

    /* Fill in 4-byte chunks for efficiency */
    uint32_t* p32 = (uint32_t*)buffer;
    uint32_t count32 = size / 4;

    for (i = 0; i < count32; i++) {
        uint32_t base = i * 4;
        p32[i] = (((base + 3) & 0xFF) << 24) |
                 (((base + 2) & 0xFF) << 16) |
                 (((base + 1) & 0xFF) << 8) |
                 ((base) & 0xFF);
    }

    /* Fill remaining bytes */
    for (i = count32 * 4; i < size; i++) {
        p[i] = (uint8_t)(i & 0xFF);
    }
}

void pattern_fill_all_ones(void* buffer, uint32_t size)
{
    memset(buffer, 0xFF, size);
}

void pattern_fill_all_zeros(void* buffer, uint32_t size)
{
    memset(buffer, 0x00, size);
}

void pattern_fill_random(void* buffer, uint32_t size, uint32_t seed)
{
    uint32_t* p32 = (uint32_t*)buffer;
    uint8_t* p8 = (uint8_t*)buffer;
    uint32_t count32 = size / 4;
    uint32_t i;

    /* Initialize PRNG with seed */
    pattern_seed_prng(seed);

    /* Fill in 4-byte chunks */
    for (i = 0; i < count32; i++) {
        p32[i] = pattern_get_random();
    }

    /* Fill remaining bytes */
    if (size % 4 != 0) {
        uint32_t last = pattern_get_random();
        for (i = count32 * 4; i < size; i++) {
            p8[i] = (uint8_t)(last & 0xFF);
            last >>= 8;
        }
    }
}

void pattern_fill_checkerboard(void* buffer, uint32_t size)
{
    uint8_t* p = (uint8_t*)buffer;
    uint32_t i;

    /* Fill in 4-byte chunks with 0xAA55AA55 */
    uint32_t* p32 = (uint32_t*)buffer;
    uint32_t count32 = size / 4;
    uint32_t pattern = 0xAA55AA55;

    for (i = 0; i < count32; i++) {
        p32[i] = pattern;
    }

    /* Fill remaining bytes */
    for (i = count32 * 4; i < size; i++) {
        p[i] = (i & 1) ? 0x55 : 0xAA;
    }
}

void pattern_fill_walking_ones(void* buffer, uint32_t size)
{
    uint8_t* p = (uint8_t*)buffer;
    uint32_t i;
    uint8_t bit = 0x01;

    for (i = 0; i < size; i++) {
        p[i] = bit;
        bit = (bit << 1) | (bit >> 7);  /* Rotate left */
    }
}

void pattern_fill_walking_zeros(void* buffer, uint32_t size)
{
    uint8_t* p = (uint8_t*)buffer;
    uint32_t i;
    uint8_t bit = 0xFE;

    for (i = 0; i < size; i++) {
        p[i] = bit;
        bit = (bit << 1) | 0x01;  /* Shift left, fill with 1 */
        if (bit == 0xFF) {
            bit = 0xFE;
        }
    }
}

/*******************************************************************************
 * Pattern Verify Functions
 ******************************************************************************/

bool pattern_verify(const void* buffer, uint32_t size, DataPattern_t pattern,
                   uint32_t seed, uint32_t* error_offset,
                   uint8_t* error_expected, uint8_t* error_actual)
{
    const uint8_t* p = (const uint8_t*)buffer;
    uint8_t expected;
    uint32_t i;

    /* For random pattern, we need to regenerate the sequence */
    if (pattern == PATTERN_RANDOM) {
        pattern_seed_prng(seed);
    }

    for (i = 0; i < size; i++) {
        switch (pattern) {
            case PATTERN_INCREMENTAL:
                expected = (uint8_t)(i & 0xFF);
                break;
            case PATTERN_ALL_ONES:
                expected = 0xFF;
                break;
            case PATTERN_ALL_ZEROS:
                expected = 0x00;
                break;
            case PATTERN_RANDOM:
                if ((i % 4) == 0) {
                    static uint32_t rand_val;
                    rand_val = pattern_get_random();
                    expected = (uint8_t)(rand_val & 0xFF);
                } else {
                    static uint32_t rand_val;
                    expected = (uint8_t)((rand_val >> ((i % 4) * 8)) & 0xFF);
                }
                break;
            case PATTERN_CHECKERBOARD:
                expected = (i & 1) ? 0x55 : 0xAA;
                break;
            default:
                expected = 0x00;
                break;
        }

        if (p[i] != expected) {
            if (error_offset) *error_offset = i;
            if (error_expected) *error_expected = expected;
            if (error_actual) *error_actual = p[i];
            return false;
        }
    }

    return true;
}

const char* pattern_get_name(DataPattern_t pattern)
{
    return pattern_to_string(pattern);
}
