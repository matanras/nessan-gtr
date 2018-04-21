#include <stdio.h>
#include <stdlib.h>
#define OPTPARSE_IMPLEMENTATION
#include <optparse.h>
#include "disass.h"

void usage() {
    puts("disass <input file> [-n X]");
}

void dump_header(FILE *nes_image) {
	struct ines2_header hdr;
	
	if (disass_get_header(nes_image, &hdr) != 0) {
		puts("Invalid header!");
		return;
	}

	if (hdr.is_nes2 == INES_HEADER_V2_EXTENSION) {
		puts("iNES header /w NES 2.0 extension:");
	}
	else {
		puts("iNes header:");
	}

	printf("PRG size: %dKB\n", hdr.prg_size * PRG_ROM_UNIT);
	if (hdr.chr_size == 0) {
		printf("CHR ROM: CHR RAM instead of ROM\n");
	}
	else {
		printf("CHR size: %dKB\n", hdr.chr_size * CHR_ROM_UNIT);
	}
	printf("Mirroring type: %s\n", hdr.mirroring_type == MIRRORING_HORIZONTAL ? "Horizontal" : "Vertical");
	printf("Persistent memory on cartridge: %s\n", hdr.is_persistent_memory_present == 1 ? "True" : "False");
	printf("Trainer present: %s\n", hdr.is_trainer_present ? "True" : "False");
	printf("Ignore mirror control: %s\n", hdr.ignore_mirror_control ? "True" : "False");
	printf("VS unisystem: %s\n", hdr.vs_unisystem ? "True" : "False");
	printf("PlayChoice: %s\n", hdr.playchoice ? "True" : "False");
	printf("TV system: %s\n", hdr.tv_system == TVSYS_NTSC ? "NTSC" : "PAL");
	printf("Mapper number: %d\n", hdr.mapper_num_high << 4 + hdr.mapper_num_low);
}

void dump_code(FILE *nes_image, int num_of_instructions) {
	char* code;

	code = disass_get_code(nes_image, num_of_instructions);
	
	if (!code) {
		puts("Error: couldn't disassemble file.");
	}

	puts("");
	puts(code);
	disass_cleanup_code(code);
}

int main(int argc, char *argv[]) {
	FILE *nes_image;
	struct optparse options;
	char *fpath;
	int num_of_instructions = 10, option;

    if (argc < 2) {
        usage();
        exit(0);
    }

	optparse_init(&options, argv);
	while ((option = optparse(&options, "n:")) != -1) {
		switch (option) {
			case 'n':
				num_of_instructions = atoi(options.optarg);
				break;
			default:
				break;
		}
	}

	fpath = optparse_arg(&options);

	if (!fpath) {
		usage();
		exit(0);
	}

	nes_image = fopen(fpath, "rb");

    if (!nes_image) {
		perror("Couldn't open file");
		return EXIT_FAILURE;
    }

    dump_header(nes_image);
	dump_code(nes_image, num_of_instructions);

	fclose(nes_image);

    return EXIT_SUCCESS;
}