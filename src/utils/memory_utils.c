/**
 * @file memory_utils.c
 * @brief Memory Utilities Implementation
 */

#include <string.h>
#include "memory_utils.h"
#include "timer_utils.h"
#include "debug_print.h"
#include "xil_cache.h"
#include "../dma_benchmark.h"

/*******************************************************************************
 * Static Memory Pools for Test Regions
 ******************************************************************************/

/* Track allocations from each region */
static uint32_t g_RegionOffsets[MEM_REGION_COUNT] = {0};

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

void* memory_alloc_aligned(MemoryRegion_t region, uint32_t size, uint32_t alignment)
{
    uint64_t base;
    uint64_t aligned_addr;
    uint32_t current_offset;
    const MemoryRegionInfo_t* info;

    if (region >= MEM_REGION_COUNT || region == MEM_REGION_HOST) {
        return NULL;
    }

    info = &g_MemoryRegions[region];
    current_offset = g_RegionOffsets[region];

    /* Calculate aligned address */
    base = info->test_base + current_offset;
    aligned_addr = ALIGN_UP(base, alignment);

    /* Check if allocation fits */
    if ((aligned_addr - info->test_base) + size > info->test_size) {
        LOG_WARNING("Allocation failed in %s (size=%u)\r\n", info->name, size);
        return NULL;
    }

    /* Update offset for next allocation */
    g_RegionOffsets[region] = (aligned_addr - info->test_base) + size;

    return (void*)(uintptr_t)aligned_addr;
}

void memory_free_aligned(void* ptr)
{
    /* Simple allocator - no individual frees, use reset instead */
    (void)ptr;
}

uint64_t memory_get_test_addr(MemoryRegion_t region, uint32_t offset, uint32_t size)
{
    const MemoryRegionInfo_t* info;

    LOG_DEBUG("memory_get_test_addr: region=%d, offset=0x%lX, size=%lu\r\n",
              (int)region, (unsigned long)offset, (unsigned long)size);

    if (region >= MEM_REGION_COUNT || region == MEM_REGION_HOST) {
        LOG_ERROR("memory_get_test_addr: Invalid region %d\r\n", (int)region);
        return 0;
    }

    info = &g_MemoryRegions[region];

    LOG_DEBUG("memory_get_test_addr: region '%s', test_base=0x%llX, test_size=0x%llX\r\n",
              info->name, (unsigned long long)info->test_base, (unsigned long long)info->test_size);

    /* Check if requested range fits */
    if (offset + size > info->test_size) {
        LOG_ERROR("memory_get_test_addr: Range doesn't fit! offset+size=0x%llX > test_size=0x%llX\r\n",
                  (unsigned long long)(offset + size), (unsigned long long)info->test_size);
        return 0;
    }

    LOG_DEBUG("memory_get_test_addr: returning 0x%llX\r\n",
              (unsigned long long)(info->test_base + offset));
    return info->test_base + offset;
}

bool memory_is_valid_range(MemoryRegion_t region, uint64_t addr, uint32_t size)
{
    const MemoryRegionInfo_t* info;

    if (region >= MEM_REGION_COUNT || region == MEM_REGION_HOST) {
        return false;
    }

    info = &g_MemoryRegions[region];

    /* Check if address is within test region */
    if (addr < info->test_base) {
        return false;
    }

    if (addr + size > info->test_base + info->test_size) {
        return false;
    }

    return true;
}

bool memory_compare(const void* buf1, const void* buf2, uint32_t size, uint32_t* first_diff)
{
    const uint8_t* p1 = (const uint8_t*)buf1;
    const uint8_t* p2 = (const uint8_t*)buf2;
    uint32_t i;

    /* Compare in 64-bit chunks for speed */
    const uint64_t* p1_64 = (const uint64_t*)buf1;
    const uint64_t* p2_64 = (const uint64_t*)buf2;
    uint32_t count_64 = size / 8;

    for (i = 0; i < count_64; i++) {
        if (p1_64[i] != p2_64[i]) {
            /* Find exact byte */
            uint32_t byte_offset = i * 8;
            for (uint32_t j = 0; j < 8; j++) {
                if (p1[byte_offset + j] != p2[byte_offset + j]) {
                    if (first_diff) {
                        *first_diff = byte_offset + j;
                    }
                    return false;
                }
            }
        }
    }

    /* Compare remaining bytes */
    for (i = count_64 * 8; i < size; i++) {
        if (p1[i] != p2[i]) {
            if (first_diff) {
                *first_diff = i;
            }
            return false;
        }
    }

    return true;
}

bool memory_verify_pattern(const void* buf, uint32_t size, uint8_t pattern_type, uint32_t* first_diff)
{
    const uint8_t* p = (const uint8_t*)buf;
    uint8_t expected;
    uint32_t i;

    for (i = 0; i < size; i++) {
        switch (pattern_type) {
            case PATTERN_INCREMENTAL:
                expected = (uint8_t)(i & 0xFF);
                break;
            case PATTERN_ALL_ONES:
                expected = 0xFF;
                break;
            case PATTERN_ALL_ZEROS:
                expected = 0x00;
                break;
            case PATTERN_CHECKERBOARD:
                expected = (i & 1) ? 0x55 : 0xAA;
                break;
            default:
                return false;
        }

        if (p[i] != expected) {
            if (first_diff) {
                *first_diff = i;
            }
            return false;
        }
    }

    return true;
}

double memory_cpu_memcpy_benchmark(void* dst, const void* src, uint32_t size, uint32_t iterations)
{
    uint64_t start_time, elapsed_us;
    uint32_t i;

    /* Warmup */
    for (i = 0; i < 3; i++) {
        memcpy(dst, src, size);
    }

    /* Flush caches */
    Xil_DCacheFlushRange((UINTPTR)src, size);
    Xil_DCacheInvalidateRange((UINTPTR)dst, size);

    /* Measure */
    start_time = timer_start();

    for (i = 0; i < iterations; i++) {
        memcpy(dst, src, size);
        /* Memory barrier to ensure completion */
        __asm__ __volatile__("dsb sy" ::: "memory");
    }

    elapsed_us = timer_stop_us(start_time);

    /* Calculate throughput */
    uint64_t total_bytes = (uint64_t)size * iterations;
    return CALC_THROUGHPUT_MBPS(total_bytes, elapsed_us);
}

uint64_t memory_get_max_size(MemoryRegion_t region)
{
    if (region >= MEM_REGION_COUNT || region == MEM_REGION_HOST) {
        return 0;
    }
    return g_MemoryRegions[region].test_size;
}

void memory_print_region_info(MemoryRegion_t region)
{
    const MemoryRegionInfo_t* info;

    if (region >= MEM_REGION_COUNT) {
        return;
    }

    info = &g_MemoryRegions[region];

    LOG_RESULT("  %s:\r\n", info->name);
    LOG_RESULT("    Base:      0x%016llX\r\n", info->base_addr);
    LOG_RESULT("    Size:      %llu KB\r\n", info->size / 1024);
    LOG_RESULT("    Test Base: 0x%016llX\r\n", info->test_base);
    LOG_RESULT("    Test Size: %llu KB\r\n", info->test_size / 1024);
    LOG_RESULT("    Cacheable: %s\r\n", info->cacheable ? "Yes" : "No");
}

void memory_print_all_regions(void)
{
    uint32_t i;

    LOG_RESULT("\r\n=== Memory Regions ===\r\n");
    for (i = 0; i < MEM_REGION_COUNT; i++) {
        if (i != MEM_REGION_HOST) {
            memory_print_region_info((MemoryRegion_t)i);
        }
    }
    LOG_RESULT("======================\r\n");
}
