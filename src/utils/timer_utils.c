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
    uint64_t start_cycles, end_cycles, elapsed_cycles;

    if (g_TimerInitialized) {
        return 0;
    }

    LOG_ALWAYS("Timer: Initializing PMU cycle counter\r\n");

    /* Enable PMU cycle counter */
    pmu_enable_cycle_counter();

    /* Set frequency */
    g_TimerFreqHz = APU_CLK_FREQ_HZ;

    /* Calibration test: measure 1ms delay */
    start_cycles = pmu_read_cycle_counter();
    usleep(1000);  /* 1ms delay */
    end_cycles = pmu_read_cycle_counter();
    elapsed_cycles = end_cycles - start_cycles;

    LOG_ALWAYS("Timer: Calibration test (1ms delay):\r\n");
    LOG_ALWAYS("  Start cycles:   %llu\r\n", (unsigned long long)start_cycles);
    LOG_ALWAYS("  End cycles:     %llu\r\n", (unsigned long long)end_cycles);
    LOG_ALWAYS("  Elapsed cycles: %llu\r\n", (unsigned long long)elapsed_cycles);
    LOG_ALWAYS("  Expected cycles: %llu (for 1ms @ %llu Hz)\r\n",
               (unsigned long long)(g_TimerFreqHz / 1000),
               (unsigned long long)g_TimerFreqHz);

    /* If elapsed is very different from expected, warn user */
    uint64_t expected = g_TimerFreqHz / 1000;
    if (elapsed_cycles < expected / 10 || elapsed_cycles > expected * 10) {
        LOG_ERROR("Timer: WARNING - PMU cycle counter may not be working correctly!\r\n");
        LOG_ERROR("Timer: Measured %llu cycles, expected ~%llu for 1ms\r\n",
                  (unsigned long long)elapsed_cycles, (unsigned long long)expected);
    }

    g_TimerInitialized = true;
    LOG_ALWAYS("Timer: Initialization complete\r\n");

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
