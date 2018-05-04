/*
 * Internal header for RAM subsystem.
 * Do not include outside RAM code.
 */

#pragma once

#include <stdbool.h>

struct ram {
	bool is_initialized;
	uint8_t *mem_pool;
};