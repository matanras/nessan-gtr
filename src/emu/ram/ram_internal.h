/*
 * Internal header for RAM subsystem.
 * Do not include outside RAM code.
 */

#pragma once

#include <stdbool.h>

struct ram_state {
	bool is_initialized;
	unsigned char *mem_pool;
};