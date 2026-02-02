/**
 * @file cache_utils.c
 * @brief Cache Management Utilities Implementation
 */

#include "cache_utils.h"
#include "xil_cache.h"

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

void cache_flush_range(uint64_t addr, uint32_t size)
{
    Xil_DCacheFlushRange((UINTPTR)addr, size);
}

void cache_invalidate_range(uint64_t addr, uint32_t size)
{
    Xil_DCacheInvalidateRange((UINTPTR)addr, size);
}

void cache_flush_invalidate_range(uint64_t addr, uint32_t size)
{
    Xil_DCacheFlushRange((UINTPTR)addr, size);
    Xil_DCacheInvalidateRange((UINTPTR)addr, size);
}

void cache_flush_all(void)
{
    Xil_DCacheFlush();
}

void cache_invalidate_all(void)
{
    Xil_DCacheInvalidate();
}

void cache_enable(void)
{
    Xil_DCacheEnable();
}

void cache_disable(void)
{
    Xil_DCacheDisable();
}

int cache_is_enabled(void)
{
    /* Read SCTLR_EL1 to check cache status */
    uint64_t sctlr;
    __asm__ __volatile__("mrs %0, sctlr_el1" : "=r" (sctlr));
    return (sctlr & (1 << 2)) ? 1 : 0;  /* Bit 2 = C (data cache enable) */
}

void cache_memory_barrier(void)
{
    __asm__ __volatile__("dsb sy" ::: "memory");
}

void cache_instruction_barrier(void)
{
    __asm__ __volatile__("isb" ::: "memory");
}

void cache_prep_dma_src(uint64_t addr, uint32_t size)
{
    /* Flush source buffer to ensure DMA reads current data */
    Xil_DCacheFlushRange((UINTPTR)addr, size);
    __asm__ __volatile__("dsb sy" ::: "memory");
}

void cache_prep_dma_dst(uint64_t addr, uint32_t size)
{
    /* Invalidate destination buffer before DMA writes */
    Xil_DCacheInvalidateRange((UINTPTR)addr, size);
    __asm__ __volatile__("dsb sy" ::: "memory");
}

void cache_complete_dma_dst(uint64_t addr, uint32_t size)
{
    /* Invalidate destination buffer after DMA to see new data */
    __asm__ __volatile__("dsb sy" ::: "memory");
    Xil_DCacheInvalidateRange((UINTPTR)addr, size);
}
