#include "disass.h"

/* For example, "ORA ($abcd),Y\n" */
#define MAX_MNEMONIC_SIZE 15

static char *opcode_to_mnemonic[] = {
	[0x0] = "BRK #$%X\n",
	[0x1] = "ORA ($%X,X)\n",
	[0x2] = "HLT\n",
	[0x3] = "ASO ($%X,X)\n",
	[0x4] = "SKB $%X\n",
	[0x5] = "ORA $%X\n",
	[0x6] = "ASL $%X\n",
	[0x7] = "ASO $%X\n",
	[0x8] = "PHP\n",
	[0x9] = "ORA #$%X\n",
	[0xa] = "ASLA\n",
	[0xb] = "ANC #$%X\n",
	[0xc] = "SKW $%X\n",
	[0xd] = "ORA $%X\n",
	[0xe] = "ASL $%X\n",
	[0xf] = "ASO $%X\n",
	[0x10] = "BPL $%X\n",
	[0x11] = "ORA ($%X),Y\n",
	[0x12] = "HLT\n",
	[0x13] = "ASO ($%X),Y\n",
	[0x14] = "SKB $%X,X\n",
	[0x15] = "ORA $%X,X\n",
	[0x16] = "ASL $%X,X\n",
	[0x17] = "ASO $%X,X\n",
	[0x18] = "CLC\n",
	[0x19] = "ORA $%X,Y\n",
	[0x1a] = "NOP\n",
	[0x1b] = "ASO $%X,Y\n",
	[0x1c] = "SKW $%X,X\n",
	[0x1d] = "ORA $%X,X\n",
	[0x1e] = "ASL $%X,X\n",
	[0x1f] = "ASO $%X,X\n",
	[0x20] = "JSR $%X\n",
	[0x21] = "AND ($%X,X)\n",
	[0x22] = "HLT\n",
	[0x23] = "RLA ($%X,X)\n",
	[0x24] = "BIT $%X\n",
	[0x25] = "AND $%X\n",
	[0x26] = "ROL $%X\n",
	[0x27] = "RLA $%X\n",
	[0x28] = "PLP\n",
	[0x29] = "AND #$%X\n",
	[0x2a] = "ROLA\n",
	[0x2b] = "ANC #$%X\n",
	[0x2c] = "BIT $%X\n",
	[0x2d] = "AND $%X\n",
	[0x2e] = "ROL $%X\n",
	[0x2f] = "RLA $%X\n",
	[0x30] = "BMI $%X\n",
	[0x31] = "AND ($%X),Y\n",
	[0x32] = "HLT\n",
	[0x33] = "RLA ($%X),Y\n",
	[0x34] = "SKB $%X,X\n",
	[0x35] = "AND $%X,X\n",
	[0x36] = "ROL $%X,X\n",
	[0x37] = "RLA $%X,X\n",
	[0x38] = "SEC\n",
	[0x39] = "AND $%X,Y\n",
	[0x3a] = "NOP\n",
	[0x3b] = "RLA $%X,Y\n",
	[0x3c] = "SKW $%X,X\n",
	[0x3d] = "AND $%X,X\n",
	[0x3e] = "ROL $%X,X\n",
	[0x3f] = "RLA $%X,X\n",
	[0x40] = "RTI\n",
	[0x41] = "EOR ($%X,X)\n",
	[0x42] = "HLT\n",
	[0x43] = "LSE ($%X,X)\n",
	[0x44] = "SKB $%X\n",
	[0x45] = "EOR $%X\n",
	[0x46] = "LSR $%X\n",
	[0x47] = "LSE $%X\n",
	[0x48] = "PHA\n",
	[0x49] = "EOR #$%X\n",
	[0x4a] = "LSRA\n",
	[0x4b] = "ALR #$%X\n",
	[0x4c] = "JMP $%X\n",
	[0x4d] = "EOR $%X\n",
	[0x4e] = "LSR $%X\n",
	[0x4f] = "LSE $%X\n",
	[0x50] = "BVC $%X\n",
	[0x51] = "EOR ($%X),Y\n",
	[0x52] = "HLT\n",
	[0x53] = "LSE ($%X),Y\n",
	[0x54] = "SKB $%X,X\n",
	[0x55] = "EOR $%X,X\n",
	[0x56] = "LSR $%X,X\n",
	[0x57] = "LSE $%X,X\n",
	[0x58] = "CLI\n",
	[0x59] = "EOR $%X,Y\n",
	[0x5a] = "NOP\n",
	[0x5b] = "LSE $%X,Y\n",
	[0x5c] = "SKW $%X,X\n",
	[0x5d] = "EOR $%X,X\n",
	[0x5e] = "LSR $%X,X\n",
	[0x5f] = "LSE $%X,X\n",
	[0x60] = "RTS\n",
	[0x61] = "ADC ($%X,X)\n",
	[0x62] = "HLT\n",
	[0x63] = "RRA ($%X,X)\n",
	[0x64] = "SKB $%X\n",
	[0x65] = "ADC $%X\n",
	[0x66] = "ROR $%X\n",
	[0x67] = "RRA $%X\n",
	[0x68] = "PLA\n",
	[0x69] = "ADC #$%X\n",
	[0x6a] = "RORA\n",
	[0x6b] = "ARR #$%X\n",
	[0x6c] = "JMP ($%X)\n",
	[0x6d] = "ADC $%X\n",
	[0x6e] = "ROR $%X\n",
	[0x6f] = "RRA $%X\n",
	[0x70] = "BVS $%X\n",
	[0x71] = "ADC ($%X),Y\n",
	[0x72] = "HLT\n",
	[0x73] = "RRA ($%X),Y\n",
	[0x74] = "SKB $%X,X\n",
	[0x75] = "ADC $%X,X\n",
	[0x76] = "ROR $%X,X\n",
	[0x77] = "RRA $%X,X\n",
	[0x78] = "SEI\n",
	[0x79] = "ADC $%X,Y\n",
	[0x7a] = "NOP\n",
	[0x7b] = "RRA $%X,Y\n",
	[0x7c] = "SKW $%X,X\n",
	[0x7d] = "ADC $%X,X\n",
	[0x7e] = "ROR $%X,X\n",
	[0x7f] = "RRA $%X,X\n",
	[0x80] = "SKB #$%X\n",
	[0x81] = "STA ($%X,X)\n",
	[0x82] = "SKB #$%X\n",
	[0x83] = "SAX ($%X,X)\n",
	[0x84] = "STY $%X\n",
	[0x85] = "STA $%X\n",
	[0x86] = "STX $%X\n",
	[0x87] = "SAX $%X\n",
	[0x88] = "DEY\n",
	[0x89] = "SKB #$%X\n",
	[0x8a] = "TXA\n",
	[0x8b] = "ANE #$%X\n",
	[0x8c] = "STY $%X\n",
	[0x8d] = "STA $%X\n",
	[0x8e] = "STX $%X\n",
	[0x8f] = "SAX $%X\n",
	[0x90] = "BCC $%X\n",
	[0x91] = "STA ($%X),Y\n",
	[0x92] = "HLT\n",
	[0x93] = "SHA ($%X),Y\n",
	[0x94] = "STY $%X,X\n",
	[0x95] = "STA $%X,X\n",
	[0x96] = "STX $%X,Y\n",
	[0x97] = "SAX $%X,Y\n",
	[0x98] = "TYA\n",
	[0x99] = "STA $%X,Y\n",
	[0x9a] = "TXS\n",
	[0x9b] = "SHS $%X,Y\n",
	[0x9c] = "SHY $%X,X\n",
	[0x9d] = "STA $%X,X\n",
	[0x9e] = "SHX $%X,Y\n",
	[0x9f] = "SHA $%X,Y\n",
	[0xa0] = "LDY #$%X\n",
	[0xa1] = "LDA ($%X,X)\n",
	[0xa2] = "LDX #$%X\n",
	[0xa3] = "LAX ($%X,X)\n",
	[0xa4] = "LDY $%X\n",
	[0xa5] = "LDA $%X\n",
	[0xa6] = "LDX $%X\n",
	[0xa7] = "LAX $%X\n",
	[0xa8] = "TAY\n",
	[0xa9] = "LDA #$%X\n",
	[0xaa] = "TAX\n",
	[0xab] = "ANX #$%X\n",
	[0xac] = "LDY $%X\n",
	[0xad] = "LDA $%X\n",
	[0xae] = "LDX $%X\n",
	[0xaf] = "LAX $%X\n",
	[0xb0] = "BCS $%X\n",
	[0xb1] = "LDA ($%X),Y\n",
	[0xb2] = "HLT\n",
	[0xb3] = "LAX ($%X),Y\n",
	[0xb4] = "LDY $%X,X\n",
	[0xb5] = "LDA $%X,X\n",
	[0xb6] = "LDX $%X,Y\n",
	[0xb7] = "LAX $%X,Y\n",
	[0xb8] = "CLV\n",
	[0xb9] = "LDA $%X,Y\n",
	[0xba] = "TSX\n",
	[0xbb] = "LAS $%X,Y\n",
	[0xbc] = "LDY $%X,X\n",
	[0xbd] = "LDA $%X,X\n",
	[0xbe] = "LDX $%X,Y\n",
	[0xbf] = "LAX $%X,Y\n",
	[0xc0] = "CPY #$%X\n",
	[0xc1] = "CMP ($%X,X)\n",
	[0xc2] = "SKB #$%X\n",
	[0xc3] = "DCM ($%X,X)\n",
	[0xc4] = "CPY $%X\n",
	[0xc5] = "CMP $%X\n",
	[0xc6] = "DEC $%X\n",
	[0xc7] = "DCM $%X\n",
	[0xc8] = "INY\n",
	[0xc9] = "CMP #$%X\n",
	[0xca] = "DEX\n",
	[0xcb] = "SBX #$%X\n",
	[0xcc] = "CPY $%X\n",
	[0xcd] = "CMP $%X\n",
	[0xce] = "DEC $%X\n",
	[0xcf] = "DCM $%X\n",
	[0xd0] = "BNE $%X\n",
	[0xd1] = "CMP ($%X),Y\n",
	[0xd2] = "HLT\n",
	[0xd3] = "DCM ($%X),Y\n",
	[0xd4] = "SKB $%X,X\n",
	[0xd5] = "CMP $%X,X\n",
	[0xd6] = "DEC $%X,X\n",
	[0xd7] = "DCM $%X,X\n",
	[0xd8] = "CLD\n",
	[0xd9] = "CMP $%X,Y\n",
	[0xda] = "NOP\n",
	[0xdb] = "DCM $%X,Y\n",
	[0xdc] = "SKW $%X,X\n",
	[0xdd] = "CMP $%X,X\n",
	[0xde] = "DEC $%X,X\n",
	[0xdf] = "DCM $%X,X\n",
	[0xe0] = "CPX #$%X\n",
	[0xe1] = "SBC ($%X,X)\n",
	[0xe2] = "SKB #$%X\n",
	[0xe3] = "INS ($%X,X)\n",
	[0xe4] = "CPX $%X\n",
	[0xe5] = "SBC $%X\n",
	[0xe6] = "INC $%X\n",
	[0xe7] = "INS $%X\n",
	[0xe8] = "INX\n",
	[0xe9] = "SBC #$%X\n",
	[0xea] = "NOP\n",
	[0xeb] = "SBC #$%X\n",
	[0xec] = "CPX $%X\n",
	[0xed] = "SBC $%X\n",
	[0xee] = "INC $%X\n",
	[0xef] = "INS $%X\n",
	[0xf0] = "BEQ $%X\n",
	[0xf1] = "SBC ($%X),Y\n",
	[0xf2] = "HLT\n",
	[0xf3] = "INS ($%X),Y\n",
	[0xf4] = "SKB $%X,X\n",
	[0xf5] = "SBC $%X,X\n",
	[0xf6] = "INC $%X,X\n",
	[0xf7] = "INS $%X,X\n",
	[0xf8] = "SED\n",
	[0xf9] = "SBC $%X,Y\n",
	[0xfa] = "NOP\n",
	[0xfb] = "INS $%X,Y\n",
	[0xfc] = "SKW $%X,X\n",
	[0xfd] = "SBC $%X,X\n",
	[0xfe] = "INC $%X,X\n",
	[0xff] = "INS $%X,X\n"
};

static bool is_header_valid(const struct ines2_header *hdr) {
	return memcmp(hdr->magic, INES_HEADER_MAGIC, INES_HEADER_MAGIC_SIZE) == 0 &&
		hdr->prg_size != 0;
}

int disass_get_header(FILE *nes_image, struct ines2_header *hdr) {
	size_t read_bytes;

	if (fseek(nes_image, 0, SEEK_SET) != 0)
		return -1;

	read_bytes = fread(hdr, 1, INES_HEADER_SIZE, nes_image);

	if (read_bytes != INES_HEADER_SIZE)
		return -1;

	if (!is_header_valid(hdr))
		return -1;

	return 0;
}

static size_t get_prg_offset(const struct ines2_header *hdr) {
	return hdr->is_trainer_present ? sizeof(*hdr) + TRAINER_SIZE : sizeof(*hdr);
}

static void fill_mnemonic(char *mnemonic, const struct instruction_description *desc, bool is_valid) {
	if (!is_valid) {
		strcpy(mnemonic, "<unknown>\n");
	}
	else {
		if (desc->instruction_size > 1) {
			/* Size of mnemonic + newline. */
			sprintf(mnemonic, opcode_to_mnemonic[desc->opcode], desc->operand);
		}
		else {
			strcpy(mnemonic, opcode_to_mnemonic[desc->opcode]);
		}
	}
}

char *disass_get_code(FILE *nes_image, int num_of_instructions) {
	/* Max mnemonic size + null termination. */
	char current_mnemonic[MAX_MNEMONIC_SIZE + 1];
	char *output, *new_output;
	unsigned char *image_data;
	size_t prg_offset, output_size, image_data_size, current_mnemonic_size;
	size_t read_bytes = 0, current_output_size = 0;
	const size_t average_mnemonic_size = 10;
	struct ines2_header hdr;
	struct instruction_description current_desc;
	bool error = false;

	if (disass_get_header(nes_image, &hdr))
		return NULL;

	prg_offset = get_prg_offset(&hdr);
	image_data_size = num_of_instructions == -1 ? hdr.prg_size * PRG_ROM_UNIT :
		num_of_instructions * MAX_INSN_SIZE; /* Extra byte to prevent instruction cut-off. */

	if (num_of_instructions != -1)
		output_size = average_mnemonic_size * num_of_instructions;
	else
		output_size = average_mnemonic_size * 100;

	output = malloc(output_size);

	if (output == NULL)
		return NULL;

	/* Extra 2 bytes in case the last instruction is cut. */
	image_data = malloc(image_data_size + 2);

	if (image_data == NULL)
		return NULL;

	*output = '\0';

	if (fseek(nes_image, prg_offset, SEEK_SET)) {
		error = true;
		goto out;
	}

	if (fread(image_data, 1, image_data_size, nes_image) != image_data_size) {
		error = true;
		goto out;
	}

	while (read_bytes < image_data_size) {
		if (parser_get_instruction_description(image_data + read_bytes, image_data_size - read_bytes, &current_desc) != 0) {
			/* Check if parsing failed because we cut off
			 * the last instruction when the data was read from the file. */
			if (read_bytes + parser_get_instruction_size(image_data + read_bytes) > image_data_size) {
				/* We make sure that image_data + read_bytes points to 3 bytes of an instruction
				 * (the max instruction size, worst case scenario).
				 */
				fread(image_data + read_bytes + 1, 1, 2, nes_image);
				if (parser_get_instruction_description(image_data + read_bytes, MAX_INSN_SIZE, &current_desc)) {
					fill_mnemonic(current_mnemonic, &current_desc, false);
					/* Iterate by one until we find a valid instruction. */
					current_desc.instruction_size = 1;
				}
				else {
					fill_mnemonic(current_mnemonic, &current_desc, true);
				}
			}
			else {
				fill_mnemonic(current_mnemonic, &current_desc, false);
				/* Iterate by one until we find a valid instruction. */
				current_desc.instruction_size = 1;
			}
		}
		else {
			fill_mnemonic(current_mnemonic, &current_desc, true);
		}

		current_mnemonic_size = strlen(current_mnemonic);

		/* Account for null-termination when checking possible overflow. */
		if (current_output_size + current_mnemonic_size + 1 > output_size) {
			/* Double the output buffer size and update the iterator. */
			output_size *= 2;
			new_output = realloc(output, output_size);

			if (new_output == NULL) {
				error = true;
				goto out;
			}

			output = new_output;
		}

		strcpy(output + current_output_size, current_mnemonic);
		current_output_size += current_mnemonic_size;

		read_bytes += current_desc.instruction_size;
	}

out:
	if (error)
		free(output);
	free(image_data);
	return error ? NULL : output;
}

void disass_cleanup_code(char* buff) {
	free(buff);
}