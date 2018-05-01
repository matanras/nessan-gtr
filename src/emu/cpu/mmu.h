/*
 * Memory management unit, handles memory translation for the CPU (mirrorings, etc.)
 */

#pragma once

#include <stdint.h>
#include "cpudefs.h"

/**
 * Configure the mmu.
 */
void mmu_configure(enum memory_mode mode);

/**
* Read a value from RAM.
* @param addr The address to read from.
* @return The byte in the given address.
*/
uint8_t mem_read(uint16_t addr);

/**
* Write a byte to the given address in RAM.
* @param addr The address to write into.
* @param value The value to write.
*/
void mem_write(uint16_t addr, uint8_t value);