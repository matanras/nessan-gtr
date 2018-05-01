#pragma once

/**
 * Power on the CPU.
 */
int cpu_power_on(enum memory_mode mem_mode);

/**
 * Reset the CPU. CPU must be powered on for this function to succeed.
 */
int cpu_reset(void);