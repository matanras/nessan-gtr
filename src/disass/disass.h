/*
 * 2A03 CPU disassembler utility.
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <nessan-gtr/parser.h>
#include <nessan-gtr/ines.h>

struct disassembler_settings {
	struct ines2_header hdr;
	char binary_fpath[FILENAME_MAX];
};

/**
 * Initialize the disassembler.
 * @param nes_binary_fpath The path of the binary image to disassemble.
 * @return True if initialization was successful, false otherwise.
 */
int disassembler_init(char *nes_binary_fpath);

/**
 * Print the contents of the iNES header.
 */
void dump_header(void);

/**
 * Print the instructions in the PRG ROM.
 */
void dump_code(void);