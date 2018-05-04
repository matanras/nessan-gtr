#include <stdbool.h>
#include "mmu.h"
#include "cpu_internal.h"
#include "../ram/ram.h"

/*
* The memory mode. Affects memory translations.
*/
static enum memory_mode memory_mode;

/**
* Check whether the given address is between min and max (inclusive).
* @param addr The address to check.
* @param min The minimum address of the range.
* @param max The maximum address of the range.
* @returns True whether addr is or above min and is or below max.
*/
static inline bool is_in_range(uint16_t addr, uint16_t min, uint16_t max) {
	return addr >= min && addr <= max;
}

/**
* Translate the address according to the memory mode.
*/
static uint16_t translate_address(uint16_t addr) {

	if (is_in_range(addr, MEMREGION_SYSTEM_BEGIN, MEMREGION_SYSTEM_END)) {
		addr %= 2048;
	}
	else if (is_in_range(addr, MEMREGION_PPUIO_BEGIN, MEMREGION_PPUIO_END)) {
		addr = MEMREGION_PPUIO_BEGIN + (addr % 8);
	}
	else if (is_in_range(addr, MEMREGION_PRG_ROM_BEGIN, MEMREGION_PRG_ROM_END) &&
		memory_mode == NROM128) {
		/* 16KB mirroring in this region. */
		addr = MEMREGION_PRG_ROM_BEGIN + (addr % 0x4000);
	}

	return addr;
}

void mmu_configure(const enum memory_mode mode) {
	memory_mode = mode;
}

/**
* Read a value from RAM.
* @param addr The address to read from.
* @return The word in the given address.
*/
uint8_t mem_read(uint16_t addr) {
	return ram_read(translate_address(addr));
}

void mem_write(uint16_t addr, uint8_t value) {
	ram_write(translate_address(addr), value);
}