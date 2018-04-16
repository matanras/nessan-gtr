/*
 * The parser module.
 * Parses binary representation of 2A03 instructions into descriptor objects.
 */

#pragma once

#include <stdint.h>

/*
 * Metadata about 2A03 instructions.
 */
struct instruction_description {
    uint8_t opcode;
    uint8_t instruction_size;
    uint16_t operand;
};

/**
 * Parse the given instruction data in buff into an instruction_description structure.
 * @param buff A buffer containing binary instruction data.
 * @param buff_size The buffer's size.
 * @param desc An instruction_description struct to be filled.
 * @return 0 If parsing was successful, -1 otherwise.
 */
int get_instruction_description(
        unsigned char *buff,
        size_t buff_size,
        struct instruction_description *desc);