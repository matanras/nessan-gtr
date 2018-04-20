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

/**
 * Get the iNes header from the file.
 * @param nes_image The binary nes file.
 * @param hdr The header to be filled with info.
 * @return 0 if the header was valid, -1 otherwise.
 */
int disass_get_header(FILE *nes_image, struct ines2_header *hdr);

/**
 * Get a buffer filled with disassembled code.
 * Pass buffer to @disass_cleanup_code when done.
 * @param nes_image The binary nes file.
 * @param num_of_instructions The number of instructions to disassemble. -1 for all instructions.
 * @return A buffer with disassembled code, or NULL if an error occured.
 */
char *disass_get_code(FILE *nes_image, int num_of_instructions);

/**
 * Cleanup a buffer filled with disassembled code.
 * Must be a pointer that was returned from @disass_get_code.
 * @param buff The buffer to clean.
 */
void disass_cleanup_code(char *buff);