#include <nessan-gtr/parser.h>
#include <stdint.h>
#include "cpu.h"
#include "cpu_internal.h"
#include "mmu.h"

#define ZERO_PAGE_LIMIT 0x100

static struct cpu cpu;

static void stack_push_word(uint16_t word) {
	mem_write(cpu.regs.s, word);
	cpu.regs.s -= 2;
}

static void stack_push_byte(uint8_t byte) {
	mem_write(cpu.regs.s, byte);
	--cpu.regs.s;
}

/* Instruction handlers. */

/*
* Operand is at the given address + x register;
*/
static inline uint8_t get_address_absx(uint16_t addr) {
	return addr + cpu.regs.x;
}

/*
* Operand is at the given address + y register;
*/
static inline uint8_t get_address_absy(uint16_t addr) {
	return addr + cpu.regs.y;
}

/*
* Operand is at the adress pointed to by addr.
*/
static inline uint8_t get_address_indirect_absolute(uint16_t addr) {
	return mem_read(addr);
}

/*
* Operand is at the given zero page address + x register;
* If addr + x overflows ZERO_PAGE_LIMIT, it simply wraps around.
*/
static inline uint8_t get_address_dpx(uint8_t addr) {
	return (addr + cpu.regs.x) % ZERO_PAGE_LIMIT;
}

/*
* Operand is at the given zero page  address + y register;
* If addr + y overflows ZERO_PAGE_LIMIT, it simply wraps around.
*/
static inline uint8_t get_address_dpy(uint8_t addr) {
	return (addr + cpu.regs.y) % ZERO_PAGE_LIMIT;
}

/*
* Operand is at the address pointed to by the sum of the given zero page address and the x register.
* If addr + x overflows ZERO_PAGE_LIMIT, it simply wraps around.
*/
static inline uint16_t get_address_dpindx(uint8_t addr) {
	return mem_read((addr + cpu.regs.x) % ZERO_PAGE_LIMIT);
}

/*
* Operand is at the address pointed to by the sum of the address at the zero page address and the y register.
* If addr + x overflows ZERO_PAGE_LIMIT, it simply wraps around.
*/
static inline uint16_t get_address_dpindy(uint8_t addr) {
	return mem_read(addr) + cpu.regs.y;
}

static uint8_t ASL_impl(uint8_t operand) {
	/* Insert highest bit into carry flag. */
	cpu.regs.p.bits.carry = operand & 0x80;
	operand <<= 1;
	/* Check if negative after operation. */
	cpu.regs.p.bits.negative = operand & 0x80;
	cpu.regs.p.bits.zero = !operand;

	return operand;
}

static void ASL_DP(uint16_t operand) {
	uint8_t ret;
	ret = ASL_impl(mem_read(operand));
	mem_write(operand, ret);
	cpu.regs.pc += 2;
}

static void ASL_Acc(uint16_t operand) {
	cpu.regs.a = ASL_impl(cpu.regs.a);
	++cpu.regs.pc;
}

static void ASL_Abs(uint16_t operand) {
	uint8_t ret;
	ret = ASL_impl(mem_read(operand));
	mem_write(operand, ret);
	cpu.regs.pc += 3;
}

static void ASL_DPX(uint16_t operand) {
	uint8_t ret;

	uint16_t actual_addr = get_address_absx(operand);
	ret = ASL_impl(mem_read(actual_addr));
	mem_write(actual_addr, ret);

	cpu.regs.pc += 2;
}

static void ASL_AbsX(uint16_t operand) {
	uint8_t ret;

	uint16_t actual_addr = get_address_absx(operand);
	ret = ASL_impl(mem_read(actual_addr));
	mem_write(actual_addr, ret);

	cpu.regs.pc += 3;
}

static void ORA_impl(uint8_t operand) {
	cpu.regs.a |= operand;
	cpu.regs.p.bits.negative = operand & 0x80; /* Test highest bit. */
	cpu.regs.p.bits.zero = !cpu.regs.a;
}

static void ORA_DPIndX(uint16_t operand) {
	ORA_impl(mem_read(get_address_dpindx(operand)));
	cpu.regs.pc += 2;
}

static void ORA_DP(uint16_t operand) {
	ORA_impl(mem_read(operand));
	cpu.regs.pc += 2;
}

static void ORA_Imm(uint16_t operand) {
	ORA_impl(operand);
	cpu.regs.pc += 2;
}

static void ORA_Abs(uint16_t operand) {
	ORA_impl(mem_read(operand));
	cpu.regs.pc += 3;
}

static void ORA_DPIndY(uint16_t operand) {
	ORA_impl(mem_read(get_address_dpindy(operand)));
	cpu.regs.pc += 2;
}

static void ORA_DPX(uint16_t operand) {
	ORA_impl(mem_read(get_address_dpx(operand)));
	cpu.regs.pc += 2;
}

static void ORA_AbsY(uint16_t operand) {
	ORA_impl(mem_read(get_address_absy(operand)));
	cpu.regs.pc += 3;
}

static void ORA_AbsX(uint16_t operand) {
	ORA_impl(mem_read(get_address_absx(operand)));
	cpu.regs.pc += 3;
}

static void BRK(uint16_t operand) {
	cpu.regs.p.bits.brk = SR_BRK_ON;
	cpu.regs.p.bits.irq = SR_IRQ_DISABLED;
	stack_push_word(cpu.regs.pc);
	stack_push_byte(cpu.regs.p.value);
	cpu.regs.pc = mem_read(ADDR_BRK_VEC);
}

static void SET(uint16_t operand) {
	cpu.regs.p.bits.irq = SR_IRQ_DISABLED;
	++cpu.regs.pc;
}

static void STA_Abs(uint16_t operand) {
	mem_write(operand, cpu.regs.a);
	cpu.regs.pc += 3;
}

static void LDX_Imm(uint16_t operand) {
	cpu.regs.x += operand & 0xff;
	cpu.regs.pc += 2;
}

static void LDA_Imm(uint16_t operand) {
	cpu.regs.a = operand & 0xff;
	cpu.regs.pc += 2;
}

static void CLD(uint16_t operand) {
	++cpu.regs.pc;
}

static instruction_logic opcode_to_insn_logic[] = {
	[0x00] = BRK,
	[0x01] = ORA_DPIndX,
	[0x05] = ORA_DP,
	[0x09] = ORA_Imm,
	[0x0D] = ORA_Abs,
	[0x11] = ORA_DPIndY,
	[0x15] = ORA_DPX,
	[0x19] = ORA_AbsY,
	[0x1D] = ORA_AbsX,
	[0x78] = SET,
	[0x8D] = STA_Abs,
	[0xA2] = LDX_Imm,
	[0xA9] = LDA_Imm,
	[0xD8] = CLD,
};


int execution_loop(void) {
	struct instruction_description desc;
	uint16_t first_insn_addr;
	unsigned char insn_data[MAX_INSN_SIZE];

	/* Jump to address in reset vector and begin executing. */
	first_insn_addr = mem_read(ADDR_RESET_VEC);
	cpu.regs.pc = first_insn_addr;
	insn_data[0] = (uint8_t)mem_read(cpu.regs.pc);
	*(uint16_t *)(insn_data + 1) = mem_read(cpu.regs.pc + 1);

	while (parser_get_instruction_description((unsigned char *)&insn_data, MAX_INSN_SIZE, &desc) == 0) {
		opcode_to_insn_logic[desc.opcode](desc.operand);
		insn_data[0] = (uint8_t)mem_read(cpu.regs.pc);
		*(uint16_t *)(insn_data + 1) = mem_read(cpu.regs.pc + 1);
	}

	return -1;
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

	if (execution_loop())
		return -1;

	return 0;
}

int cpu_reset(void) {
	if (!cpu.is_powered_on) {
		return -1;
	}

	cpu.regs.s -= 3;
	cpu.regs.p.bits.irq = SR_IRQ_DISABLED;
	mem_write(REG_SND_CHN, 0); /* Disable sound channels. */
}
