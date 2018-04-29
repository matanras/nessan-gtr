#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <nessan-gtr/types.h>

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
#define REG_TRI_LO            0x400A
#define REG_TRI_HI            0x400B
#define REG_NOISE_VOL         0x400C
#define REG_NOISE_LO          0x400E
#define REG_NOISE_HI          0x400F
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
#define ADDR_RESET_VEC 0xFFFC

/* Status register constants. */
#define SR_IRQ_ENABLED  0
#define SR_IRQ_DISABLED 1
#define SR_CARRY_OFF    0
#define SR_CARRY_ON     1
#define SR_NEGATIVE_OFF 0
#define SR_NEGATIVE_ON  1

/* Memory regions. */
#define MEMREGION_SYSTEM_BEGIN     0x0
#define MEMREGION_SYSTEM_END    0x1FFF
#define MEMREGION_PPUIO_BEGIN   0x2000
#define MEMREGION_PPUIO_END     0x3FFF
#define MEMREGION_PRG_ROM_BEGIN 0x8000
#define MEMREGION_PRG_ROM_END   0xFFFF

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
	uint8_t y; /*Indexer 2. */
};

/* The state of the CPU. */
struct cpu {
	bool is_powered_on;
	struct cpu_registers regs;
};

/* Instruction logic. */
typedef void(*instruction_logic)(word_t operand);