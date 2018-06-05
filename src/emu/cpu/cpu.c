#include <nessan-gtr/parser.h>
#include <stdint.h>
#include <assert.h>
#include "cpu.h"
#include "cpu_internal.h"
#include "mmu.h"
#include <stdio.h>

#define ZERO_PAGE_LIMIT 0x100

/**
* Test if the bit at the given index is set.
* @param n the number to test.
* @param index the index of the bit to test.
* @return the value of the bit at the specified index.
*/
static inline int testbit(uint8_t n, uint8_t index) {
	assert(index >= 0 && index <= 7);

	return (n & (1 << index)) >> index;
}

static struct cpu cpu;

static void stack_push_byte(uint8_t byte) {
	mem_write(cpu.regs.s, byte);
	--cpu.regs.s;
}

static void stack_push_word(uint16_t word) {
	mem_write(cpu.regs.s, word & 0xff);
	mem_write(cpu.regs.s + 1, word & 0xff00);
	cpu.regs.s -= 2;
}


uint8_t stack_pop_byte(void) {
	return mem_read(cpu.regs.s++);
}

uint16_t stack_pop_word(void) {
	uint16_t result = mem_read(cpu.regs.s + 1) << 8 | mem_read(cpu.regs.s);
	cpu.regs.s += 2;
	return result;
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
	/*
	* The early revisions of 6502 had a bug where if the word operand crosses a page boundry
	* (for example 0x1ff and 0x200), the MSB is read from the first byte of the same page, instead of the next.
	*/
	if ((addr & 0xff) == 0xff) {
		return mem_read(addr & ~0xff) << 8 | mem_read(addr);
	}
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
	cpu.regs.p.bits.carry = testbit(operand, 7);
	operand <<= 1;
	/* Check if negative after operation. */
	cpu.regs.p.bits.negative = testbit(operand, 7);
	cpu.regs.p.bits.zero = !operand;

	return operand;
}

static uint8_t ORA(uint8_t operand) {
	cpu.regs.a |= operand;
	cpu.regs.p.bits.negative = testbit(operand, 7);
	cpu.regs.p.bits.zero = !cpu.regs.a;

	return 0;
}

static uint8_t ADC(uint8_t operand) {
	uint8_t result;
	result = operand + cpu.regs.a + cpu.regs.p.bits.carry;
	cpu.regs.p.bits.negative = testbit(operand, 7);

	/*
	* Overflow flag is set only if register A and the operand had the same sign and the result
	* changed the sign.
	*                             check if
	*               check if  	   result         test the
	*				 same sign	   changed sign   sign
	* overflow = ~(A ^ operand) & (A ^ result) & highbit
	*/
	cpu.regs.p.bits.overflow = testbit(~(cpu.regs.a ^ operand) & (cpu.regs.a ^ result), 7);
	cpu.regs.p.bits.carry = testbit(result, 7);
	cpu.regs.a = result;
	cpu.regs.p.bits.zero = !result;

	return 0;
}

static uint8_t AND(uint8_t operand) {
	cpu.regs.a &= operand;
	cpu.regs.p.bits.negative = testbit(cpu.regs.a, 7);
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

static uint8_t LDA(uint8_t operand) {
	cpu.regs.a = operand;

	return 0;
}

static uint8_t LDX(uint8_t operand) {
	cpu.regs.x = operand;
	cpu.regs.p.bits.negative = testbit(cpu.regs.x, 7);
	cpu.regs.p.bits.zero = !cpu.regs.x;

	return 0;
}

static uint8_t LDY(uint8_t operand) {
	cpu.regs.y = operand;
	cpu.regs.p.bits.negative = testbit(cpu.regs.y, 7);
	cpu.regs.p.bits.zero = !cpu.regs.y;

	return 0;
}

static uint8_t CLC(uint8_t operand) {
	cpu.regs.p.bits.carry = SR_CARRY_OFF;

	return 0;
}

static uint8_t CLD(uint8_t operand) {
	return 0;
}

static uint8_t CLI(uint8_t operand) {
	cpu.regs.p.bits.irq = SR_IRQ_ENABLED;

	return 0;
}

static uint8_t CLV(uint8_t operand) {
	cpu.regs.p.bits.overflow = SR_OVERFLOW_OFF;

	return 0;
}

static uint8_t BCC(uint8_t operand) {
	if (!cpu.regs.p.bits.carry)
		cpu.regs.pc += (int8_t)operand;
	else
		cpu.regs.pc += 2;

	return 0;
}

static uint8_t BCS(uint8_t operand) {
	if (cpu.regs.p.bits.carry)
		cpu.regs.pc += (int8_t)operand;
	else
		cpu.regs.pc += 2;

	return 0;
}

static uint8_t BEQ(uint8_t operand) {
	if (cpu.regs.p.bits.zero)
		cpu.regs.pc += (int8_t)operand;
	else
		cpu.regs.pc += 2;

	return 0;
}

static uint8_t BNE(uint8_t operand) {
	if (!cpu.regs.p.bits.zero)
		cpu.regs.pc += (int8_t)operand;
	else
		cpu.regs.pc += 2;

	return 0;
}

static uint8_t BMI(uint8_t operand) {
	if (cpu.regs.p.bits.negative)
		cpu.regs.pc += (int8_t)operand;
	else
		cpu.regs.pc += 2;

	return 0;
}

static uint8_t BPL(uint8_t operand) {
	if (!cpu.regs.p.bits.negative)
		cpu.regs.pc += (int8_t)operand;
	else
		cpu.regs.pc += 2;

	return 0;
}

static uint8_t BVC(uint8_t operand) {
	if (!cpu.regs.p.bits.overflow)
		cpu.regs.pc += (int8_t)operand;
	else
		cpu.regs.pc += 2;

	return 0;
}

static uint8_t BVS(uint8_t operand) {
	if (cpu.regs.p.bits.overflow)
		cpu.regs.pc += (int8_t)operand;
	else
		cpu.regs.pc += 2;

	return 0;
}

static uint8_t CMP(uint8_t operand) {
	cpu.regs.p.bits.negative = testbit(cpu.regs.a - operand, 7);
	cpu.regs.p.bits.zero = cpu.regs.a == operand;
	cpu.regs.p.bits.carry = cpu.regs.a >= operand;

	return 0;
}

static uint8_t CPX(uint8_t operand) {
	cpu.regs.p.bits.negative = testbit(cpu.regs.x - operand, 7);
	cpu.regs.p.bits.zero = cpu.regs.x == operand;
	cpu.regs.p.bits.carry = cpu.regs.x >= operand;

	return 0;
}

static uint8_t CPY(uint8_t operand) {
	cpu.regs.p.bits.negative = testbit(cpu.regs.y - operand, 7);
	cpu.regs.p.bits.zero = cpu.regs.y == operand;
	cpu.regs.p.bits.carry = cpu.regs.y >= operand;

	return 0;
}

static uint8_t DEC(uint8_t operand) {
	uint8_t result = operand - 1;
	cpu.regs.p.bits.negative = testbit(result, 7);
	cpu.regs.p.bits.zero = !result;

	return result;
}

static uint8_t DEY(uint8_t operand) {
	--cpu.regs.y;
	cpu.regs.p.bits.negative = testbit(cpu.regs.y, 7);
	cpu.regs.p.bits.zero = !cpu.regs.y;

	return 0;
}

static uint8_t DEX(uint8_t operand) {
	--cpu.regs.x;
	cpu.regs.p.bits.negative = testbit(cpu.regs.x, 7);
	cpu.regs.p.bits.zero = !cpu.regs.x;

	return 0;
}

static uint8_t EOR(uint8_t operand) {
	cpu.regs.a ^= operand;
	cpu.regs.p.bits.negative = testbit(cpu.regs.a, 7);
	cpu.regs.p.bits.zero = !cpu.regs.a;

	return 0;
}

static uint8_t INC(uint8_t operand) {
	uint8_t result = operand + 1;
	cpu.regs.p.bits.negative = testbit(result, 7);
	cpu.regs.p.bits.zero = !result;

	return result;
}

static uint8_t INX(uint8_t operand) {
	++cpu.regs.x;
	cpu.regs.p.bits.negative = testbit(cpu.regs.x, 7);
	cpu.regs.p.bits.zero = !cpu.regs.x;

	return 0;
}

static uint8_t INY(uint8_t operand) {
	--cpu.regs.y;
	cpu.regs.p.bits.negative = testbit(cpu.regs.y, 7);
	cpu.regs.p.bits.zero = !cpu.regs.y;

	return 0;
}

static void JMP(uint16_t operand) {
	cpu.regs.pc = operand;
}

static void JSR(uint16_t operand) {
	stack_push_word(cpu.regs.pc + 3);
	cpu.regs.pc = operand;
}

static uint8_t LSR(uint8_t operand) {
	uint8_t result;

	cpu.regs.p.bits.carry = testbit(operand, 0);
	result = operand >> 1;
	cpu.regs.p.bits.negative = SR_NEGATIVE_OFF;
	cpu.regs.p.bits.zero = !result;

	return result;
}

static uint8_t NOP(uint8_t operand) {
	return 0;
}

static uint8_t PHA(uint8_t operand) {
	stack_push_byte(cpu.regs.a);

	return 0;
}

static uint8_t PHP(uint8_t operand) {
	cpu.regs.p.bits.brk = SR_BRK_ON;
	stack_push_byte(cpu.regs.p.value);
	cpu.regs.p.bits.brk = SR_BRK_OFF;

	return 0;
}

static uint8_t PLA(uint8_t operand) {
	cpu.regs.a = stack_pop_byte();
	cpu.regs.p.bits.negative = testbit(cpu.regs.a, 7);
	cpu.regs.p.bits.zero = !cpu.regs.a;

	return 0;
}

static uint8_t PLP(uint8_t operand) {
	cpu.regs.p.value = (cpu.regs.p.value & 0x30) | (stack_pop_byte() & 0xcf); /* Ignore bits brk and 5. */

	return 0;
}

static uint8_t ROL(uint8_t operand) {
	uint8_t result;

	if (cpu.regs.p.bits.carry) {
		cpu.regs.p.bits.carry = testbit(operand, 7);
		result = (operand << 1) + 1;
	}
	else {
		cpu.regs.p.bits.carry = testbit(operand, 7);
		result = operand << 1;
	}

	cpu.regs.p.bits.negative = testbit(result, 7);
	cpu.regs.p.bits.zero = !result;

	return 0;
}

static uint8_t ROR(uint8_t operand) {
	uint8_t result;

	if (cpu.regs.p.bits.carry) {
		cpu.regs.p.bits.carry = testbit(operand, 0);
		result = operand >> 1;
		result &= 7; /* Turn on highest bit. */
	}
	else {
		cpu.regs.p.bits.carry = testbit(operand, 0);
		result = operand >> 1;
	}

	cpu.regs.p.bits.negative = cpu.regs.p.bits.carry;
	cpu.regs.p.bits.zero = !result;

	return 0;
}

static uint8_t RTI(uint8_t operand) {
	cpu.regs.p.value = (cpu.regs.p.value & 0x30) | (stack_pop_byte() & 0xcf); /* Perserve bits 4 and 5*/
	cpu.regs.pc = stack_pop_word();

	return 0;
}

static uint8_t RTS(uint8_t operand) {
	cpu.regs.pc = stack_pop_word();

	return 0;
}

static uint8_t SBC(uint8_t operand) {
	uint8_t result = operand;

	if (!cpu.regs.p.bits.carry)
		++result;

	cpu.regs.p.bits.carry = result <= cpu.regs.a;
	result = cpu.regs.a - result;
	cpu.regs.p.bits.overflow = testbit((cpu.regs.a ^ operand) & (cpu.regs.a ^ result), 7);
	cpu.regs.p.bits.negative = testbit(result, 7);
	cpu.regs.p.bits.zero = !result;

	cpu.regs.a = result;

	return 0;
}

static uint8_t STA(uint8_t operand) {
	return cpu.regs.a;
}

static uint8_t STX(uint8_t operand) {
	return cpu.regs.x;
}

static uint8_t STY(uint8_t operand) {
	return cpu.regs.x;
}

static uint8_t SEC(uint8_t operand) {
	cpu.regs.p.bits.carry = SR_CARRY_ON;

	return 0;
}

static uint8_t SED(uint8_t operand) {
	return 0;
}

static uint8_t SEI(uint8_t operand) {
	cpu.regs.p.bits.irq = SR_IRQ_DISABLED;

	return 0;
}

static uint8_t TAX(uint8_t operand) {
	cpu.regs.x = cpu.regs.a;
	cpu.regs.p.bits.negative = testbit(cpu.regs.a, 7);
	cpu.regs.p.bits.zero = !cpu.regs.a;

	return 0;
}

static uint8_t TAY(uint8_t operand) {
	cpu.regs.y = cpu.regs.a;
	cpu.regs.p.bits.negative = testbit(cpu.regs.a, 7);
	cpu.regs.p.bits.zero = !cpu.regs.a;

	return 0;
}

static uint8_t TSX(uint8_t operand) {
	cpu.regs.x = cpu.regs.s;
	cpu.regs.p.bits.negative = testbit(cpu.regs.x, 7);
	cpu.regs.p.bits.zero = !cpu.regs.x;

	return 0;
}

static uint8_t TXA(uint8_t operand) {
	cpu.regs.a = cpu.regs.x;
	cpu.regs.p.bits.negative = testbit(cpu.regs.a, 7);
	cpu.regs.p.bits.zero = !cpu.regs.a;

	return 0;
}

static uint8_t TYA(uint8_t operand) {
	cpu.regs.a = cpu.regs.y;
	cpu.regs.p.bits.negative = testbit(cpu.regs.a, 7);
	cpu.regs.p.bits.zero = !cpu.regs.a;

	return 0;
}

static uint8_t TXS(uint8_t operand) {
	cpu.regs.s = cpu.regs.x;
	cpu.regs.p.bits.negative = testbit(cpu.regs.x, 7);
	cpu.regs.p.bits.zero = !cpu.regs.x;

	return 0;
}

static struct instruction_handler_data_ext opcode_to_handler_data_ext[0xff] = {
	[0x20] = {
		.instruction_impl = JSR,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = CPU_REGISTER_PC,
	},
	[0x4c] = {
		.instruction_impl = JMP,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = CPU_REGISTER_PC,
	},
	[0x6c] = {
		.instruction_impl = JMP,
		.addressing_mode = INDIRECT_ABSOLUTE,
		.instruction_destination = CPU_REGISTER_PC,
	},
};
static struct instruction_handler_data opcode_to_handler_data[] = {
	[0x00] = {
		.instruction_impl = BRK,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER_PC,
	},
	[0x01] = {
		.instruction_impl = ORA,
		.addressing_mode = DP_INDIRECT_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0x05] = {
		.instruction_impl = ORA,
		.addressing_mode = DP,
		.instruction_destination = CPU_REGISTER,
	},
	[0x06] = {
		.instruction_impl = ASL,
		.addressing_mode = DP,
		.instruction_destination = MEMORY,
	},
	[0x08] = {
		.instruction_impl = PHP,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0x09] = {
		.instruction_impl = ORA,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER,
	},
	[0x0a] = {
		.instruction_impl = ASL,
		.addressing_mode = ACCUMULATOR,
		.instruction_destination = CPU_REGISTER,
	},
	[0x0d] = {
		.instruction_impl = ORA,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = CPU_REGISTER,
	},
	[0x0e] = {
		.instruction_impl = ASL,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = MEMORY,
	},
	[0x10] = {
		.instruction_impl = BPL,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER_PC,
	},
	[0x11] = {
		.instruction_impl = ORA,
		.addressing_mode = DP_INDIRECT_Y,
		.instruction_destination = CPU_REGISTER,
	},
	[0x15] = {
		.instruction_impl = ORA,
		.addressing_mode = DP_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0x16] = {
		.instruction_impl = ASL,
		.addressing_mode = DP_X,
		.instruction_destination = MEMORY,
	},
	[0x18] = {
		.instruction_impl = CLC,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0x19] = {
		.instruction_impl = ORA,
		.addressing_mode = ABSOLUTE_Y,
		.instruction_destination = CPU_REGISTER,
	},
	[0x1d] = {
		.instruction_impl = ORA,
		.addressing_mode = ABSOLUTE_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0x1e] = {
		.instruction_impl = ASL,
		.addressing_mode = ABSOLUTE_X,
		.instruction_destination = MEMORY,
	},
	[0x21] = {
		.instruction_impl = AND,
		.addressing_mode = DP_INDIRECT_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0x25] = {
		.instruction_impl = AND,
		.addressing_mode = DP,
		.instruction_destination = CPU_REGISTER,
	},
	[0x26] = {
		.instruction_impl = ROL,
		.addressing_mode = DP,
		.instruction_destination = MEMORY,
	},
	[0x28] = {
		.instruction_impl = PLP,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0x29] = {
		.instruction_impl = AND,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER,
	},
	[0x2a] = {
		.instruction_impl = ROL,
		.addressing_mode = ACCUMULATOR,
		.instruction_destination = CPU_REGISTER,
	},
	[0x2d] = {
		.instruction_impl = AND,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = CPU_REGISTER,
	},
	[0x2e] = {
		.instruction_impl = ROL,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = MEMORY,
	},
	[0x30] = {
		.instruction_impl = BMI,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER_PC,
	},
	[0x31] = {
		.instruction_impl = AND,
		.addressing_mode = DP_INDIRECT_Y,
		.instruction_destination = CPU_REGISTER,
	},
	[0x35] = {
		.instruction_impl = AND,
		.addressing_mode = DP_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0x36] = {
		.instruction_impl = ROL,
		.addressing_mode = DP_X,
		.instruction_destination = MEMORY,
	},
	[0x38] = {
		.instruction_impl = ROL,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0x39] = {
		.instruction_impl = AND,
		.addressing_mode = ABSOLUTE_Y,
		.instruction_destination = CPU_REGISTER,
	},
	[0x3d] = {
		.instruction_impl = AND,
		.addressing_mode = ABSOLUTE_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0x3e] = {
		.instruction_impl = ROL,
		.addressing_mode = ABSOLUTE_X,
		.instruction_destination = MEMORY,
	},
	[0x41] = {
		.instruction_impl = RTI,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER_PC,
	},
	[0x41] = {
		.instruction_impl = EOR,
		.addressing_mode = DP_INDIRECT_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0x45] = {
		.instruction_impl = EOR,
		.addressing_mode = DP,
		.instruction_destination = CPU_REGISTER,
	},
	[0x46] = {
		.instruction_impl = LSR,
		.addressing_mode = DP,
		.instruction_destination = MEMORY,
	},
	[0x48] = {
		.instruction_impl = PHA,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0x49] = {
		.instruction_impl = EOR,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER,
	},
	[0x4a] = {
		.instruction_impl = LSR,
		.addressing_mode = ACCUMULATOR,
		.instruction_destination = CPU_REGISTER,
	},
	[0x4d] = {
		.instruction_impl = EOR,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = CPU_REGISTER,
	},
	[0x4e] = {
		.instruction_impl = LSR,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = MEMORY,
	},
	[0x50] = {
		.instruction_impl = BVC,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER_PC,
	},
	[0x51] = {
		.instruction_impl = EOR,
		.addressing_mode = DP_INDIRECT_Y,
		.instruction_destination = CPU_REGISTER,
	},
	[0x55] = {
		.instruction_impl = EOR,
		.addressing_mode = DP_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0x56] = {
		.instruction_impl = LSR,
		.addressing_mode = DP_X,
		.instruction_destination = MEMORY,
	},
	[0x58] = {
		.instruction_impl = CLI,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0x59] = {
		.instruction_impl = EOR,
		.addressing_mode = ABSOLUTE_Y,
		.instruction_destination = CPU_REGISTER,
	},
	[0x5d] = {
		.instruction_impl = EOR,
		.addressing_mode = ABSOLUTE_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0x5e] = {
		.instruction_impl = LSR,
		.addressing_mode = ABSOLUTE_X,
		.instruction_destination = MEMORY,
	},
	[0x60] = {
		.instruction_impl = RTS,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER_PC,
	},
	[0x61] = {
		.instruction_impl = ADC,
		.addressing_mode = DP_INDIRECT_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0x65] = {
		.instruction_impl = ADC,
		.addressing_mode = DP,
		.instruction_destination = CPU_REGISTER,
	},
	[0x66] = {
		.instruction_impl = ROR,
		.addressing_mode = DP,
		.instruction_destination = MEMORY,
	},
	[0x69] = {
		.instruction_impl = PLA,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0x69] = {
		.instruction_impl = ADC,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER,
	},
	[0x6a] = {
		.instruction_impl = ROR,
		.addressing_mode = ACCUMULATOR,
		.instruction_destination = CPU_REGISTER,
	},
	[0x6d] = {
		.instruction_impl = ADC,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = CPU_REGISTER,
	},
	[0x6e] = {
		.instruction_impl = ROR,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = MEMORY,
	},
	[0x70] = {
		.instruction_impl = BVS,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER_PC,
	},
	[0x71] = {
		.instruction_impl = ADC,
		.addressing_mode = DP_INDIRECT_Y,
		.instruction_destination = CPU_REGISTER,
	},
	[0x75] = {
		.instruction_impl = ADC,
		.addressing_mode = DP_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0x76] = {
		.instruction_impl = ROR,
		.addressing_mode = DP_X,
		.instruction_destination = MEMORY,
	},
	[0x78] = {
		.instruction_impl = SEI,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0x79] = {
		.instruction_impl = ADC,
		.addressing_mode = ABSOLUTE_Y,
		.instruction_destination = CPU_REGISTER,
	},
	[0x7d] = {
		.instruction_impl = ADC,
		.addressing_mode = ABSOLUTE_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0x7e] = {
		.instruction_impl = ROR,
		.addressing_mode = ABSOLUTE_X,
		.instruction_destination = MEMORY,
	},
	[0x81] = {
		.instruction_impl = STA,
		.addressing_mode = DP_INDIRECT_X,
		.instruction_destination = MEMORY,
	},
	[0x84] = {
		.instruction_impl = STY,
		.addressing_mode = DP,
		.instruction_destination = MEMORY,
	},
	[0x85] = {
		.instruction_impl = STA,
		.addressing_mode = DP,
		.instruction_destination = MEMORY,
	},
	[0x86] = {
		.instruction_impl = STX,
		.addressing_mode = DP,
		.instruction_destination = MEMORY,
	},
	[0x88] = {
		.instruction_impl = DEY,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0x8a] = {
		.instruction_impl = TXA,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0x8c] = {
		.instruction_impl = STY,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = MEMORY,
	},
	[0x8d] = {
		.instruction_impl = STA,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = MEMORY,
	},
	[0x8e] = {
		.instruction_impl = STX,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = MEMORY,
	},
	[0x90] = {
		.instruction_impl = BCC,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER,
	},
	[0x91] = {
		.instruction_impl = STA,
		.addressing_mode = DP_INDIRECT_Y,
		.instruction_destination = MEMORY,
	},
	[0x94] = {
		.instruction_impl = STY,
		.addressing_mode = DP_X,
		.instruction_destination = MEMORY,
	},
	[0x95] = {
		.instruction_impl = STA,
		.addressing_mode = DP_X,
		.instruction_destination = MEMORY,
	},
	[0x96] = {
		.instruction_impl = STX,
		.addressing_mode = DP_Y,
		.instruction_destination = MEMORY,
	},
	[0x98] = {
		.instruction_impl = TYA,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0x99] = {
		.instruction_impl = STA,
		.addressing_mode = ABSOLUTE_Y,
		.instruction_destination = MEMORY,
	},
	[0x9a] = {
		.instruction_impl = TXS,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0x9d] = {
		.instruction_impl = STA,
		.addressing_mode = ABSOLUTE_X,
		.instruction_destination = MEMORY,
	},
	[0xa0] = {
		.instruction_impl = LDY,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER,
	},
	[0xa1] = {
		.instruction_impl = LDA,
		.addressing_mode = DP_INDIRECT_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0xa2] = {
		.instruction_impl = LDX,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER,
	},
	[0xa4] = {
		.instruction_impl = LDY,
		.addressing_mode = DP,
		.instruction_destination = CPU_REGISTER,
	},
	[0xa5] = {
		.instruction_impl = LDA,
		.addressing_mode = DP,
		.instruction_destination = CPU_REGISTER,
	},
	[0xa6] = {
		.instruction_impl = LDX,
		.addressing_mode = DP,
		.instruction_destination = CPU_REGISTER,
	},
	[0xa8] = {
		.instruction_impl = TAY,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0xa9] = {
		.instruction_impl = LDA,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER,
	},
	[0xaa] = {
		.instruction_impl = TAX,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0xac] = {
		.instruction_impl = LDY,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = CPU_REGISTER,
	},
	[0xad] = {
		.instruction_impl = LDA,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = CPU_REGISTER,
	},
	[0xae] = {
		.instruction_impl = LDX,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = CPU_REGISTER,
	},
	[0xb0] = {
		.instruction_impl = BCS,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER,
	},
	[0xb1] = {
		.instruction_impl = LDA,
		.addressing_mode = DP_INDIRECT_Y,
		.instruction_destination = CPU_REGISTER,
	},
	[0xb4] = {
		.instruction_impl = LDY,
		.addressing_mode = DP_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0xb5] = {
		.instruction_impl = LDA,
		.addressing_mode = DP_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0xb8] = {
		.instruction_impl = CLV,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0xb9] = {
		.instruction_impl = LDA,
		.addressing_mode = ABSOLUTE_Y,
		.instruction_destination = CPU_REGISTER,
	},
	[0xba] = {
		.instruction_impl = TSX,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0xbc] = {
		.instruction_impl = LDY,
		.addressing_mode = ABSOLUTE_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0xbd] = {
		.instruction_impl = LDA,
		.addressing_mode = ABSOLUTE_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0xc0] = {
		.instruction_impl = CPY,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER,
	},
	[0xc1] = {
		.instruction_impl = CMP,
		.addressing_mode = DP_INDIRECT_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0xc4] = {
		.instruction_impl = CPY,
		.addressing_mode = DP,
		.instruction_destination = CPU_REGISTER,
	},
	[0xc5] = {
		.instruction_impl = CMP,
		.addressing_mode = DP,
		.instruction_destination = CPU_REGISTER,
	},
	[0xc6] = {
		.instruction_impl = DEC,
		.addressing_mode = DP,
		.instruction_destination = MEMORY,
	},
	[0xc8] = {
		.instruction_impl = INY,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0xc9] = {
		.instruction_impl = CMP,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER,
	},
	[0xca] = {
		.instruction_impl = DEX,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0xcc] = {
		.instruction_impl = CPY,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = CPU_REGISTER,
	},
	[0xcd] = {
		.instruction_impl = CMP,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = CPU_REGISTER,
	},
	[0xce] = {
		.instruction_impl = DEC,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = MEMORY,
	},
	[0xd0] = {
		.instruction_impl = BNE,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER_PC,
	},
	[0xd1] = {
		.instruction_impl = CMP,
		.addressing_mode = DP_INDIRECT_Y,
		.instruction_destination = CPU_REGISTER,
	},
	[0xd5] = {
		.instruction_impl = CMP,
		.addressing_mode = DP_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0xd6] = {
		.instruction_impl = DEC,
		.addressing_mode = DP_X,
		.instruction_destination = MEMORY,
	},
	[0xd8] = {
		.instruction_impl = CLD,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0xd9] = {
		.instruction_impl = CMP,
		.addressing_mode = ABSOLUTE_Y,
		.instruction_destination = CPU_REGISTER,
	},
	[0xdd] = {
		.instruction_impl = CMP,
		.addressing_mode = ABSOLUTE_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0xde] = {
		.instruction_impl = DEC,
		.addressing_mode = ABSOLUTE_X,
		.instruction_destination = MEMORY,
	},
	[0xe0] = {
		.instruction_impl = CPX,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER,
	},
	[0xe1] = {
		.instruction_impl = SBC,
		.addressing_mode = DP_INDIRECT_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0xe4] = {
		.instruction_impl = CPX,
		.addressing_mode = DP,
		.instruction_destination = CPU_REGISTER,
	},
	[0xe5] = {
		.instruction_impl = SBC,
		.addressing_mode = DP,
		.instruction_destination = CPU_REGISTER,
	},
	[0xe6] = {
		.instruction_impl = INC,
		.addressing_mode = DP,
		.instruction_destination = MEMORY,
	},
	[0xe8] = {
		.instruction_impl = INX,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0xe9] = {
		.instruction_impl = SBC,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER,
	},
	[0xea] = {
		.instruction_impl = NOP,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0xec] = {
		.instruction_impl = CPX,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = CPU_REGISTER,
	},
	[0xed] = {
		.instruction_impl = SBC,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = CPU_REGISTER,
	},
	[0xee] = {
		.instruction_impl = INC,
		.addressing_mode = ABSOLUTE,
		.instruction_destination = MEMORY,
	},
	[0xf0] = {
		.instruction_impl = BEQ,
		.addressing_mode = IMMEDIATE,
		.instruction_destination = CPU_REGISTER_PC,
	},
	[0xf1] = {
		.instruction_impl = SBC,
		.addressing_mode = DP_INDIRECT_Y,
		.instruction_destination = CPU_REGISTER,
	},
	[0xf5] = {
		.instruction_impl = SBC,
		.addressing_mode = DP_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0xf6] = {
		.instruction_impl = INC,
		.addressing_mode = DP_X,
		.instruction_destination = MEMORY,
	},
	[0xf8] = {
		.instruction_impl = SED,
		.addressing_mode = IMPLIED,
		.instruction_destination = CPU_REGISTER,
	},
	[0xf9] = {
		.instruction_impl = SBC,
		.addressing_mode = ABSOLUTE_Y,
		.instruction_destination = CPU_REGISTER,
	},
	[0xfd] = {
		.instruction_impl = SBC,
		.addressing_mode = ABSOLUTE_X,
		.instruction_destination = CPU_REGISTER,
	},
	[0xfe] = {
		.instruction_impl = INC,
		.addressing_mode = ABSOLUTE_X,
		.instruction_destination = MEMORY,
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

static void ext_instruction_handler(const struct instruction_handler_data_ext *data, uint16_t operand) {
	uint16_t operand_addr = address_translators[data->addressing_mode](operand);
	data->instruction_impl(operand_addr);
}

static uint8_t get_insn_size(const struct instruction_handler_data *data) {
	if (data->instruction_destination == CPU_REGISTER_PC)
		return 0;

	switch (data->addressing_mode) {
		case IMPLIED:
		case ACCUMULATOR:
			return 1;

		case DP:
		case DP_X:
		case DP_Y:
		case IMMEDIATE:
		case DP_INDIRECT_X:
		case DP_INDIRECT_Y:
			return 2;

		case ABSOLUTE:
		case ABSOLUTE_X:
		case ABSOLUTE_Y:
			return 3;

		default:
			/* Should never reach here. */
			return 0;
	}
}

/* Generic handler for instructions. */
static void instruction_handler(const struct instruction_handler_data *data, uint16_t operand) {
	uint16_t operand_address;
	uint8_t result;

	if (data->addressing_mode == IMPLIED || data->addressing_mode == IMMEDIATE) {
		data->instruction_impl(operand & 0xff);
	}
	else if (data->addressing_mode == ACCUMULATOR) {
		cpu.regs.a = data->instruction_impl(cpu.regs.a);
	}
	else {
		operand_address = address_translators[data->addressing_mode](operand);

		if (data->instruction_destination == CPU_REGISTER) {
			data->instruction_impl(mem_read(operand_address));
		}
		else {
			result = data->instruction_impl(mem_read(operand_address));
			mem_write(operand_address, result);
		}
	}

	cpu.regs.pc += get_insn_size(data);
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
	unsigned char next[MAX_INSN_SIZE];

	/* Jump to address in reset vector and begin executing. */
	first_insn_addr = mem_read(ADDR_RESET_VEC);
	first_insn_addr |= mem_read(ADDR_RESET_VEC + 1) << 8;
	cpu.regs.pc = first_insn_addr;

	fetch_instruction(insn_data, cpu.regs.pc);

	while (parser_get_instruction_description(insn_data, MAX_INSN_SIZE, &desc) == 0) {
		if (opcode_to_handler_data_ext[desc.opcode].instruction_impl)
			ext_instruction_handler(&opcode_to_handler_data_ext[desc.opcode], desc.operand);
		else
			instruction_handler(&opcode_to_handler_data[desc.opcode], desc.operand);
		
		++cpu.executed_instructions;
		
		fetch_instruction(next, cpu.regs.pc);
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
