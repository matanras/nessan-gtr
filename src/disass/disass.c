#include "disass.h"

static struct disassembler_settings settings;

static char *opcode_to_mnemonic[] = {
	[0x0] = "BRK #$%d",
	[0x1] = "ORA ($%d,X)",
	[0x2] = "HLT",
	[0x3] = "ASO ($%d,X)",
	[0x4] = "SKB $%d",
	[0x5] = "ORA $%d",
	[0x6] = "ASL $%d",
	[0x7] = "ASO $%d",
	[0x8] = "PHP",
	[0x9] = "ORA #$%d",
	[0xa] = "ASLA",
	[0xb] = "ANC #$%d",
	[0xc] = "SKW $%d",
	[0xd] = "ORA $%d",
	[0xe] = "ASL $%d",
	[0xf] = "ASO $%d",
	[0x10] = "BPL %d",
	[0x11] = "ORA ($%d),Y",
	[0x12] = "HLT",
	[0x13] = "ASO ($%d),Y",
	[0x14] = "SKB $%d,X",
	[0x15] = "ORA $%d,X",
	[0x16] = "ASL $%d,X",
	[0x17] = "ASO $%d,X",
	[0x18] = "CLC",
	[0x19] = "ORA $%d,Y",
	[0x1a] = "NOP",
	[0x1b] = "ASO $%d,Y",
	[0x1c] = "SKW $%d,X",
	[0x1d] = "ORA $%d,X",
	[0x1e] = "ASL $%d,X",
	[0x1f] = "ASO $%d,X",
	[0x20] = "JSR $%d",
	[0x21] = "AND ($%d,X)",
	[0x22] = "HLT",
	[0x23] = "RLA ($%d,X)",
	[0x24] = "BIT $%d",
	[0x25] = "AND $%d",
	[0x26] = "ROL $%d",
	[0x27] = "RLA $%d",
	[0x28] = "PLP",
	[0x29] = "AND #$%d",
	[0x2a] = "ROLA",
	[0x2b] = "ANC #$%d",
	[0x2c] = "BIT $%d",
	[0x2d] = "AND $%d",
	[0x2e] = "ROL $%d",
	[0x2f] = "RLA $%d",
	[0x30] = "BMI %d",
	[0x31] = "AND ($%d),Y",
	[0x32] = "HLT",
	[0x33] = "RLA ($%d),Y",
	[0x34] = "SKB $%d,X",
	[0x35] = "AND $%d,X",
	[0x36] = "ROL $%d,X",
	[0x37] = "RLA $%d,X",
	[0x38] = "SEC",
	[0x39] = "AND $%d,Y",
	[0x3a] = "NOP",
	[0x3b] = "RLA $%d,Y",
	[0x3c] = "SKW $%d,X",
	[0x3d] = "AND $%d,X",
	[0x3e] = "ROL $%d,X",
	[0x3f] = "RLA $%d,X",
	[0x40] = "RTI",
	[0x41] = "EOR ($%d,X)",
	[0x42] = "HLT",
	[0x43] = "LSE ($%d,X)",
	[0x44] = "SKB $%d",
	[0x45] = "EOR $%d",
	[0x46] = "LSR $%d",
	[0x47] = "LSE $%d",
	[0x48] = "PHA",
	[0x49] = "EOR #$%d",
	[0x4a] = "LSRA",
	[0x4b] = "ALR #$%d",
	[0x4c] = "JMP $%d",
	[0x4d] = "EOR $%d",
	[0x4e] = "LSR $%d",
	[0x4f] = "LSE $%d",
	[0x50] = "BVC %d",
	[0x51] = "EOR ($%d),Y",
	[0x52] = "HLT",
	[0x53] = "LSE ($%d),Y",
	[0x54] = "SKB $%d,X",
	[0x55] = "EOR $%d,X",
	[0x56] = "LSR $%d,X",
	[0x57] = "LSE $%d,X",
	[0x58] = "CLI",
	[0x59] = "EOR $%d,Y",
	[0x5a] = "NOP",
	[0x5b] = "LSE $%d,Y",
	[0x5c] = "SKW $%d,X",
	[0x5d] = "EOR $%d,X",
	[0x5e] = "LSR $%d,X",
	[0x5f] = "LSE $%d,X",
	[0x60] = "RTS",
	[0x61] = "ADC ($%d,X)",
	[0x62] = "HLT",
	[0x63] = "RRA ($%d,X)",
	[0x64] = "SKB $%d",
	[0x65] = "ADC $%d",
	[0x66] = "ROR $%d",
	[0x67] = "RRA $%d",
	[0x68] = "PLA",
	[0x69] = "ADC #$%d",
	[0x6a] = "RORA",
	[0x6b] = "ARR #$%d",
	[0x6c] = "JMP ($%d)",
	[0x6d] = "ADC $%d",
	[0x6e] = "ROR $%d",
	[0x6f] = "RRA $%d",
	[0x70] = "BVS %d",
	[0x71] = "ADC ($%d),Y",
	[0x72] = "HLT",
	[0x73] = "RRA ($%d),Y",
	[0x74] = "SKB $%d,X",
	[0x75] = "ADC $%d,X",
	[0x76] = "ROR $%d,X",
	[0x77] = "RRA $%d,X",
	[0x78] = "SEI",
	[0x79] = "ADC $%d,Y",
	[0x7a] = "NOP",
	[0x7b] = "RRA $%d,Y",
	[0x7c] = "SKW $%d,X",
	[0x7d] = "ADC $%d,X",
	[0x7e] = "ROR $%d,X",
	[0x7f] = "RRA $%d,X",
	[0x80] = "SKB #$%d",
	[0x81] = "STA ($%d,X)",
	[0x82] = "SKB #$%d",
	[0x83] = "SAX ($%d,X)",
	[0x84] = "STY $%d",
	[0x85] = "STA $%d",
	[0x86] = "STX $%d",
	[0x87] = "SAX $%d",
	[0x88] = "DEY",
	[0x89] = "SKB #$%d",
	[0x8a] = "TXA",
	[0x8b] = "ANE #$%d",
	[0x8c] = "STY $%d",
	[0x8d] = "STA $%d",
	[0x8e] = "STX $%d",
	[0x8f] = "SAX $%d",
	[0x90] = "BCC %d",
	[0x91] = "STA ($%d),Y",
	[0x92] = "HLT",
	[0x93] = "SHA ($%d),Y",
	[0x94] = "STY $%d,X",
	[0x95] = "STA $%d,X",
	[0x96] = "STX $%d,Y",
	[0x97] = "SAX $%d,Y",
	[0x98] = "TYA",
	[0x99] = "STA $%d,Y",
	[0x9a] = "TXS",
	[0x9b] = "SHS $%d,Y",
	[0x9c] = "SHY $%d,X",
	[0x9d] = "STA $%d,X",
	[0x9e] = "SHX $%d,Y",
	[0x9f] = "SHA $%d,Y",
	[0xa0] = "LDY #$%d",
	[0xa1] = "LDA ($%d,X)",
	[0xa2] = "LDX #$%d",
	[0xa3] = "LAX ($%d,X)",
	[0xa4] = "LDY $%d",
	[0xa5] = "LDA $%d",
	[0xa6] = "LDX $%d",
	[0xa7] = "LAX $%d",
	[0xa8] = "TAY",
	[0xa9] = "LDA #$%d",
	[0xaa] = "TAX",
	[0xab] = "ANX #$%d",
	[0xac] = "LDY $%d",
	[0xad] = "LDA $%d",
	[0xae] = "LDX $%d",
	[0xaf] = "LAX $%d",
	[0xb0] = "BCS %d",
	[0xb1] = "LDA ($%d),Y",
	[0xb2] = "HLT",
	[0xb3] = "LAX ($%d),Y",
	[0xb4] = "LDY $%d,X",
	[0xb5] = "LDA $%d,X",
	[0xb6] = "LDX $%d,Y",
	[0xb7] = "LAX $%d,Y",
	[0xb8] = "CLV",
	[0xb9] = "LDA $%d,Y",
	[0xba] = "TSX",
	[0xbb] = "LAS $%d,Y",
	[0xbc] = "LDY $%d,X",
	[0xbd] = "LDA $%d,X",
	[0xbe] = "LDX $%d,Y",
	[0xbf] = "LAX $%d,Y",
	[0xc0] = "CPY #$%d",
	[0xc1] = "CMP ($%d,X)",
	[0xc2] = "SKB #$%d",
	[0xc3] = "DCM ($%d,X)",
	[0xc4] = "CPY $%d",
	[0xc5] = "CMP $%d",
	[0xc6] = "DEC $%d",
	[0xc7] = "DCM $%d",
	[0xc8] = "INY",
	[0xc9] = "CMP #$%d",
	[0xca] = "DEX",
	[0xcb] = "SBX #$%d",
	[0xcc] = "CPY $%d",
	[0xcd] = "CMP $%d",
	[0xce] = "DEC $%d",
	[0xcf] = "DCM $%d",
	[0xd0] = "BNE %d",
	[0xd1] = "CMP ($%d),Y",
	[0xd2] = "HLT",
	[0xd3] = "DCM ($%d),Y",
	[0xd4] = "SKB $%d,X",
	[0xd5] = "CMP $%d,X",
	[0xd6] = "DEC $%d,X",
	[0xd7] = "DCM $%d,X",
	[0xd8] = "CLD",
	[0xd9] = "CMP $%d,Y",
	[0xda] = "NOP",
	[0xdb] = "DCM $%d,Y",
	[0xdc] = "SKW $%d,X",
	[0xdd] = "CMP $%d,X",
	[0xde] = "DEC $%d,X",
	[0xdf] = "DCM $%d,X",
	[0xe0] = "CPX #$%d",
	[0xe1] = "SBC ($%d,X)",
	[0xe2] = "SKB #$%d",
	[0xe3] = "INS ($%d,X)",
	[0xe4] = "CPX $%d",
	[0xe5] = "SBC $%d",
	[0xe6] = "INC $%d",
	[0xe7] = "INS $%d",
	[0xe8] = "INX",
	[0xe9] = "SBC #$%d",
	[0xea] = "NOP",
	[0xeb] = "SBC #$%d",
	[0xec] = "CPX $%d",
	[0xed] = "SBC $%d",
	[0xee] = "INC $%d",
	[0xef] = "INS $%d",
	[0xf0] = "BEQ %d",
	[0xf1] = "SBC ($%d),Y",
	[0xf2] = "HLT",
	[0xf3] = "INS ($%d),Y",
	[0xf4] = "SKB $%d,X",
	[0xf5] = "SBC $%d,X",
	[0xf6] = "INC $%d,X",
	[0xf7] = "INS $%d,X",
	[0xf8] = "SED",
	[0xf9] = "SBC $%d,Y",
	[0xfa] = "NOP",
	[0xfb] = "INS $%d,Y",
	[0xfc] = "SKW $%d,X",
	[0xfd] = "SBC $%d,X",
	[0xfe] = "INC $%d,X",
	[0xff] = "INS $%d,X"
};

static int validate_header(const struct ines2_header *hdr) {
	return memcmp(hdr->magic, INES_HEADER_MAGIC, INES_HEADER_MAGIC_SIZE) &&
		hdr->prg_size != 0;
}

int disassembler_init(char *nes_binary_fpath) {
	int ret;
	FILE *image_file;
	struct ines2_header hdr;

	image_file = fopen(nes_binary_fpath, "rb");

	if (!image_file)
		return -EBADF;

	if (fread(&hdr, 1, sizeof(hdr), image_file) < sizeof(hdr)) {
		ret = -EINVAL;
		goto err;
	}

	ret = validate_header(&hdr);

	if (ret) {
		ret = -EINVAL;
		goto err;
	}

	memcpy(&settings.hdr, &hdr, sizeof(settings.hdr));
	memcpy(settings.binary_fpath, nes_binary_fpath, strlen(nes_binary_fpath));

	return 0;

err:
	fclose(image_file);
	return ret;
}

void dump_header(void) {
	if (settings.hdr.is_nes2 == INES_HEADER_V2_EXTENSION) {
		puts("iNES header /w NES 2.0 extension:");
	}
	else {
		puts("iNes header:");
	}

	printf("PRG size: %dKB\n", settings.hdr.prg_size * PRG_ROM_UNIT);
	if (settings.hdr.chr_size == 0) {
		printf("CHR ROM: CHR RAM instead of ROM\n");
	}
	else {
		printf("CHR size: %dKB\n", settings.hdr.chr_size * CHR_ROM_UNIT);
	}
	printf("Mirroring type: %s\n", settings.hdr.mirroring_type == MIRRORING_HORIZONTAL ? "Horizontal" : "Vertical");
	printf("Persistent memory on cartridge: %s\n", settings.hdr.is_persistent_memory_present == 1 ? "True" : "False");
	printf("Trainer present: %s\n", settings.hdr.is_trainer_present ? "True" : "False");
	printf("Ignore mirror control: %s\n", settings.hdr.ignore_mirror_control ? "True" : "False");
	printf("VS unisystem: %s\n", settings.hdr.vs_unisystem ? "True" : "False");
	printf("PlayChoice: %s\n", settings.hdr.playchoice ? "True" : "False");
	printf("TV system: %s\n", settings.hdr.tv_system == TVSYS_NTSC ? "NTSC" : "PAL");
}

static void print_instruction(const struct instruction_description *desc) {
	/* Instruction has an operand. */
	if (desc->instruction_size > 1) {
		printf("\t");
		printf(opcode_to_mnemonic[desc->opcode], desc->operand);
		puts("");
	}
	else {
		printf("\t");
		puts(opcode_to_mnemonic[desc->opcode]);
	}
}

void dump_code(void) {
	int ret;
	long prg_rom_offset;
	FILE* binary_file;
	size_t buff_size, bytes_used = 0;;
	unsigned char *buff, *iterator;
	struct instruction_description current;

	buff_size = settings.hdr.prg_size * PRG_ROM_UNIT;
	buff = malloc(buff_size);
	iterator = buff;

	binary_file = fopen(settings.binary_fpath, "rb");
	if (!binary_file) {
		perror("Couldn't open file!");
		goto out;
	}

	if (settings.hdr.is_trainer_present)
		prg_rom_offset = INES_HEADER_SIZE + TRAINER_SIZE;
	else
		prg_rom_offset = INES_HEADER_SIZE;

	fseek(binary_file, prg_rom_offset, SEEK_SET);
	fread(buff, 1, buff_size, binary_file);
	ret = get_instruction_description(buff, buff_size, &current);
	if (ret)
		goto out;
	print_instruction(&current);
	bytes_used += current.instruction_size;

	while (bytes_used < buff_size) {
		iterator += current.instruction_size;
		ret = get_instruction_description(iterator, buff_size - bytes_used, &current);
		if (ret)
			goto out;
		print_instruction(&current);
		bytes_used += current.instruction_size;
	}

out:
	free(buff);
	fclose(binary_file);
}