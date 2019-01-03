#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <log.h>

/* CPU memory mapped registers. */
#define REG_SQ1_VOL           0x4000
#define REG_SQ1_SWEEP         0x4001
#define REG_SQ1_LO            0x4002
#define REG_SQ1_HI            0x4003
#define REG_SQ2_VOL           0x4004
#define REG_SQ2_SWEEP         0x4005
#define REG_SQ2_LO            0x4006
#define REG_SQ2_HI            0x4007
#define REG_TRI_LINEAR        0x4008
#define REG_TRI_LO            0x400a
#define REG_TRI_HI            0x400b
#define REG_NOISE_VOL         0x400c
#define REG_NOISE_LO          0x400e
#define REG_NOISE_HI          0x400f
#define REG_DMC_FREQ          0x4010
#define REG_DMC_RAW           0x4011
#define REG_DMC_START         0x4012
#define REG_DMC_LEN           0x4013
#define REG_OAMDMA            0x4014
#define REG_SND_CHN           0x4015
#define REG_JOY1              0x4016
#define REG_JOY2              0x4017
#define REG_FRAME_COUNTER_CTL 0x4017

/* Other CPU addresses. */
#define ADDR_RESET_VEC 0xfffc
#define ADDR_BRK_VEC   0xfffe

/* Status register constants. */
#define SR_BRK_ON       1
#define SR_BRK_OFF      1
#define SR_IRQ_ENABLED  0
#define SR_IRQ_DISABLED 1
#define SR_CARRY_OFF    0
#define SR_CARRY_ON     1
#define SR_NEGATIVE_OFF 0
#define SR_NEGATIVE_ON  1
#define SR_ZERO_OFF     0
#define SR_ZERO_ON      1
#define SR_OVERFLOW_OFF 0
#define SR_OVERFLOW_ON  1

/* Memory regions. */
#define MEMREGION_SYSTEM_BEGIN     0x0
#define MEMREGION_SYSTEM_END    0x1fff
#define MEMREGION_PPUIO_BEGIN   0x2000
#define MEMREGION_PPUIO_END     0x3fff
#define MEMREGION_PRG_ROM_BEGIN 0x8000
#define MEMREGION_PRG_ROM_END   0xffff

struct cpu_registers {
	/* Status register. */
	union {
		struct {
			uint8_t carry : 1;
			uint8_t zero : 1;
			uint8_t irq : 1;
			uint8_t decimal : 1;
			uint8_t brk : 1;
			uint8_t unused : 1;
			uint8_t overflow : 1;
			uint8_t negative : 1;
		} bits;
		uint8_t value;
	} p;
	uint16_t pc; /* Program counter. */
	uint8_t s; /* Stack pointer. */
	uint8_t a; /* Accumulator. */
	uint8_t x; /* Indexer 1. */
	uint8_t y; /* Indexer 2. */
};

/* The state of the CPU. */
struct cpu {
	bool is_powered_on;
	long long executed_instructions;
	struct cpu_registers regs;
	logger_t *logger;
};

enum addressing_mode {
	/* The operand is implied in the opcode. */
	IMPLIED,
	/* The byte following the opcode is the operand. */
	IMMEDIATE,
	/* The result is written to the accumulator. */
	ACCUMULATOR,
	/* The operand is in the given address. */
	ABSOLUTE,
	/* The operand is at the given address + X register */
	ABSOLUTE_X,
	/* The operand is at the given address + Y register */
	ABSOLUTE_Y,
	/* The operand is at the adress pointed to by the given address. */
	INDIRECT_ABSOLUTE,
	/* The operand is in the zero-page given address. */
	DP,
	/* Operand is at the given zero page address + X register;
	  If addr + X overflows ZERO_PAGE_LIMIT, it simply wraps around. */
	DP_X,
	/* Same as DP_X only with y register instead. */
	DP_Y,
	/* Operand is at the address pointed to by the sum of the given zero page address and the X register.
	   If addr + X overflows ZERO_PAGE_LIMIT, it simply wraps around. */
	DP_INDIRECT_X,
	/* Operand is at the address pointed to by the sum of the address at the zero page address and the y register.
	* If addr + x overflows ZERO_PAGE_LIMIT, it simply wraps around. */
	DP_INDIRECT_Y,
};

enum destination {
	/* The instruction writes result to register. */
	CPU_REGISTER,
	/* The instruction affects PC register. */
	CPU_REGISTER_PC,
	/* The instruction writes result to memory. */
	MEMORY,
};

/* Instruction implementation logic. */
typedef uint8_t(*instruction_impl)(uint8_t operand);

/* Instruction implementation logic for instructions with 16-bit wide operands. */
typedef void(*instruction_impl_ext)(uint16_t operand);

typedef uint16_t(*address_translator)(uint16_t operand);

struct instruction_handler_data {
	enum destination instruction_destination;
	enum addressing_mode addressing_mode;
	instruction_impl instruction_impl;
};

struct instruction_handler_data_ext {
	enum destination instruction_destination;
	enum addressing_mode addressing_mode;
	instruction_impl_ext instruction_impl;
};