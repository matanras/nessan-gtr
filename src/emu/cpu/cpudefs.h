/*
 * Here are all public constants and typedefs related to the CPU.
 */

#pragma once

 /* The memory mode. */
enum memory_mode {
	NROM128,
	NROM256
};


/* Beginning of PRG ROM space. PRG ROM should be mapped into this address. */
#define PRG_ROM_SPACE_BEGIN 0x8000