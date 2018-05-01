#include <stdio.h>
#include <stdlib.h>
#include <nessan-gtr/ines.h>
#include "emu.h"
#include "ram/ram.h"
#include "cpu/cpu.h"
#include "cpu/cpudefs.h"

static int load_cartridge(FILE *nes_image, const struct ines2_header *hdr) {
	size_t cartridge_data_offset = sizeof(*hdr);
	uint16_t prg_rom_size = hdr->prg_size * PRG_ROM_UNIT;

	if (hdr->is_trainer_present)
		cartridge_data_offset += TRAINER_SIZE;

	if (fseek(nes_image, 0, SEEK_SET))
		return -1;
	
	if (ram_file_to_ram(nes_image, cartridge_data_offset, PRG_ROM_SPACE_BEGIN, prg_rom_size))
		return -1;

	return 0;
}

static int get_ines_header(FILE *nes_image, struct ines2_header *hdr) {
	if (fseek(nes_image, 0, SEEK_SET))
		return -1;

	if (fread(hdr, 1, sizeof(*hdr), nes_image) != sizeof(*hdr))
		return -1;

	return 0;
}

static size_t get_file_size(FILE *f) {
	long file_size;
	
	if (fseek(f, 0, SEEK_END) != 0)
		return 0;
	
	file_size = ftell(f);

	if (fseek(f, 0, SEEK_SET) != 0)
		return 0;

	return file_size;
}


int emu_init(char *fpath) {
	FILE *nes_image;
	char *buff;
	size_t image_size;
	struct ines2_header hdr;

	nes_image = fopen(fpath, "rb");
	
	if (!nes_image)
		return -1;
	
	image_size = get_file_size(nes_image);
	
	if (image_size == 0)
		goto err1;
	
	buff = malloc(image_size);
	
	if (!buff)
		goto err1;

	if (fread(buff, 1, image_size, nes_image) != image_size)
		goto err2;
	
	if (get_ines_header(nes_image, &hdr) || hdr.prg_size > 2)
		goto err2;

	if (ram_power_on())
		goto err2;

	load_cartridge(nes_image, &hdr);

	fclose(nes_image);
	
	if (cpu_power_on(hdr.prg_size == 1 ? NROM128 : NROM256))
		goto err3;

err1:
	fclose(nes_image);
	return -1;

err2:
	fclose(nes_image);
	free(buff);
	return -1;

err3:
	free(buff);
	return -1;
}
