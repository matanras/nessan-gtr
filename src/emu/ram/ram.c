#include <stdlib.h>
#include <string.h>
#include "ram.h"
#include "ram_internal.h"

static struct ram_state state;

int ram_power_on(void) {
	if (state.is_initialized)
		return 0;
	
	state.mem_pool = malloc(RAM_SIZE);
	
	if (!state.mem_pool)
		return -1;

	state.is_initialized = true;
	return 0;
}

void ram_shutdown() {
	state.is_initialized = false;
	free(state.mem_pool);
	state.mem_pool = NULL;
}

word_t ram_read(word_t addr) {
	if (!state.is_initialized)
		return 0;

	return *(word_t *)(state.mem_pool + addr);
}

void ram_write(word_t addr, word_t value) {
	if (!state.is_initialized)
		return;

	*(word_t *)(state.mem_pool + addr) = value;
}

int ram_file_to_ram(FILE *f, size_t f_offset, word_t ram_offset, word_t size) {
	if (!state.is_initialized)
		return -1;
	
	if (fseek(f, f_offset, SEEK_SET))
		return -1;

	if (fread(state.mem_pool + ram_offset, 1, size, f) != size)
		return -1;

	return 0;
}