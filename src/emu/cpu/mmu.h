/*
 * Memory management unit, handles memory translation for the CPU (mirrorings, etc.)
 */

#pragma once

#include <nessan-gtr/types.h>
#include "cpudefs.h"

/**
 * Configure the mmu.
 */
void mmu_configure(enum memory_mode mode);

/**
* Read a value from RAM.
* @param addr The address to read from.
* @return The word in the given address.
*/
word_t mem_read(word_t addr);

/**
* Write a word to the given address in RAM.
* @param addr The address to write into.
* @param value The value to write.
*/
void mem_write(word_t addr, word_t value);