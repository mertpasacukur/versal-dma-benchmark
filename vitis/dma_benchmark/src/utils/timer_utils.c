/**
 * @file timer_utils.c
 * @brief High-Resolution Timer Utilities Implementation
 *
 * Uses ARM PMU cycle counter for high-resolution timing.
 */

#include <stdbool.h>
#include "timer_utils.h"
#include "debug_print.h"
#include "sleep.h"
#include "../platform_config.h"

/*******************************************************************************
 * Local Variables
 ******************************************************************************/

static uint64_t g_TimerFreqHz = APU_CLK_FREQ_HZ;
static bool g_TimerInitialized = false;

/*******************************************************************************
 * ARM PMU Access Functions
 ******************************************************************************/

static inline void pmu_enable_cycle_counter(void)
{
    uint64_t val;

    /* Enable user-mode access to PMU */
    __asm__ __volatile__("mrs %0, pmuserenr_el0" : "=r" (val));
    val |= (1 << 0);  /* Enable EL0 access */
    __asm__ __volatile__("msr pmuserenr_el0, %0" : : "r" (val));

    /* Enable cycle counter */
    __asm__ __volatile__("mrs %0, pmcr_el0" : "=r" (val));
    val |= (1 << 0);  /* Enable */
    val |= (1 << 2);  /* Reset cycle counter */
    __asm__ __volatile__("msr pmcr_el0, %0" : : "r" (val));

    /* Enable cycle counter specifically */
    __asm__ __volatile__("mrs %0, pmcntenset_el0" : "=r" (val));
    val |= (1 << 31);  /* Enable cycle counter */
    __asm__ __volatile__("msr pmcntenset_el0, %0" : : "r" (val));

    /* Instruction barrier */
    __asm__ __volatile__("isb");
}

static inline uint64_t pmu_read_cycle_counter(void)
{
    uint64_t val;
    __asm__ __volatile__("mrs %0, pmccntr_el0" : "=r" (val));
    return val;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

int timer_init(void)
{
    if (g_TimerInitialized) {
        return 0;
    }

    LOG_DEBUG("Timer: Initializing PMU cycle counter\r\n");

    /* Enable PMU cycle counter */
    pmu_enable_cycle_counter();

    /* Calibrate timer frequency if needed */
    g_TimerFreqHz = APU_CLK_FREQ_HZ;

    g_TimerInitialized = true;
    LOG_DEBUG("Timer: Frequency = %llu Hz\r\n", g_TimerFreqHz);

    return 0;
}

void timer_cleanup(void)
{
    g_TimerInitialized = false;
}

uint64_t timer_get_us(void)
{
    return timer_cycles_to_us(pmu_read_cycle_counter());
}

uint64_t timer_get_ns(void)
{
    return timer_cycles_to_ns(pmu_read_cycle_counter());
}

uint64_t timer_get_cycles(void)
{
    return pmu_read_cycle_counter();
}

uint64_t timer_cycles_to_us(uint64_t cycles)
{
    return (cycles * 1000000ULL) / g_TimerFreqHz;
}

uint64_t timer_cycles_to_ns(uint64_t cycles)
{
    return (cycles * 1000000000ULL) / g_TimerFreqHz;
}

uint64_t timer_start(void)
{
    /* Memory barrier before reading */
    __asm__ __volatile__("dsb sy" ::: "memory");
    return pmu_read_cycle_counter();
}

uint64_t timer_stop_us(uint64_t start_time)
{
    uint64_t end_time;
    /* Memory barrier before reading */
    __asm__ __volatile__("dsb sy" ::: "memory");
    end_time = pmu_read_cycle_counter();
    return timer_cycles_to_us(end_time - start_time);
}

uint64_t timer_stop_ns(uint64_t start_time)
{
    uint64_t end_time;
    /* Memory barrier before reading */
    __asm__ __volatile__("dsb sy" ::: "memory");
    end_time = pmu_read_cycle_counter();
    return timer_cycles_to_ns(end_time - start_time);
}

void timer_delay_us(uint32_t us)
{
    usleep(us);
}

uint64_t timer_get_frequency(void)
{
    return g_TimerFreqHz;
}
