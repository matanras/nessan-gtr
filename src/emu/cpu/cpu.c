#include <nessan-gtr/parser.h>
#include "cpu.h"
#include "cpu_internal.h"
#include "mmu.h"

static struct cpu cpu;

static void stack_push_word(word_t word) {
	mem_write(cpu.regs.s, word);
	cpu.regs.s -= 2;
}

static void stack_push_byte(uint8_t byte) {
	mem_write(cpu.regs.s, byte);
	--cpu.regs.s;
}

/* Instruction handlers. */

static void ORA_impl(uint8_t operand) {
	cpu.regs.a |= operand;
	cpu.regs.p.bits.negative = operand & 0x80; /* Test highest bit. */
	cpu.regs.p.bits.zero = !cpu.regs.a;
}

static void ORA_DPIndX(word_t operand) {
	uint8_t zero_page_addr = (operand & 0xff + cpu.regs.x) % 0xff;
	word_t actual_operand_addr = mem_read(zero_page_addr);
	ORA_impl(mem_read(actual_operand_addr) & 0xff);
	cpu.regs.pc += 2;
}

static void ORA_DP(word_t operand) {
	ORA_impl(mem_read(operand & 0xff));
	cpu.regs.pc += 2;
}

static void ORA_Imm(word_t operand) {
	ORA_impl(operand & 0xff);
	cpu.regs.pc += 2;
}

static void ORA_Abs(word_t operand) {
	ORA_impl(mem_read(operand));
	cpu.regs.pc += 3;
}

static void ORA_DPIndY(word_t operand) {
	word_t actual_operand_addr = mem_read(operand) + cpu.regs.y;
	ORA_impl(mem_read(actual_operand_addr) & 0xff);
	cpu.regs.pc += 2;
}

static void ORA_DPX(word_t operand) {
	uint8_t actual_operand = mem_read(operand & 0xff + cpu.regs.x);
	ORA_impl(actual_operand);
	cpu.regs.pc += 2;
}

static void ORA_AbsY(word_t operand) {
	ORA_impl(mem_read((operand + cpu.regs.y) % 0xffff));
	cpu.regs.pc += 3;
}

static void ORA_AbsX(word_t operand) {
	ORA_impl(mem_read((operand + cpu.regs.x) % 0xffff));
	cpu.regs.pc += 3;
}

static void BRK(word_t operand) {
	cpu.regs.p.bits.brk = SR_BRK_ON;
	cpu.regs.p.bits.irq = SR_IRQ_DISABLED;
	stack_push_word(cpu.regs.pc);
	stack_push_byte(cpu.regs.p.value);
	cpu.regs.pc = mem_read(ADDR_BRK_VEC);
}

static void SET(word_t operand) {
	cpu.regs.p.bits.irq = SR_IRQ_DISABLED;
	++cpu.regs.pc;
}

static void STA_Abs(word_t operand) {
	mem_write(operand, cpu.regs.a);
	cpu.regs.pc += 3;
}

static void LDX_Imm(word_t operand) {
	cpu.regs.x += operand & 0xff;
	cpu.regs.pc += 2;
}

static void LDA_Imm(word_t operand) {
	cpu.regs.a = operand & 0xff;
	cpu.regs.pc += 2;
}

static void CLD(word_t operand) {
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
	word_t first_insn_addr;
	unsigned char insn_data[MAX_INSN_SIZE];

	/* Jump to address in reset vector and begin executing. */
	first_insn_addr = mem_read(ADDR_RESET_VEC);
	cpu.regs.pc = first_insn_addr;
	insn_data[0] = (uint8_t)mem_read(cpu.regs.pc);
	*(word_t *)(insn_data + 1) = mem_read(cpu.regs.pc + 1);

	while (parser_get_instruction_description((unsigned char *)&insn_data, MAX_INSN_SIZE, &desc) == 0) {
		opcode_to_insn_logic[desc.opcode](desc.operand);
		insn_data[0] = (uint8_t)mem_read(cpu.regs.pc);
		*(word_t *)(insn_data + 1) = mem_read(cpu.regs.pc + 1);
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
	cpu.regs.s -= 3;
	cpu.regs.p.bits.irq = SR_IRQ_DISABLED;
	mem_write(REG_SND_CHN, 0); /* Disable sound channels. */
}
