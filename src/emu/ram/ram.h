#pragma once

#include <stdint.h>
#include <stdio.h>

#define RAM_SIZE 0x10000

/**
 * Power on the RAM subsystem.
 */
int ram_power_on(void);

/**
 * Shutdown the RAM subsystem.
 */
void ram_shutdown(void);

/**
 * Read a value from RAM.
 * @param addr The address to read from.
 * @return The byte in the given address.
 */
uint8_t ram_read(uint16_t addr);

/**
 * Write a byte to the given address in RAM.
 * @param addr The address to write into.
 * @param value The value to write.
 */
void ram_write(uint16_t addr, uint8_t value);

/**
 * Insert data from file into RAM. Intended to be used by the emulator, and not during cpu runtime.
 * @param f The file to read from.
 * @param f_offset The offset into the file.
 * @param ram_offset The offset into the RAM.
 * @param size How many bytes to read.
 * @return 0 if success, nonzero otherwise.
 */
int ram_file_to_ram(FILE *f, size_t f_offset, uint16_t ram_offset, uint16_t size);