#include <stdlib.h>
#include <string.h>
#include "ram.h"
#include "ram_internal.h"

static struct ram ram;

int ram_power_on(void) {
	if (ram.is_initialized)
		return 0;
	
	ram.mem_pool = malloc(RAM_SIZE);
	
	if (!ram.mem_pool)
		return -1;

	ram.is_initialized = true;
	return 0;
}

void ram_shutdown() {
	ram.is_initialized = false;
	free(ram.mem_pool);
	ram.mem_pool = NULL;
}

uint8_t ram_read(uint16_t addr) {
	if (!ram.is_initialized)
		return 0;

	return *(ram.mem_pool + addr);
}

void ram_write(uint16_t addr, uint8_t value) {
	if (!ram.is_initialized)
		return;

	*(ram.mem_pool + addr) = value;
}

int ram_file_to_ram(FILE *f, size_t f_offset, uint16_t ram_offset, uint16_t size) {
	if (!ram.is_initialized)
		return -1;
	
	if (fseek(f, f_offset, SEEK_SET))
		return -1;

	if (fread(ram.mem_pool + ram_offset, 1, size, f) != size)
		return -1;

	return 0;
}