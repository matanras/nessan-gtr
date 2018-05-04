#include <nessan-gtr/parser.h>
#include <stdint.h>
#include "cpu.h"
#include "cpu_internal.h"
#include "mmu.h"

#define ZERO_PAGE_LIMIT 0x100

static struct cpu cpu;

static void stack_push_word(uint16_t word) {
	mem_write(cpu.regs.s, word & 0xff);
	mem_write(cpu.regs.s + 1, word & 0xff00);
	cpu.regs.s -= 2;
}

static void stack_push_byte(uint8_t byte) {
	mem_write(cpu.regs.s, byte);
	--cpu.regs.s;
}

/* Instruction handlers. */

static inline uint16_t get_address_abs_and_dp(uint16_t addr) {
	return addr;
}

/*
* Operand is at the given address + x register;
*/
static inline uint16_t get_address_absx(uint16_t addr) {
	return addr + cpu.regs.x;
}

/*
* Operand is at the given address + y register;
*/
static inline uint16_t get_address_absy(uint16_t addr) {
	return addr + cpu.regs.y;
}

/*
* Operand is at the adress pointed to by addr.
*/
static inline uint16_t get_address_indirect_absolute(uint16_t addr) {
	return mem_read(addr);
}

/*
* Operand is at the given zero page address + x register;
* If addr + x overflows ZERO_PAGE_LIMIT, it simply wraps around.
*/
static inline uint16_t get_address_dpx(uint16_t addr) {
	return (addr + cpu.regs.x) % ZERO_PAGE_LIMIT;
}

/*
* Operand is at the given zero page  address + y register;
* If addr + y overflows ZERO_PAGE_LIMIT, it simply wraps around.
*/
static inline uint16_t get_address_dpy(uint16_t addr) {
	return (addr + cpu.regs.y) % ZERO_PAGE_LIMIT;
}

/*
* Operand is at the address pointed to by the sum of the given zero page address and the x register.
* If addr + x overflows ZERO_PAGE_LIMIT, it simply wraps around.
*/
static inline uint16_t get_address_dpindx(uint16_t addr) {
	return mem_read((addr + cpu.regs.x) % ZERO_PAGE_LIMIT);
}

/*
* Operand is at the address pointed to by the sum of the address at the zero page address and the y register.
* If addr + x overflows ZERO_PAGE_LIMIT, it simply wraps around.
*/
static inline uint16_t get_address_dpindy(uint16_t addr) {
	return mem_read(addr) + cpu.regs.y;
}

static uint8_t ASL(uint8_t operand) {
	/* Insert highest bit into carry flag. */
	cpu.regs.p.bits.carry = operand & 0x80;
	operand <<= 1;
	/* Check if negative after operation. */
	cpu.regs.p.bits.negative = operand & 0x80;
	cpu.regs.p.bits.zero = !operand;

	return operand;
}

static uint8_t ORA(uint8_t operand) {
	cpu.regs.a |= operand;
	cpu.regs.p.bits.negative = operand & 0x80; /* Test highest bit. */
	cpu.regs.p.bits.zero = !cpu.regs.a;

	return 0;
}

static uint8_t BRK(uint8_t operand) {
	cpu.regs.p.bits.brk = SR_BRK_ON;
	cpu.regs.p.bits.irq = SR_IRQ_DISABLED;
	stack_push_word(cpu.regs.pc);
	stack_push_byte(cpu.regs.p.value);
	cpu.regs.pc = mem_read(ADDR_BRK_VEC);

	return 0;
}

static uint8_t SEI(uint8_t operand) {
	cpu.regs.p.bits.irq = SR_IRQ_DISABLED;

	return 0;
}

static uint8_t STA(uint8_t operand) {
	return cpu.regs.a;
}

static uint8_t LDX(uint8_t operand) {
	cpu.regs.x += operand;

	return 0;
}

static uint8_t LDA(uint8_t operand) {
	cpu.regs.a = operand;
	
	return 0;
}

static uint8_t CLD(uint8_t operand) {
	return 0;
}

static struct instruction_handler_data opcode_to_handler_data[] = {
	[0x00] = {
		.instruction_impl = BRK,
		.addressing_mode = IMPLIED,
		.instruction_destination = NONE,
		.instruction_size = 2
	},
	[0x01] = {
		.instruction_impl = ORA,
		.addressing_mode = DP_INDIRECT_X,
		.instruction_destination = CPU_REGISTER,
		.instruction_size = 2
	},
	[0x05] = {
		.instruction_impl = ORA,
		.addressing_mode = DP,
		.instruction_destination = CPU_REGISTER,
		.instruction_size = 2
	},
	[0x09] = {
		.instruction_impl = ORA,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER,
		.instruction_size = 2
	},
	[0x0d] = {
		.instruction_impl = ORA,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = CPU_REGISTER,
		.instruction_size = 3
	},
	[0x11] = {
		.instruction_impl = ORA,
		.addressing_mode = DP_INDIRECT_Y,
		.instruction_destination = CPU_REGISTER,
		.instruction_size = 2
	},
	[0x15] = {
		.instruction_impl = ORA,
		.addressing_mode = DP_X,
		.instruction_destination = CPU_REGISTER,
		.instruction_size = 2
	},
	[0x19] = {
		.instruction_impl = ORA,
		.addressing_mode = ABSOLUTE_Y,
		.instruction_destination = CPU_REGISTER,
		.instruction_size = 3
	},
	[0x1d] = {
		.instruction_impl = ORA,
		.addressing_mode = ABSOLUTE_X,
		.instruction_destination = CPU_REGISTER,
		.instruction_size = 3
	},
	[0x78] = {
		.instruction_impl = SEI,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
		.instruction_size = 1
	},
	[0x8d] = {
		.instruction_impl = STA,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = MEMORY,
		.instruction_size = 3
	},
	[0xa2] = {
	    .instruction_impl = LDX,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER,
		.instruction_size = 2
	},
	[0xa9] = {
		.instruction_impl = LDA,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER,
		.instruction_size = 2
	},
	[0xd8] = {
		.instruction_impl = CLD,
		.addressing_mode = IMPLIED,
		.instruction_destination = NONE,
		.instruction_size = 1
	}
};

static address_translator address_translators[] = {
	[ABSOLUTE] = get_address_abs_and_dp,
	[ABSOLUTE_X] = get_address_absx,
	[ABSOLUTE_Y] = get_address_absy,
	[INDIRECT_ABSOLUTE] = get_address_indirect_absolute,
	[DP] = get_address_abs_and_dp,
	[DP_X] = get_address_dpx,
	[DP_Y] = get_address_dpy,
	[DP_INDIRECT_X] = get_address_dpindx,
	[DP_INDIRECT_Y] = get_address_dpindy
};

/* Generic handler for instructions. */
void instruction_handler(const struct instruction_handler_data *data, uint16_t operand) {
	uint16_t operand_address;
	uint8_t result;

	if (data->addressing_mode == IMPLIED || data->addressing_mode == IMMEDIATE)
		data->instruction_impl(operand & 0xff);
	else {
		operand_address = address_translators[data->addressing_mode](operand);

		if (data->instruction_destination == NONE || data->instruction_destination == CPU_REGISTER) {
			data->instruction_impl(mem_read(operand_address));
		}
		else {
			result = data->instruction_impl(mem_read(operand_address));
			mem_write(operand_address, result);
		}
	}

	cpu.regs.pc += data->instruction_size;
}

static void fetch_instruction(unsigned char *insn_data, uint16_t addr) {
	insn_data[0] = mem_read(addr);
	insn_data[1] = mem_read(addr + 1);
	insn_data[2] = mem_read(addr + 2);
}

void execution_loop(void) {
	struct instruction_description desc;
	uint16_t first_insn_addr;
	unsigned char insn_data[MAX_INSN_SIZE];

	/* Jump to address in reset vector and begin executing. */
	first_insn_addr = mem_read(ADDR_RESET_VEC);
	first_insn_addr |= mem_read(ADDR_RESET_VEC + 1) << 8;
	cpu.regs.pc = first_insn_addr;

	fetch_instruction(insn_data, cpu.regs.pc);

	while (parser_get_instruction_description((unsigned char *)&insn_data, MAX_INSN_SIZE, &desc) == 0) {
		instruction_handler(&opcode_to_handler_data[desc.opcode], desc.operand);
		fetch_instruction(insn_data, cpu.regs.pc);
	}
}

int cpu_power_on(enum memory_mode mem_mode) {
	int i;

	mmu_configure(mem_mode);

	cpu.regs.a = 0;
	cpu.regs.x = 0;
	cpu.regs.y = 0;
	cpu.regs.s = 0xfd;
	cpu.regs.p.bits.irq = SR_IRQ_DISABLED;
	cpu.regs.p.bits.carry = SR_CARRY_ON;
	cpu.regs.p.bits.negative = SR_NEGATIVE_ON;
	mem_write(REG_FRAME_COUNTER_CTL, 0); /* Enable frame irq. */
	mem_write(REG_SND_CHN, 0); /* Disable sound channels. */

	for (i = REG_SQ1_VOL; i <= REG_NOISE_HI; ++i) {
		mem_write(i, 0);
	}

	execution_loop();

	return 0;
}

int cpu_reset(void) {
	if (!cpu.is_powered_on) {
		return -1;
	}

	cpu.regs.s -= 3;
	cpu.regs.p.bits.irq = SR_IRQ_DISABLED;
	mem_write(REG_SND_CHN, 0); /* Disable sound channels. */

	return 0;
}
